#! /bin/bash
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/etc/probemodem.sh,v 1.1.1.1 1994/01/14 23:09:41 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1993 Sam Leffler
# Copyright (c) 1993 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#

#
# probemodem [tty]
#
# This script probes a modem attached to a serial line and
# reports the results of certain commands.
#
PATH=/bin:/usr/bin:/etc
test -d /usr/ucb  && PATH=$PATH:/usr/ucb		# Sun and others
test -d /usr/bsd  && PATH=$PATH:/usr/bsd		# Silicon Graphics
test -d /usr/5bin && PATH=/usr/5bin:$PATH:/usr/etc	# Sun and others
test -d /usr/sbin && PATH=/usr/sbin:$PATH		# 4.4BSD-derived

if [ -d /etc/saf ]; then
    # uname -s is unreliable on svr4 as it can return the nodename
    OS=svr4
else
    OS=`uname -s 2>/dev/null || echo unknown`	# system identification
fi
SPEED=2400					# rate for talking to modem

while [ x"$1" != x"" ] ; do
    case $1 in
    -os)    OS=$2; shift;;
    -s)	    SPEED=$2; shift;;
    -*)	    echo "Usage: $0 [-os OS] [-s SPEED] [ttyname]"; exit 1;;
    *)	    TTY=$1;;
    esac
    shift
done

OUT=/tmp/addmodem$$		# temp file in which modem output is recorded
LOCKDIR=/var/spool/uucp	# UUCP locking directory
SPOOL=/var/spool/flexfax		# top of fax spooling tree
SVR4UULCKN=$SPOOL/bin/lockname	# SVR4 UUCP lock name construction program
ONDELAY=$SPOOL/bin/ondelay	# prgm to open devices blocking on carrier
CAT="cat -u"			# something to do unbuffered reads and writes

#
# Figure out which brand of echo we have and define
# prompt and printf shell functions accordingly.
# Note that we assume that if the System V-style
# echo is not present, then the BSD printf program
# is available.
#
if [ `echo foo\\\c`@ = "foo@" ]; then
    # System V-style echo supports \r
    # and \c which is all that we need
    prompt()
    {
       echo "$* \\c"
    }
    printf()
    {
       echo "$*\\c"
    }
elif [ "`echo -n foo`@" = "foo@" ]; then
    # BSD-style echo; use echo -n to get
    # a line without the trailing newline
    prompt()
    {
       echo -n "$* "
    }
else
    # something else; do without
    prompt()
    {
	echo "$*"
    }
fi
t=`printf hello` 2>/dev/null
if [ "$t" != "hello" ]; then
    echo "You don't seem to have a System V-style echo command"
    echo "or a BSD-style printf command.  I'm bailing out..."
    exit 1
fi

#
# If the killall program is not present on the system
# cobble together a shell function to emulate the
# functionality that we need.
#
(killall -l >/dev/null) 2>/dev/null || {
    killall()
    {
	# NB: ps ax should give an error on System V, so we try it first!
	pid="`ps ax 2>/dev/null | grep $2 | grep -v grep | awk '{print $1;}'`"
	test "$pid" ||
	    pid="`ps -e | grep $2 | grep -v grep | awk '{print $2;}'`"
	test "$pid" && kill $1 $pid; return
    }
}

while [ -z "$TTY" -o ! -c /dev/$TTY ]; do
    if [ "$TTY" != "" ]; then
	echo "/dev/$TTY is not a terminal device."
    fi
    prompt "Serial port that modem is connected to [$TTY]?"; read TTY
done

if [ ! -d $LOCKDIR ]; then
    prompt "Hmm, uucp lock files are not in \"$LOCKDIR\", where are they?"
    read x
    while [ ! -d $x ]; do
	prompt "Nope, \"$x\" is not a directory; try again:"
	read x
    done
    LOCKDIR=$x
fi

