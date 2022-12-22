#! /bin/bash
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/etc/faxaddmodem.sh,v 1.1.1.1 1994/01/14 23:09:40 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
# Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
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
# faxaddmodem [tty]
#
# This script interactively configures a FlexFAX server
# from keyboard input on a standard terminal.  There may
# be some system dependencies in here; hard to say with
# this mountain of shell code!
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

#
# Deduce the effective user id:
#   1. POSIX-style, the id program
#   2. the old whoami program
#   3. last gasp, check if we have write permission on /dev
#
euid=`id|sed -e 's/.*uid=[0-9]*(\([^)]*\)).*/\1/'`
test -z "$euid" && euid=`whoami 2>/dev/null`
test -z "$euid" -a -w /dev && euid=root
if [ "$euid" != "root" ]; then
    echo "Sorry, but you must run this script as the super-user!"
    exit 1
fi

SPOOL=/var/spool/flexfax		# top of fax spooling tree
CPATH=$SPOOL/etc/config		# prefix of configuration file
LOCKDIR=/var/spool/uucp	# UUCP locking directory
OUT=/tmp/addmodem$$		# temp file in which modem output is recorded
SVR4UULCKN=$SPOOL/bin/lockname	# SVR4 UUCP lock name construction program
ONDELAY=$SPOOL/bin/ondelay	# prgm to open devices blocking on carrier
CAT="cat -u"			# something to do unbuffered reads and writes
FAX=fax				# identity of the fax user
SERVICES=/etc/services		# location of services database
INETDCONF=/usr/etc/inetd.conf	# default location of inetd configuration file
ALIASES=/usr/lib/aliases	# default location of mail aliases database file
SGIPASSWD=/etc/passwd.sgi	# for hiding fax user from pandor on SGI's
PASSWD=/etc/passwd		# where to go for password entries
PROTOUID=uucp			# user who's uid we use for FAX user
defPROTOUID=3			# use this uid if PROTOUID doesn't exist
GROUP=/etc/group		# where to go for group entries
PROTOGID=dialer			# group who's gid we use for FAX user
defPROTOGID=10			# use this gid if PROTOGID doesn't exist
SERVERDIR=/usr/libexec		# directory where servers are located
QUIT=/usr/contrib/bin/faxquit	# cmd to terminate server
MODEMCONFIG=$SPOOL/etc		# location of modem configuration files

#
# Deal with known alternate locations for system files.
#
PickFile()
{
    for i do
	test -f $i && { echo $i; return; }
    done
    echo $1
}
INETDCONF=`PickFile	$INETDCONF /etc/inetd.conf /etc/inet/inetd.conf`
ALIASES=`PickFile	$ALIASES   /etc/aliases`
SERVICES=`PickFile	$SERVICES  /etc/inet/services`
test -f /etc/master.passwd			&& PASSWD=/etc/master.passwd

#
# Setup the password file manipulation functions according
# to whether we have System-V style support through the
# passmgmt program, or BSD style support through the chpass
# program.  If neither are found, we setup functions that
# will cause us to abort if we need to munge the password file.
#
if [ -f /bin/passmgmt ]; then
    addPasswd()
    {
	passmgmt -o -a -c 'Facsimile Agent' -h $4 -u $2 -g $3 $1
    }
    deletePasswd()
    {
	passmgmt -d $1
    }
    modifyPasswd()
    {
	passmgmt -m -h $4 -u $2 -o -g $3 $1
    }
    lockPasswd()
    {
	passwd -l $1
    }
elif [ -f /usr/bin/chpass ]; then
    addPasswd()
    {
	chpass -a "$1:*:$2:$3::0:0:Facsimile Agent:$4:"
    }
    modifyPasswd()
    {
	chpass -a "$1:*:$2:$3::0:0:Facsimile Agent:$4:"
    }
    lockPasswd()
    {
	return 0				# entries are always locked
    }
elif [ -f /etc/useradd -o -f /usr/sbin/useradd ]; then
    addPasswd()
    {
	useradd -c 'Facsimile Agent' -d $4 -u $2 -o -g $3 $1
    }
    deletePasswd()
    {
	userdel $1
    }
    modifyPasswd()
    {
	usermod -m -d $4 -u $2 -o -g $3 $1
    }
    lockPasswd()
    {
	passwd -l $1
    }
else
    addPasswd()
    {
	echo "Help, I don't know how to add a passwd entry!"; exit 1
    }
    modifyPasswd()
    {
	echo "Help, I don't know how to modify a passwd entry!"; exit 1
    }
fi

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

#
# Prompt the user for a string that can not be null.
#
promptForNonNullStringParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	else
	    x="$1"
	fi
    done
    param="$x"
}

#
# Prompt the user for a numeric value.
#
promptForNumericParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    match=`expr "$x" : "\([0-9]*\)"`
	    if [ "$match" != "$x" ]; then
		echo ""
		echo "This must be entirely numeric; please correct it."
		echo ""
		x="";
	    fi
	else
	    x="$1"
	fi
    done
    param="$x"
}

#
# Prompt the user for a C-style numeric value.
#
promptForCStyleNumericParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space and C-style 0x prefix
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    match=`expr "$x" : "\([0-9]*\)" \| "$x" : "\(0x[0-9a-fA-F]*\)"`
	    if [ "$match" != "$x" ]; then
		echo ""
		echo "This must be entirely numeric; please correct it."
		echo ""
		x="";
	    fi
	else
	    x="$1"
	fi
    done
    param="$x"
}

#
# Start of system-related setup checking.  All this stuff
# should be done outside this script, but we check here
# so that it's sure to be done before starting up a fax
# server process.
# 
echo "Verifying that your system is setup properly for fax service..."

faxUID=`grep "^$PROTOUID:" $PASSWD | cut -d: -f3`
if [ -z "$faxUID" ]; then faxUID=$defPROTOUID; fi
faxGID=`grep "^$PROTOGID:" $GROUP | cut -d: -f3`
if [ -z "$faxGID" ]; then faxGID=$defPROTOGID; fi

#
# Change the password file entry for the fax user.
#
fixupFaxUser()
{
    emsg1=`modifyPasswd $FAX $faxUID $faxGID $SPOOL 2>&1`
    case $? in
    0)	echo "Done, the \"$FAX\" user should now have the right user id.";;
    *) cat <<-EOF
	Something went wrong; the command failed with:

	"$emsg1"

	The fax server will not work correctly until the proper uid
	is setup for it in the password file.  Please fix this problem
	and then rerun faxaddmodem.
	EOF
	exit 1
	;;
    esac
}