#
# Try to deduce if the tty devices are named in the SGI
# sense (ttyd<port>, ttym<port>, and ttyf<port>) or the
# way that everyone else seems to do it--tty<port>
#
# (I'm sure that someone will tell me there is another way as well.)
#
case "$OS" in
IRIX)
    PORT=`expr $TTY : 'tty.\(.*\)'`
    for x in f m d; do
	LOCKX="$LOCKX $LOCKDIR/LCK..tty$x${PORT}"
    done
    DEVS="/dev/ttyd${PORT} /dev/ttym${PORT} /dev/ttyf${PORT}"
    #
    # NB: we use ttyd* device names in the following
    # work so that we are not stopped by a need for DCD.
    #
    tdev=/dev/ttyd${PORT}
    ;;
BSDi|BSD/386|386bsd|386BSD)
    PORT=`expr $TTY : 'com\(.*\)'`
    LOCKX="$LOCKDIR/LCK..$TTY"
    DEVS=/dev/$TTY
    tdev=/dev/$TTY
    ;;
SunOS|Linux|ULTRIX|HP-UX)
    PORT=`expr $TTY : 'tty\(.*\)'`
    LOCKX="$LOCKDIR/LCK..$TTY"
    DEVS=/dev/$TTY
    tdev=/dev/$TTY
    ;;
svr4)
    PORT=`expr $TTY : 'term\/\(.*\)' \| $TTY`	# Usual
    PORT=`expr $PORT : 'cua\/\(.*\)' \| $PORT`	# Solaris
    PORT=`expr $PORT : 'tty\(.*\)' \| $PORT`	# Old-style
    DEVS=/dev/$TTY
    tdev=/dev/$TTY
    LOCKX="$LOCKDIR/`$SVR4UULCKN $DEVS`" || {
	echo "Sorry, I cannot determine the UUCP lock file name for $DEVS"
	exit 1
    }
    ;;
*)
    echo "Beware, I am guessing the tty naming conventions on your system:"
    PORT=`expr $TTY : 'tty\(.*\)'`;	echo "Serial port: $PORT"
    LOCKX="$LOCKDIR/LCK..$TTY";		echo "UUCP lock file: $LOCKX"
    DEVS=/dev/$TTY; tdev=/dev/$TTY;	echo "TTY device: $DEVS"
    ;;
esac
DEVID="`echo $TTY | tr '/' '_'`"
CONFIG=$CPATH.$DEVID

#
# Check that device is not currently being used.
#
for x in $LOCKX; do
    if [ -f $x ]; then
	echo "Sorry, the device is currently in use by another program."
	exit 1
    fi
done

#
# Lock the device for later use when deducing the modem type.
#
JUNK="$LOCKX $OUT"
trap "rm -f $JUNK; exit 1" 0 1 2 15

LOCKSTR=`expr "         $$" : '.*\(..........\)'`
# lock the device by all of its names
for x in $LOCKX; do
    echo "$LOCKSTR" > $x
done
# zap any gettys or other users
fuser -k $DEVS >/dev/null 2>&1 || {
    cat<<EOF
Hmm, there does not appear to be an fuser command on your machine.
This means that I am unable to insure that all processes using the
modem have been killed.  I will keep going, but beware that you may
have competition for the modem.
EOF
}

cat<<EOF

Now we are going to probe the tty port.  This takes a few seconds,
so be patient.  Note that if you do not have the modem cabled to
the port, or the modem is turned off, this may hang (just go and
cable up the modem or turn it on, or whatever).
EOF

if [ $OS = "SunOS" ]; then
    #
    # Sun systems have a command for manipulating software
    # carrier on a terminal line.  Set or reset carrier
    # according to the type of tty device being used.
    #
    case $TTY in
    tty*) ttysoftcar -y $TTY;;
    cua*) ttysoftcar -n $TTY;;
    esac
fi

if [ -x ${ONDELAY} ]; then
    onDev() {
	if [ "$1" = "-c" ]; then
	    shift; catpid=`${ONDELAY} $tdev sh -c "$* >$OUT" & echo $!`
	else
	    ${ONDELAY} $tdev sh -c "$*"
	fi
    }