#
# Add a fax user to the password file and lock the
# entry so that noone can login as the user.
#
addFaxUser()
{
    emsg1=`addPasswd $FAX $faxUID $faxGID $SPOOL 2>&1`
    case $? in
    0)  emsg2=`lockPasswd $FAX 2>&1`
	case $? in
	0) echo "Added user \"$FAX\" to $PASSWD.";;
	*) emsg3=`deletePasswd $FAX 2>&1`
	   case $? in
	   0|9) cat <<-EOF
		Failed to add user "$FAX" to $PASSWD because the
		attempt to lock the password failed with:

		"$emsg2"

		Please fix this problem and rerun this script."
		EOF
		exit 1
		;;
	   *)   cat <<-EOF
		You will have to manually edit $PASSWD because
		after successfully adding the new user "$FAX", the
		attempt to lock its password failed with:

		"$emsg2"

		and the attempt to delete the insecure passwd entry failed with:

		"$emsg3"

		To close this security hole, you should add a password
		to the "$FAX" entry in the file $PASSWD, or lock this
		entry with an invalid password.
		EOF
		;;
	    esac
	    exit 1;;
	esac;;
    9)  # fax was already in $PASSWD, but not found with grep
	;;
    *)  cat <<-EOF
	There was a problem adding user "$FAX" to $PASSWD;
	the command failed with:

	"$emsg1"

	The fax server will not work until you have corrected this problem.
	EOF
	exit 1;;
    esac
}

x=`grep "^$FAX" $PASSWD | cut -d: -f3`
if [ -z "$x" ]; then
    echo ""
    echo "You do not appear to have a "$FAX" user in the password file."
    prompt "The fax software needs this to work properly, add it [yes]?"
    read x
    if [ -z "$x" -o "$x" = "y" -o "$x" = "yes" ]; then
	addFaxUser
    fi
elif [ "$x" != "$faxUID" ]; then
    echo ""
    echo "It looks like you have a \"$FAX\" user in the password file,"
    echo "but with a uid different than the uid for uucp.  You probably"
    echo "have old fax software installed.  In order for this software"
    echo "to work properly, the fax user uid must be the same as uucp."
    prompt "Is it ok to change the password entry for \"$FAX\" [yes]?"
    read x
    if [ -z "$x" -o "$x" = "y" -o "$x" = "yes" ]; then
	fixupFaxUser
    fi
fi
#
# This should only have an effect on an SGI system...
# hide the fax user from pandora & co.
#
if [ "$OS" = "IRIX" -a -f $SGIPASSWD ]; then
    x=$FAX:noshow
    grep $x $SGIPASSWD >/dev/null 2>&1 ||
    (echo ""; echo "Adding fax user to \"$SGIPASSWD\"."; echo $x >>$SGIPASSWD)
fi

#
# Make sure a service entry is present.
#
hasYP=`ypcat services 2>/dev/null | tail -1` 2>/dev/null
x=
if [ "$hasYP" ]; then
    x=`ypcat services 2>/dev/null | egrep '^(flex)?fax[ 	]'` 2>/dev/null
    if [ -z "$x" ]; then
	echo ""
	echo "There does not appear to be an entry for the fax service in the YP database;"
	echo "the software will not work properly without one.  Contact your administrator"
	echo "to get the folllowing entry added:"
	echo ""
	echo "fax	4557/tcp		# FAX transmission service"
	echo ""
	exit 1
    fi
else
    x=`egrep '^(flex)?fax[ 	]' $SERVICES 2>/dev/null` 2>/dev/null
    if [ -z "$x" ]; then
	echo ""
	echo "There does not appear to be an entry for the fax service in the $SERVICES file;"
	prompt "should an entry be added to $SERVICES [yes]?"
	read x
	if [ "$x" = "" -o "$x" = "y" -o "$x" = "yes" ]; then
	    echo "fax		4557/tcp		# FAX transmission service" >>$SERVICES
	fi
    fi
fi

#
# Check that inetd is setup to provide service.
#
E="fax	stream	tcp	nowait	$FAX	$SERVERDIR/faxd.recv	faxd.recv"
test -f $INETDCONF && {
    egrep "^(flex)?fax[ 	]*stream[ 	]*tcp" $INETDCONF >/dev/null 2>&1 ||
    (echo ""
     echo "There is no entry for the fax service in \"$INETDCONF\";"
     prompt "should one be added [yes]?"
     read x
     if [ "$x" = "" -o "$x" = "y" -o "$x" = "yes" ]; then
	echo "$E" >>$INETDCONF;
	if killall -HUP inetd 2>/dev/null; then
	    echo "Poked inetd so that it re-reads the configuration file."
	else
	    echo "Beware, you may need to send a HUP signal to inetd."
	fi
     fi
    )
}

#
# Check for a FaxMaster entry for sending mail.
#
x=`ypcat aliases 2>/dev/null | grep -i '^faxmaster'` 2>/dev/null
if [ -z "$x" -a -f $ALIASES ]; then
    x=`grep -i '^faxmaster' $ALIASES`
fi
if [ -z "$x" ]; then
    echo ""
    echo "There does not appear to be an entry for the FaxMaster either in"
    echo "the yellow pages database or in the $ALIASES file;"
    prompt "should an entry be added to $ALIASES [yes]?"
    read x
    if [ "$x" = "" -o "$x" = "y" -o "$x" = "yes" ]; then
	promptForNonNullStringParameter "${USER:-root}" \
	   "Users to receive fax-related mail"
	(echo "# alias for notification messages from FlexFAX servers";
	 echo "FaxMaster: $param") >>$ALIASES
	if newaliases 2>/dev/null; then
	    echo "Rebuilt $ALIASES database."
	else
	    echo "Can not find newaliases to rebuild $ALIASES;"
	    echo "you will have to do it yourself."
	fi
    fi
fi


echo ""
echo "Done verifying system setup."
echo ""
# End of system-related setup checking

while [ -z "$TTY" -o ! -c /dev/$TTY ]; do
    if [ "$TTY" != "" ]; then
	echo "/dev/$TTY is not a terminal device."
    fi
    prompt "Serial port that modem is connected to [$TTY]?"; read TTY
done

JUNK="$OUT"
trap "rm -f \$JUNK; exit 1" 0 1 2 15

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
# Look for conflicting configuration stuff.
#
OLDCONFIG=""

checkPort()
{
    devID="`echo $1 | tr '/' '_'`"
    if [ -f $CPATH.$devID -a -p $SPOOL/FIFO.$devID ]; then
	echo "There appears to be a modem already setup on $devID,"
	prompt "is this to be replaced [yes]?"
	read x;
	if [ "$x" = "n" -o "$x" = "no" ]; then
	    echo "Sorry, but you can not configure multiple servers on"
	    echo "the same serial port."
	    exit 1

	fi
	echo "Removing old FIFO special file $SPOOL/FIFO.$devID."
	$QUIT $devID >/dev/null 2>&1; rm -f $SPOOL/FIFO.$devID
	OLDCONFIG=$CPATH.$devID
    fi
}

if [ "$OS" = "IRIX" ]; then
    case $TTY in
    ttym${PORT}) checkPort ttyd${PORT}; checkPort ttyf${PORT};;
    ttyf${PORT}) checkPort ttym${PORT}; checkPort ttyd${PORT};;
    ttyd${PORT}) checkPort ttym${PORT}; checkPort ttyf${PORT};;
    esac
fi
$QUIT $DEVID >/dev/null 2>&1; sleep 1		# shutdown existing server

#
# Lock the device for later use when deducing the modem type.
#
JUNK="$JUNK $LOCKX"

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

Ok, time to setup a configuration file for the modem.  The manual
page config(4F) may be useful during this process.  Also be aware
that at any time you can safely interrupt this procedure.

EOF

FAXNumber="" AreaCode="" CountryCode="1"
LongDistancePrefix="1"
InternationalPrefix="011"
DialStringRules="etc/dialrules"
ServerTracing="1"
SessionTracing="11"
RecvFileMode="0600"
LogFileMode="0600"
DeviceMode="0600"
RingsBeforeAnswer="1"
SpeakerVolume="off"
GettyArgs=""
QualifyTSI=""
# ancient stuff...
DialingPrefix=""

getParameter()
{
    param=`grep "^$1:" $2 | sed -e 's/[ 	]*#.*//' -e 's/.*:[ 	]*//'`
}

saveServerConfig()
{
    protoFAXNumber="$FAXNumber"
    protoAreaCode="$AreaCode"
    protoCountryCode="$CountryCode"
    protoLongDistancePrefix="$LongDistancePrefix"
    protoInternationalPrefix="$InternationalPrefix"
    protoDialStringRules="$DialStringRules"
    protoServerTracing="$ServerTracing"
    protoSessionTracing="$SessionTracing"
    protoRecvFileMode="$RecvFileMode"
    protoLogFileMode="$LogFileMode"
    protoDeviceMode="$DeviceMode"
    protoRingsBeforeAnswer="$RingsBeforeAnswer"
    protoSpeakerVolume="$SpeakerVolume"
    protoGettyArgs="$GettyArgs"
    protoQualifyTSI="$QualityTSI"
}

#
# Get the default server config values
# from the prototype configuration file.
#
getServerConfig()
{
    file=$1
    getParameter FAXNumber $file;	    FAXNumber="$param"
    getParameter DialStringRules $file;	    DialStringRules="$param"
    getParameter CountryCode $file;	    CountryCode="${param:-1}"
    getParameter AreaCode $file;	    AreaCode="$param"
    getParameter LongDistancePrefix $file;  LongDistancePrefix="${param:-1}"
    getParameter InternationalPrefix $file; InternationalPrefix="${param:-011}"
    getParameter ServerTracing $file;	    ServerTracing="${param:-1}"
    getParameter SessionTracing $file;	    SessionTracing="${param:-11}"
    getParameter RecvFileMode $file;	    RecvFileMode="${param:-0600}"
    getParameter LogFileMode $file;	    LogFileMode="${param:-0600}"
    getParameter DeviceMode $file;	    DeviceMode="${param:-0600}"
    getParameter RingsBeforeAnswer $file;   RingsBeforeAnswer="${param:-1}"
    getParameter SpeakerVolume $file;	    SpeakerVolume="${param:-off}"
    # convert old numeric style
    case "SpeakerVolume" in
    0)	SpeakerVolume="off";;
    1)	SpeakerVolume="low";;
    2)	SpeakerVolume="quiet";;
    3)	SpeakerVolume="medium";;
    4)	SpeakerVolume="high";;
    esac
    getParameter UseDialPrefix $file;
    case "$param" in
    [yY]es|[oO]n)
	getParameter DialingPrefix $file;   DialingPrefix="$param"
	;;
    esac
    getParameter GettyAllowed $file;
    case "$GettyAllowed" in
    [yY]es|[oO]n)
	getParameter GettySpeed $file;	    GettyArgs="$param"
	;;
    esac
    getParameter QualifyTSI $file;
    case "$QualifyTSI" in
    [yY]es|[oO]n)
	QualifyTSI="etc/tsi"
	;;
    esac

    saveServerConfig
}

#
# Append a sed command to the ServerCmds if we need to
# alter the existing configuration parameter.  Note that
# we handle the case where there are embedded blanks or
# tabs by enclosing the string in quotes.
#
addServerSedCmd()
{
    test "$1" != "$2" && {
	if [ `expr "$2" : "[^\"].*[ ]"` = 0 ]; then
	    ServerCmds="$ServerCmds -e '/$3:/s/$1/$2/'"
	else
	    ServerCmds="$ServerCmds -e '/$3:/s/$1/\"$2\"/'"
	fi
    }
}

#
# Setup the sed commands for crafting the configuration file:
#
makeSedServerCommands()
{
    ServerCmds=""
    addServerSedCmd "$protoFAXNumber"		"$FAXNumber"	FAXNumber
    addServerSedCmd "$protoAreaCode"		"$AreaCode"	AreaCode
    addServerSedCmd "$protoCountryCode"		"$CountryCode"	CountryCode
    addServerSedCmd "$protoLongDistancePrefix"	"$LongDistancePrefix" \
	LongDistancePrefix
    addServerSedCmd "$protoInternationalPrefix"	"$InternationalPrefix" \
	InternationalPrefix
    addServerSedCmd "$protoDialStringRules"	"$DialStringRules" \
	DialStringRules
    addServerSedCmd "$protoServerTracing"	"$ServerTracing" \
	ServerTracing
    addServerSedCmd "$protoSessionTracing"	"$SessionTracing" \
	SessionTracing
    addServerSedCmd "$protoRecvFileMode"	"$RecvFileMode"	RecvFileMode
    addServerSedCmd "$protoLogFileMode"		"$LogFileMode"	LogFileMode
    addServerSedCmd "$protoDeviceMode"		"$DeviceMode"	DeviceMode
    addServerSedCmd "$protoRingsBeforeAnswer"	"$RingsBeforeAnswer" \
	RingsBeforeAnswer
    addServerSedCmd "$protoSpeakerVolume"	"$SpeakerVolume" \
	SpeakerVolume
    addServerSedCmd "$protoGettyArgs"		"$GettyArgs"	GettyArgs
    addServerSedCmd "$protoQualifyTSI"		"$QualityTSI"	QualityTSI
}