else
cat<<'EOF'

The "ondelay" program to open the device without blocking is not
present.  We're going to try to continue without it; let's hope that
the serial port won't block waiting for carrier...
EOF
    onDev() {
	if [ "$1" = "-c" ]; then
	    shift; catpid=`sh <$tdev >$tdev -c "$* >$OUT" & echo $!`
	else
	    sh <$tdev >$tdev -c "$*"
	fi
    }
fi

#
# Send each command in SendString to the modem and collect
# the result in $OUT.  Read this very carefully.  It's got
# a lot of magic in it!
#
SendToModem()
{
    onDev stty 0				# reset the modem (hopefully)
    onDev -c "stty clocal && exec $CAT $tdev"	# start listening for output
    sleep 3					# let listener open dev first
    onDev stty -echo -icrnl -ixon -ixoff -isig clocal $SPEED; sleep 1
    # NB: merging \r & ATQ0 causes some modems problems
    printf "\r" >$tdev; sleep 1;		# force consistent state
    printf "ATQ0V1E1\r" >$tdev; sleep 1;	# enable echo and result codes
    for i in $*; do
	printf "$i\r" >$tdev; sleep 1;
    done
    kill -9 $catpid; catpid=
    # NB: [*&\\$] must have the "$" last for AIX (yech)
    pat=`echo "$i"|sed -e 's/[*&\\$]/\\\\&/g'`	# escape regex metacharacters
    RESPONSE=`tr -d '\015' < $OUT | \
	sed -n "/$pat/{n;s/ *$//;p;q;}" | sed 's/+F.*=//'`
}

RESULT="";
while [ -z "$RESULT" ]; do
    #
    # This goes in the background while we try to
    # reset the modem.  If something goes wrong, it'll
    # nag the user to check on the problem.
    #
    (trap 0 1 2 15;
     while true; do
	sleep 10;
	echo ""
	echo "Hmm, something seems to be hung, check your modem eh?"
     done)& nagpid=$!
    trap "rm -f \$JUNK; kill $nagpid \$catpid; exit 1" 0 1 2 15

    SendToModem "AT+FCLASS=?" 			# ask for class support

    kill $nagpid
    trap "rm -f $JUNK; test "$catpid" && kill $catpid; exit 1" 0 1 2 15
    sleep 1

    RESULT=`tr -d "\015" < $OUT | tail -1`
    if [ -z "$RESPONSE" ]; then
	echo ""
	echo "There was no response from the modem.  Perhaps the modem is"
	echo "turned off or the cable between the modem and host is not"
	echo "connected.  Please check the modem and hit a carriage return"
	prompt "when you are ready to try again:"
	read x
    fi
done

Try()
{
    onDev -c "stty clocal && exec $CAT $tdev"	# start listening for output
    onDev stty -echo -icrnl -ixon -ixoff -isig clocal $SPEED; sleep 1
    for i in $*; do
	printf "$i\r" >$tdev; sleep 1;
    done
    kill -9 $catpid; catpid=
    # NB: [*&\\$] must have the "$" last for AIX (yech)
    pat=`echo "$i"|sed -e 's/[*&\\$]/\\\\&/g'`	# escape regex metacharacters
    RESPONSE=`tr -d '\015' < $OUT | sed -n "/$pat/{n;s/ *$//;p;q;}"`
    RESULT=`tr -d "\015" < $OUT | tail -1`

    printf "$*	RESULT = \"$RESULT\"	RESPONSE = \"$RESPONSE\"\n"
}