#
# Prompt the user for volume setting.
#
promptForSpeakerVolume()
{
    x=""
    while [ -z "$x" ]; do
	prompt "Modem speaker volume [$SpeakerVolume]?"; read x
	if [ "$x" != "" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    case "$x" in
	    [oO]*)	x="off";;
	    [lL]*)	x="low";;
	    [qQ]*)	x="quiet";;
	    [mM]*)	x="medium";;
	    [hH]*)	x="high";;
	    *)
cat <<EOF

"$x" is not a valid speaker volume setting; use one
of: "off", "low", "quiet", "medium", and "high".
EOF
		x="";;
	    esac
	else
	    x="$SpeakerVolume"
	fi
    done
    SpeakerVolume="$x"
}

#
# Verify that the fax number, area code, and country
# code jibe.  Perhaps this is too specific to the USA?
#
checkFaxNumber()
{
    pat="+$CountryCode[-. ]*$AreaCode[-. ]*[0-9][- .0-9]*"
    match=`expr "$FAXNumber" : "\($pat\)"`
    if [ "$match" != "$FAXNumber" ]; then
	cat<<EOF

Your facsimile phone number ($FAXNumber) does not agree with your
country code ($CountryCode) or area code ($AreaCode).  The number
should be a fully qualified international dialing number of the form:

    +$CountryCode $AreaCode <local phone number>

Spaces, hyphens, and periods can be included for legibility.  For example,

    +$CountryCode.$AreaCode.555.1212

is a possible phone number (using your country and area codes).
EOF
	ok="no";
    fi
}

#
# Verify that a number is octal and if not, add a prefixing "0".
#
checkOctalNumber()
{
    param=$1
    if [ "`expr "$param" : '\(.\)'`" != "0" ]; then
	param="0${param}"
	return 0
    else
	return 1
    fi
}

#
# Verify that the dial string rules file exists.
#
checkDialStringRules()
{
    if [ ! -f $SPOOL/$DialStringRules ]; then
	cat<<EOF

Warning, the dial string rules file,

    $SPOOL/$DialStringRules

does not exist, or is not a plain file.  The rules file
must reside in the $SPOOL directory tree.
EOF
	ok="no";
    fi
}

#
# Print the current server configuration parameters.
#
printServerConfig()
{
    cat<<EOF

The server configuration parameters are:

FAXNumber:		$FAXNumber
AreaCode:		$AreaCode
CountryCode:		$CountryCode
LongDistancePrefix:	$LongDistancePrefix
InternationalPrefix:	$InternationalPrefix
DialStringRules:	$DialStringRules
ServerTracing:		$ServerTracing
SessionTracing:		$SessionTracing
RecvFileMode:		$RecvFileMode
LogFileMode:		$LogFileMode
DeviceMode:		$DeviceMode
RingsBeforeAnswer:	$RingsBeforeAnswer
SpeakerVolume:		$SpeakerVolume

EOF
}

if [ -f $CONFIG -o -z "$OLDCONFIG" ]; then
    OLDCONFIG=$CONFIG
fi
if [ -f $OLDCONFIG ]; then
    echo "Hey, there is an existing config file "$OLDCONFIG"..."
    getServerConfig $OLDCONFIG
    ok="skip"				# skip prompting first time through
else
    echo "No existing configuration, let's do this from scratch."
    echo ""
    ok="prompt"				# prompt for parameters
fi

#
# Prompt user for server-related configuration parameters
# and do consistency checking on what we get.
#
while [ "$ok" != "" -a "$ok" != "y" -a "$ok" != "yes" ]; do
    if [ "$ok" != "skip" ]; then
	promptForNonNullStringParameter "$FAXNumber" \
	    "Phone number of fax modem"; FAXNumber=$param;
	promptForNumericParameter "$AreaCode" \
	    "Area code"; AreaCode=$param;
	promptForNumericParameter "$CountryCode" \
	    "Country code"; CountryCode=$param;
	promptForNumericParameter "$LongDistancePrefix" \
	    "Long distance dialing prefix"; LongDistancePrefix=$param;
	promptForNumericParameter "$InternationalPrefix" \
	    "International dialing prefix"; InternationalPrefix=$param;
	promptForNonNullStringParameter "$DialStringRules" \
	    "Dial string rules file"; DialStringRules=$param;
	promptForCStyleNumericParameter "$ServerTracing" \
	    "Tracing during normal server operation"; ServerTracing=$param;
	promptForCStyleNumericParameter "$SessionTracing" \
	    "Tracing during send and receive sessions"; SessionTracing=$param;
	promptForNumericParameter "$RecvFileMode" \
	    "Protection mode for received facsimile"; RecvFileMode=$param;
	promptForNumericParameter "$LogFileMode" \
	    "Protection mode for session logs"; LogFileMode=$param;
	promptForNumericParameter "$DeviceMode" \
	    "Protection mode for $TTY"; DeviceMode=$param;
	promptForNumericParameter "$RingsBeforeAnswer" \
	    "Rings to wait before answering"; RingsBeforeAnswer=$param;
	promptForSpeakerVolume;
    fi
    checkOctalNumber $RecvFileMode &&	RecvFileMode=$param
    checkOctalNumber $LogFileMode &&	LogFileMode=$param
    checkOctalNumber $DeviceMode &&	DeviceMode=$param
    checkDialStringRules;
    printServerConfig; prompt "Are these ok [yes]?"; read ok
    checkFaxNumber;
done
makeSedServerCommands

#
# We've got all the server-related parameters, now for the modem ones.
#

cat<<EOF

Now we are going to probe the tty port to figure out the type
of modem that is attached.  This takes a few seconds, so be patient.
Note that if you do not have the modem cabled to the port, or the
modem is turned off, this may hang (just go and cable up the modem
or turn it on, or whatever).
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
    trap "rm -f \$JUNK; test "$catpid" && kill $catpid; exit 1" 0 1 2 15
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

giveup()
{
	cat<<EOF

We were unable to deduce what type of modem you have.  This means that
it did not respond as a Class 1 or Class 2 modem should and it did not
appear as though it was an Abaton 24/96 (as made by Everex).  If you
believe that your modem conforms to the Class 1 or Class 2 interface
specification, or is an Abaton modem, then check that the modem is
operating properly and that you can communicate with the modem from
the host.  If your modem is not one of the above types of modems, then
this software does not support it and you will need to write a driver
that supports it.

EOF
	exit 1
}
ModemType="" Manufacturer="" Model="" ProtoType="config.skel"

echo ""
if [ "$RESULT" = "OK" ]; then
    # Looks like a Class 1 or 2 modem, get more information
    case "`echo $RESPONSE | sed -e 's/[()]//g'`" in
    2|0,2|0,1,2|2,*|0,2,*|0,1,2,*)
	ModemType=Class2
	echo "Hmm, this looks like a Class 2 modem."
	#
	# Use AT+FMDL? to get the model and AT+FMFR? to get
	# manuafacturer's identity and then compare them against
	# the set of known values in the Class 2 config files.
	# Note that we do this with a tricky bit of shell
	# hacking--generating a case statement that we
	# then eval with the result being the setup of the
	# ProtoType shell variable.
	#
	SendToModem "AT+FCLASS=2" "AT+FMFR?"
	Manufacturer=$RESPONSE
	echo "Modem manufacturer is \"$Manufacturer\"."

	SendToModem "AT+FCLASS=2" "AT+FMDL?"
	Model=$RESPONSE
	echo "Modem model is \"$Model\"."

	eval `(cd $MODEMCONFIG; \
	    grep 'CONFIG:[ 	]*CLASS2' config.* |\
	    awk -F: '
		BEGIN { print "case \"$Manufacturer-$Model\" in" }
		{ print $4 ") ProtoType=" $1 ";;" }
		END { print "*) ProtoType=config.class2;;"; print "esac" }
	    ')`
	;;
    1|0,1|0,1,*)
	ModemType=Class1 Manufacturer=Unknown Model=Unknown
	echo "Hmm, this looks like a Class 1 modem."
	#
	# Use ATI0 to get the product code and then compare
	# it against the set of known values in the config
	# files.  Note that we do this with a tricky bit of
	# shell hacking--generating a case statement that we
	# then eval with the result being the setup of the
	# Manufacturer, Model, and ProtoType shell variables.
	#
	SendToModem "ATI0"
	echo "Product code is \"$RESPONSE\"."

	eval `(cd $MODEMCONFIG; \
	    grep 'CONFIG:[ 	]*CLASS1' config.* |\
	    awk -F: '
		BEGIN { print "case \"$RESPONSE\" in" }
		{ print $4 ") " $5 " ProtoType=" $1 ";;" }
		END { print "*) ProtoType=config.class1;;"; print "esac" }
	    ')`
	echo "Modem manufacturer is \"$Manufacturer\"."
	echo "Modem model is \"$Model\"."
	;;
    *)	echo "The result of the AT+FCLASS=? command was:"
	echo ""
	cat $OUT
	giveup
	;;
    esac
else
    echo "This not a Class 1 or Class 2 modem,"
    echo "lets check some other possibilities..."

    SendToModem "ATI4?"
    RESULT=`tr -d "\015" < $OUT | tail -1`
    echo ""
    if [ "$RESULT" != "OK" ]; then
	echo "The modem does not seem to understand the ATI4? query."
	echo "Sorry, but I can't figure out how to configure this one."
	exit 1
    fi
    case "$RESPONSE" in
    EV968*|EV958*)
	ModemType="Abaton";
	echo "Hmm, this looks like an Abaton 24/96 (specifically a $RESPONSE)."
	;;
    *)	echo "The result of the ATI4? query was:"
	echo ""
	cat $OUT
	giveup;
	;;
    esac
fi

echo ""
#
# Given a modem type, manufacturer and model, select
# a prototype configuration file to work from.
#
case $ModemType in
Class1|Class2)
    echo "Using prototype configuration file $ProtoType..."
    ;;
Abaton)
    echo "Using prototype Abaton 24/96 configuration file..."
    ProtoType=config.abaton
    ;;
esac

proto=$MODEMCONFIG/$ProtoType
if [ ! -f $proto ]; then
   cat<<EOF
Uh oh, I can't find the prototype file

"$proto"

EOF
    if [ "$ProtoType" != "config.skel" ]; then
        prompt "Do you want to continue using the skeletal configuration file [yes]?"
        read x
        if [ "$x" = "n" -o "$x" = "no" ]; then
	   exit 1
	fi
	ProtoType="config.skel";
	proto=$MODEMCONFIG/$ProtoType;
	if [ ! -f $proto ]; then
	    cat<<EOF
Sigh, the skeletal configuration file is not available either.  There
is not anything that I can do without some kind of prototype config
file; I'm bailing out..."
EOF
	    exit 1
	fi
    else
	echo "There is nothing more that I can do; I'm bailing out..."
	exit 1
    fi
fi

#
# Prompt the user for an AT-style command.
#
promptForATCmdParameter()
{
    prompt "$2 [$1]?"; read x
    if [ "$x" ]; then
	# strip leading and trailing white space,
	# quote marks, and any prefacing AT
	x=`echo "$x" | \
	    sed -e 's/^[ 	]*//' -e 's/[ 	]*$//' -e 's/\"//g' | \
	    sed -e 's/^[aA][tT]//'`
    else
	x="$1"
    fi
    param="$x"
}

#
# Prompt the user for a boolean value.
#
promptForBooleanParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    case "$x" in
	    n|no|off)	x="no";;
	    y|yes|on)	x="yes";;
	    *)
cat <<EOF

"$x" is not a valid boolean parameter setting;
use one of: "yes", "on", "no", or "off".
EOF
		x="";;
	    esac
	else
	    x="$1"
	fi
    done
    param="$x"
}

#
# Prompt the user for a bit order.
#
promptForBitOrderParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    case "$x" in
	    [lL]*)	x="LSB2MSB";;
	    [mM]*)	x="MSB2LSB";;
	    *)
cat <<EOF

"$x" is not a valid bit order parameter setting; use
one of: "lsb2msb", "LSB2MSB", "msb2lsb", or "MSB2LSB".
EOF
		x="";;
	    esac
	else
	    x="$1"
	fi
    done
    param="$x"
}

#
# Prompt the user for a flow control scheme.
#
promptForFlowControlParameter()
{
    x=""
    while [ -z "$x" ]; do
	prompt "$2 [$1]?"; read x
	if [ "$x" ]; then
	    # strip leading and trailing white space
	    x=`echo "$x" | sed -e 's/^[ 	]*//' -e 's/[ 	]*$//'`
	    case "$x" in
	    xon*|XON*)	x="xonxoff";;
	    rts*|RTS*)	x="rtscts";;
	    *)
cat <<EOF

"$x" is not a valid bit order parameter setting; use
one of: "xonxoff", "XONXOFF", "rtscts", or "RTSCTS".
EOF
		x="";;
	    esac
	else
	    x="$1"
	fi
    done
    param="$x"
}