TryClass2Commands()
{
    Try "AT+FCLASS=?";	Try "AT+FCLASS?"
    Try "AT+FCLASS=0";	Try "AT+FCLASS=1";	Try "AT+FCLASS=2"
    Try "AT+FCLASS?"
    Try "AT+FJUNK=?";	Try "AT+FJUNK?"
    Try "AT+FAA=?";	Try "AT+FAA?"
    Try "AT+FAXERR=?";	Try "AT+FAXERR?"
    Try "AT+FBADLIN=?";	Try "AT+FBADLIN?"
    Try "AT+FBADMUL=?";	Try "AT+FBADMUL?"
    Try "AT+FBOR=?";	Try "AT+FBOR?"
    Try "AT+FBUF=?";	Try "AT+FBUF?"
    Try "AT+FBUG=?";	Try "AT+FBUG?"
    Try "AT+FCIG=?";	Try "AT+FCIG?"
    Try "AT+FCQ=?";	Try "AT+FCQ?"
    Try "AT+FCR=?";	Try "AT+FCR?"
    Try "AT+FTBC=?";	Try "AT+FTBC?"
    Try "AT+FDCC=?";	Try "AT+FDCC?"
    Try "AT+FDCS=?";	Try "AT+FDCS?"
    Try "AT+FDIS=?";	Try "AT+FDIS?"
    Try "AT+FDT=?";	Try "AT+FDT?"
    Try "AT+FECM=?";	Try "AT+FECM?"
    Try "AT+FET=?";	Try "AT+FET?"
    Try "AT+FLID=?";	Try "AT+FLID?"
    Try "AT+FLNFC=?";	Try "AT+FLNFC?"
    Try "AT+FLPL=?";	Try "AT+FLPL?"
    Try "AT+FMDL?";	Try "AT+FMFR?"
    Try "AT+FMINSP=?";	Try "AT+FMINSP?"
    Try "AT+FPHCTO=?";	Try "AT+FPHCTO?"
    Try "AT+FPTS=?";	Try "AT+FPTS?"
    Try "AT+FRBC=?";	Try "AT+FRBC?"
    Try "AT+FREL=?";	Try "AT+FREL?"
    Try "AT+FREV?";
    Try "AT+FSPL=?";	Try "AT+FSPL?"
    Try "AT+FTBC=?";	Try "AT+FTBC?"
    Try "AT+FVRFC=?";	Try "AT+FVRFC?"
    Try "AT+FWDFC=?";	Try "AT+FWDFC?"
}

TryClass1Commands()
{
    Try "AT+FCLASS=?";	Try "AT+FCLASS?"
    Try "AT+FCLASS=0";	Try "AT+FCLASS=1"
    Try "AT+FCLASS?"
    Try "AT+FJUNK=?";	Try "AT+FJUNK?"
    Try "AT+FAA=?";	Try "AT+FAA?"
    Try "AT+FAE=?";	Try "AT+FAE?"
    Try "AT+FTH=?"
    Try "AT+FRH=?"
    Try "AT+FTM=?"
    Try "AT+FRM=?"
    Try "AT+FTS=?"
    Try "AT+FRS=?"
}

TryCommonCommands()
{
    for i in 0 1 2 3; do
	Try "ATI$i"
    done
}

echo ""
if [ "$RESULT" = "OK" ]; then
    # Looks like a Class 1 or 2 modem, get more information
    case "`echo $RESPONSE | sed -e 's/[()]//g'`" in
    2|0,2|2,*|0,2,*)
	echo "This looks like a Class 2 modem."
	echo ""
	TryCommonCommands
	echo ""; echo "Class 2 stuff..."; echo ""
	TryClass2Commands
	;;
    0,1,2|0,1,2,*)
	echo "This looks like a Class 1+2 modem."
	echo ""
	TryCommonCommands
	echo ""; echo "Class 1 stuff..."; echo ""
	TryClass1Commands
	echo ""; echo "Class 2 stuff..."; echo ""
	TryClass2Commands
	;;
    1|0,1|0,1,*)
	echo "This looks like a Class 1 modem."
	echo ""
	TryCommonCommands
	echo ""; echo "Class 1 stuff..."; echo ""
	TryClass1Commands
	exit
	;;
    *)	echo "The result of the AT+FCLASS=? command was:"
	echo ""
	cat $OUT
	echo ""
	echo "I don't figure that it's worthwhile to continue..."
	exit
	;;
    esac
else
    echo "This not a Class 1 or Class 2 modem,"
    exit
fi