saveModemProtoParameters()
{
    protoMaxRate="$ModemRate"
    protoFlowControl="$ModemFlowControl"
    protoFlowControlCmd="$ModemFlowControlCmd"
    protoSetupDTRCmd="$ModemSetupDTRCmd"
    protoSetupDCDCmd="$ModemSetupDCDCmd"
    protoDialCmd="$ModemDialCmd"
    protoAnswerCmd="$ModemAnswerCmd"
    protoNoAutoAnswerCmd="$ModemNoAutoAnswerCmd"
    protoSetVolumeCmd="$ModemSetVolumeCmd"
    protoEchoOffCmd="$ModemEchoOffCmd"
    protoVerboseResultsCmd="$ModemVerboseResultsCmd"
    protoResultCodesCmd="$ModemResultCodesCmd"
    protoOnHookCmd="$ModemOnHookCmd"
    protoSoftResetCmd="$ModemSoftResetCmd"
    protoWaitTimeCmd="$ModemWaitTimeCmd"
    protoCommaPauseTimeCmd="$ModemCommaPauseTimeCmd"
    protoSendFillOrder="$ModemSendFillOrder"
    protoRecvFillOrder="$ModemRecvFillOrder"
    protoResetCmds="$ModemResetCmds"
    protoBORCmd="$Class2BORCmd"
    protoCQCmd="$Class2CQCmd"
    protoRELCmd="$Class2RELCmd"
    protoAbortCmd="$Class2AbortCmd"
    protoRecvDataTrigger="$Class2RecvDataTrigger"
    protoXmitWaitForXON="$Class2XmitWaitForXON"
}

#
# Get the default modem parameter values
# from the prototype configuration file.
#
getModemProtoParameters()
{
    getParameter ModemRate $1;			ModemRate=$param
    getParameter ModemFlowControl $1;		ModemFlowControl=$param
    getParameter ModemFlowControlCmd $1;	ModemFlowControlCmd=$param
    getParameter ModemSetupDTRCmd $1;		ModemSetupDTRCmd=$param
    getParameter ModemSetupDCDCmd $1;		ModemSetupDCDCmd=$param
    getParameter ModemDialCmd $1;		ModemDialCmd=$param
    getParameter ModemAnswerCmd $1;		ModemAnswerCmd=$param
    getParameter ModemNoAutoAnswerCmd $1;	ModemNoAutoAnswerCmd=$param
    getParameter ModemSetVolumeCmd $1;		ModemSetVolumeCmd=$param
    getParameter ModemEchoOffCmd $1;		ModemEchoOffCmd=$param
    getParameter ModemVerboseResultsCmd $1;	ModemVerboseResultsCmd=$param
    getParameter ModemResultCodesCmd $1;	ModemResultCodesCmd=$param
    getParameter ModemOnHookCmd $1;		ModemOnHookCmd=$param
    getParameter ModemSoftResetCmd $1;		ModemSoftResetCmd=$param
    getParameter ModemWaitTimeCmd $1;		ModemWaitTimeCmd=$param
    getParameter ModemCommaPauseTimeCmd $1;	ModemCommaPauseTimeCmd=$param
    getParameter ModemSendFillOrder $1;		ModemSendFillOrder=$param
    getParameter ModemRecvFillOrder $1;		ModemRecvFillOrder=$param
    getParameter ModemResetCmds $1;		ModemResetCmds=$param
    getParameter Class2BORCmd $1;		Class2BORCmd=$param
    getParameter Class2CQCmd $1;		Class2CQCmd=$param
    getParameter Class2RELCmd $1;		Class2RELCmd=$param
    getParameter Class2AbortCmd $1;		Class2AbortCmd=$param
    getParameter Class2RecvDataTrigger $1;	Class2RecvDataTrigger=$param
    getParameter Class2XmitWaitForXON $1;	Class2XmitWaitForXON=$param

    saveModemProtoParameters
}

addSedCmd()
{
    test "$1" != "$2" && ModemCmds="$ModemCmds -e '/$3:/s/$1/$2/'"
}

#
# Setup the sed commands for crafting the configuration file:
#
makeSedModemCommands()
{
    ModemCmds=""
    addSedCmd "$protoMaxRate"		"$ModemRate"		MaxRate
    addSedCmd "$protoSendFillOrder"	"$ModemSendFillOrder"	SendFillOrder
    addSedCmd "$protoRecvFillOrder"	"$ModemRecvFillOrder"	RecvFillOrder
    addSedCmd "$protoFlowControl"	"$ModemFlowControl"	FlowControl
    addSedCmd "$protoFlowControlCmd"	"$ModemFlowControlCmd"	FlowControlCmd
    addSedCmd "$protoSetupDTRCmd"	"$ModemSetupDTRCmd"	SetupDTRCmd
    addSedCmd "$protoSetupDCDCmd"	"$ModemSetupDCDCmd"	SetupDCDCmd
    addSedCmd "$protoDialCmd"		"$ModemDialCmd"		DialCmd
    addSedCmd "$protoAnswerCmd"		"$ModemAnswerCmd"	AnswerCmd
    addSedCmd "$protoNoAutoAnswerCmd"	"$ModemNoAutoAnswerCmd"	NoAutoAnswerCmd
    addSedCmd "$protoSetVolumeCmd"	"$ModemSetVolumeCmd"	SetVolumeCmd
    addSedCmd "$protoEchoOffCmd"	"$ModemEchoOffCmd"	EchoOffCmd
    addSedCmd "$protoVerboseResultsCmd"	"$ModemVerboseResultsCmd" \
	VerboseResultsCmd
    addSedCmd "$protoResultCodesCmd"	"$ModemResultCodesCmd"	ResultCodesCmd
    addSedCmd "$protoOnHookCmd"		"$ModemOnHookCmd"	OnHookCmd
    addSedCmd "$protoSoftResetCmd"	"$ModemSoftResetCmd"	SoftResetCmd
    addSedCmd "$protoWaitTimeCmd"	"$ModemWaitTimeCmd"	WaitTimeCmd
    addSedCmd "$protoCommaPauseTimeCmd"	"$ModemCommaPauseTimeCmd" \
	CommaPauseTimeCmd
    addSedCmd "$protoResetCmds"		"$ModemResetCmds"	ResetCmds
    case "$ModemType" in
    Class2)
	addSedCmd "$protoBORCmd"	"$Class2BORCmd"		Class2BORCmd
	addSedCmd "$protoCQCmd"		"$Class2CQCmd"		Class2CQCmd
	addSedCmd "$protoRELCmd"	"$Class2RELCmd"		Class2RELCmd
	addSedCmd "$protoAbortCmd"	"$Class2AbortCmd"	Class2AbortCmd
	addSedCmd "$protoRecvDataTrigger" "$Class2RecvDataTrigger" \
	   Class2RecvDataTrigger
	addSedCmd "$protoXmitWaitForXON" "$Class2XmitWaitForXON" \
	   Class2XmitWaitForXON
	;;
    esac
    # escape any &'s that may appear in modem strings
    ModemCmds=`echo "$ModemCmds" | sed -e 's/&/\\\&/g'`
}

#
# Check if the configured flow control scheme is
# consistent with the tty device being used.
#
checkFlowControlAgainstTTY()
{
    case "$ModemFlowControl" in
    xonxoff|XONXOFF)
	if [ "$TTY" = "ttyf${PORT}" -a -c /dev/ttym${PORT} ]; then
	    echo ""
	    echo "Warning, the modem is setup to use software flow control,"
	    echo "but the \"$TTY\" device is used with hardware flow control"
	    prompt "Do you want to use \"ttym${PORT}\" instead [yes]?"
	    read x
	    if [ -z "$x" -o "$x" = "y" -o "$x" = "yes" ]; then
		TTY="ttym${PORT}"
		DEVID="`echo $TTY | tr '/' '_'`"
		CONFIG=$CPATH.$DEVID
	    fi
	fi
	;;
    rtscts|RTSCTS)
	if [ "$TTY" = "ttym${PORT}" -a -c /dev/ttyf${PORT} ]; then
	    echo ""
	    echo "Warning, the modem is setup to use hardware flow control,"
	    echo "but the \"$TTY\" device does not honor the RTS/CTS signals."
	    prompt "Do you want to use \"ttyf${PORT}\" instead [yes]?"
	    read x
	    if [ -z "$x" -o "$x" = "y" -o "$x" = "yes" ]; then
		TTY="ttyf${PORT}"
		DEVID="`echo $TTY | tr '/' '_'`"
		CONFIG=$CPATH.$DEVID
	    fi
	fi
	;;
    esac
}

printIfNotNull()
{
    test "$2" && echo "$1	$2"
}

#
# Print the current modem-related parameters.
#
printModemConfig()
{
    echo ""
    echo "The modem configuration parameters are:"
    echo ""
    printIfNotNull "ModemRate:	"		"$ModemRate"
    printIfNotNull "ModemSendFillOrder:"	"$ModemSendFillOrder"
    printIfNotNull "ModemRecvFillOrder:"	"$ModemRecvFillOrder"
    printIfNotNull "ModemFlowControl:"		"$ModemFlowControl"
    printIfNotNull "ModemFlowControlCmd:"	"$ModemFlowControlCmd"
    printIfNotNull "ModemSetupDTRCmd:"		"$ModemSetupDTRCmd"
    printIfNotNull "ModemSetupDCDCmd:"		"$ModemSetupDCDCmd"
    printIfNotNull "ModemDialCmd:	"	"$ModemDialCmd"
    printIfNotNull "ModemAnswerCmd:	"	"$ModemAnswerCmd"
    printIfNotNull "ModemNoAutoAnswerCmd:"	"$ModemNoAutoAnswerCmd"
    printIfNotNull "ModemSetVolumeCmd:"		"$ModemSetVolumeCmd"
    printIfNotNull "ModemEchoOffCmd:"		"$ModemEchoOffCmd"
    printIfNotNull "ModemVerboseResultsCmd"	"$ModemVerboseResultsCmd"
    printIfNotNull "ModemResultCodesCmd"	"$ModemResultCodesCmd"
    printIfNotNull "ModemOnHookCmd:	"	"$ModemOnHookCmd"
    printIfNotNull "ModemSoftResetCmd:"		"$ModemSoftResetCmd"
    printIfNotNull "ModemWaitTimeCmd:"		"$ModemWaitTimeCmd"
    printIfNotNull "ModemCommaPauseTimeCmd:"	"$ModemCommaPauseTimeCmd"
    printIfNotNull "ModemResetCmds:	"	"$ModemResetCmds"
    case "$ModemType" in
    Class2)
	printIfNotNull "Class2BORCmd:	"	"$Class2BORCmd"
	printIfNotNull "Class2CQCmd:	"	"$Class2CQCmd"
	printIfNotNull "Class2RELCmd:	"	"$Class2RELCmd"
	printIfNotNull "Class2AbortCmd:	"	"$Class2AbortCmd"
	printIfNotNull "Class2RecvDataTrigger:"	"$Class2RecvDataTrigger"
	printIfNotNull "Class2XmitWaitForXON:"	"$Class2XmitWaitForXON"
	;;
    esac
    echo ""
}

#
# Prompt the user to edit the current parameters.  Note that
# we can only edit parameters that are in the prototype config
# file; thus all the checks to see if the prototype value exists.
#
promptForModemParameters()
{
    test "$protoMaxRate" && { 
	promptForNumericParameter "$ModemRate" \
	    "DCE-DTE communication rate";
	ModemRate=$param;
    }
    test "$protoFlowControl" && {
	promptForFlowControlParameter "$ModemFlowControl" \
	    "Flow control scheme between host and modem"; \
	ModemFlowControl=$param;
    }
    test "$protoFlowControlCmd" && {
	promptForATCmdParameter "$ModemFlowControlCmd" \
	    "Command to setup DCE-DTE flow control"; \
	ModemFlowControlCmd=$param;
    }
    test "$protoSetupDTRCmd" && {
	promptForATCmdParameter "$ModemSetupDTRCmd" \
	    "Command for setting up DTR handling";
	ModemSetupDTRCmd=$param;
    }
    test "$protoSetupDCDCmd" && {
	promptForATCmdParameter "$ModemSetupDCDCmd" \
	    "Command for setting up DCD handling";
	ModemSetupDCDCmd=$param;
    }
    test "$protoDialCmd" && {
	promptForATCmdParameter "$ModemDialCmd" \
	    "Command for dialing (%s for number)";
	ModemDialCmd=$param;
    }
    test "$protoAnswerCmd" && {
	promptForATCmdParameter "$ModemAnswerCmd" \
	    "Command for answering the phone";
	ModemAnswerCmd=$param;
    }
    test "$protoNoAutoAnswerCmd" && {
	promptForATCmdParameter "$ModemNoAutoAnswerCmd" \
	    "Command for disabling auto-answer";
	ModemNoAutoAnswerCmd=$param;
    }
    test "$protoSetVolumeCmd" && {
	promptForATCmdParameter "$ModemSetVolumeCmd" \
	    "Commands for setting modem speaker volume levels"; \
	ModemSetVolumeCmd=$param;
    }
    test "$protoEchoOffCmd" && {
	promptForATCmdParameter "$ModemEchoOffCmd" \
	    "Command to disable command echo";
	ModemEchoOffCmd=$param;
    }
    test "$protoVerboseResultsCmd" && {
	promptForATCmdParameter "$ModemVerboseResultsCmd" \
	    "Command to enable verbose command results"; \
	ModemVerboseResultsCmd=$param;
    }
    test "$protoResultCodesCmd" && {
	promptForATCmdParameter "$ModemResultCodesCmd" \
	    "Command to enable result codes";
	ModemResultCodesCmd=$param;
    }
    test "$protoOnHookCmd" && {
	promptForATCmdParameter "$ModemOnHookCmd" \
	    "Command to place phone on hook (hangup)";
	ModemOnHookCmd=$param;
    }
    test "$protoSoftResetCmd" && {
	promptForATCmdParameter "$ModemSoftResetCmd" \
	    "Command to do a soft reset";
	ModemSoftResetCmd=$param;
    }
    test "$protoWaitTimeCmd" && {
	promptForATCmdParameter "$ModemWaitTimeCmd" \
	    "Command to set time to wait for carrier";
	ModemWaitTimeCmd=$param;
    }
    test "$protoCommaPauseTimeCmd" && {
	promptForATCmdParameter "$ModemCommaPauseTimeCmd" \
	    "Command to set pause time for \",\" in dialing string";
	ModemCommaPauseTimeCmd=$param;
    }
    test "$protoSendFillOrder" && {
	promptForBitOrderParameter "$ModemSendFillOrder" \
	    "Bit order modem expects for transmitted facsimile data";
	ModemSendFillOrder=$param;
    }
    test "$protoRecvFillOrder" && {
	promptForBitOrderParameter "$ModemRecvFillOrder" \
	    "Bit order of received facsimile data";
	ModemRecvFillOrder=$param;
    }
    test "$protoResetCmds" && {
	promptForATCmdParameter "$ModemResetCmds" \
	    "Additional commands to issue during modem reset";
	ModemResetCmds=$param;
    }
    case "$ModemType" in
    Class2)
	test "$protoBORCmd" && {
	    promptForATCmdParameter "$Class2BORCmd" \
		"Command to setup bit order for phase B/C/D data transfers";
	    Class2BORCmd=$param;
	}
	test "$protoCQCmd" && {
	    promptForATCmdParameter "$Class2CQCmd" \
		"Command to setup copy quality handling";
	    Class2CQCmd=$param;
	}
	test "$protoRELCmd" && {
	    promptForATCmdParameter "$Class2RELCmd" \
		"Command to setup byte-aligned EOL codes when receiving";
	    Class2RELCmd=$param;
	}
	test "$protoAbortCmd" && {
	    promptForATCmdParameter "$Class2AbortCmd" \
		"Command to abort an established session";
	    Class2AbortCmd=$param;
	}
	test "$protoRecvDataTrigger" && {
	    promptForATCmdParameter "$Class2RecvDataTrigger" \
		"Character sent before receiving page data";
	    Class2RecvDataTrigger=$param;
	}
	test "$protoXmitWaitForXON" && {
	    promptForBooleanParameter "$Class2XmitWaitForXON" \
		"Wait for XON before sending page data";
	    Class2XmitWaitForXON=$param;
	}
	;;
    esac
}

#
# Construct the configuration file.
#
if [ $ProtoType = config.skel	\
  -o $ProtoType = config.class1	\
  -o $ProtoType = config.class2	\
]; then
    # Go through each important parameter (sigh)
cat<<EOF

There is no prototype configuration file for your modem, so we will
have to fill in the appropriate parameters by hand.  You will need the
manual for how to program your modem to do this task.  In case you are
uncertain of the meaning of a configuration parameter you should
consult the config(4F) manual page for an explanation.

Note that modem commands are specified without a leading "AT".  The "AT"
will not be displayed in the prompts and if you include a leading "AT"
it will automatically be deleted.  Likewise quote marks (") will not be
displayed and will automatically be deleted.  You can use this facility
to supply null parameters as "".

Finally, beware that the set of parameters is long.  If you prefer to
use your favorite editor instead of this script you should fill things
in here as best you can and then edit the configuration file

"$CONFIG"

after completing this procedure.

EOF
    ok="no"
else
    #
    # If a dialing prefix was being used, add it to the
    # dialing command string.  Perhaps this is too simplistic.
    #
    if [ "$DialingPrefix" ]; then
	ServerCmds="$ServerCmds -e \'/ModemDialCmd:/s/%s/$DialingPrefix%s/\'"
    fi
    #
    # If getty use was previously enabled, uncomment the
    # GettyArgs line in the prototype file--though this
    # may not always work (sigh).
    #
    if [ "$GettyArgs" ]; then
	ServerCmds="$ServerCmds -e \'/GettyArgs:/s/^#//\'"
    fi
    ok="skip"
fi

getModemProtoParameters $proto
while [ "$ok" != "" -a "$ok" != "y" -a "$ok" != "yes" ]; do
    if [ "$ok" != "skip" ]; then
	promptForModemParameters
    fi
    printModemConfig
    if [ "$OS" = "IRIX" ]; then
	checkFlowControlAgainstTTY
    fi
    #
    # XXX not sure what kind of consistency checking that can
    # done w/o knowing more about the modem...
    #
    prompt "Are these ok [yes]?"; read ok
done
makeSedModemCommands

#
# All done with the prompting; edit up a config file!
#
if [ -f $CONFIG ]; then
    echo "Saving existing configuration file as \"$CONFIG.sav\"."
    mv $CONFIG $CONFIG.sav
fi
echo "Creating new configuration file \"$CONFIG\"."
eval sed $ServerCmds $ModemCmds '-e /CONFIG:/d' $proto >$CONFIG
chown $FAX $CONFIG; chgrp $faxGID $CONFIG; chmod 644 $CONFIG

#
# Create FIFO.<tty> special file.
#
FIFO=$SPOOL/FIFO.$DEVID
echo "Creating \"$FIFO\" in the spooling directory."
test -p $FIFO \
    || (mkfifo $FIFO) >/dev/null 2>&1 \
    || (mknod $FIFO p) >/dev/null 2>&1 \
    || { echo "Cannot create fifo \"$FIFO\""; exit 1; }
chown $FAX $FIFO; chgrp $faxGID $FIFO; chmod 600 $FIFO

echo "Done setting up the modem configuration."

#
# And, last but not least, startup a server for the modem.
#
echo ""
prompt "Startup a facsimile server for this modem [yes]?"; read x
if [ "$x" = "" -o "$x" = "yes" -o "$x" = "y" ]; then
    echo "$SERVERDIR/faxd -m /dev/$TTY&"
    $SERVERDIR/faxd -m /dev/$TTY&
fi
exec >/dev/null 2>&1
