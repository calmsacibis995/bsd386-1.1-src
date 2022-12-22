#! /bin/sh
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/port/sco/install.sh,v 1.1.1.1 1994/01/14 23:10:30 torek Exp $
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
# Shell script to emulate SGI install program under SCO ODT 2.0
#
preopts=
postopts=
SaveFirst=no
HasSource=yes
RemoveFirst=no

INSTALL="/etc/install"
CMD="$INSTALL"
SRC=
FILES=
DESTDIR=
CHMOD=/bin/true
CHOWN=/bin/true
CHGRP=/bin/true
RM="/bin/rm -f"
ECHO=/bin/true

while [ -n "$1" ]
do
    arg=$1
    case $arg in
    -m)		shift; CHMOD="chmod $1";;
    -u)		shift; CHOWN="chown $1";;
    -g)		shift; CHGRP="chgrp $1";;
    -o)		SaveFirst="yes";;
    -O)		RemoveFirst="yes"; SaveFirst="yes";;
    -root)	shift; ROOT=$1;;
    -dir)	CMD="mkdir -p"; HasSource="no";;
    -fifo)	CMD="mkfifo"; HasSource="no";;
    -ln)	shift; CMD="ln"; SRC=$1;;
    -lns)	shift; CMD="ln"; preopts="-s"; SRC=$1;;
    -src)	shift; SRC=$1;;
    -[fF])	shift; DESTDIR=$1;;
    # these are skipped/not handled
    -idb|-new|-rawidb|-blk|-chr) shift;;
    -v)		ECHO="/bin/echo";;
    -*) 	;;
    *)		FILES="$FILES $arg";;
    esac
    shift
done
if [ $RemoveFirst = "yes" ]; then
    for f in $FILES
    do
	bf=`basename $f`
	if [ -f $ROOT/$DESTDIR/$bf ]; then
	    $ECHO "$RM $ROOT/$DESTDIR/$bf"
	    $RM $ROOT/$DESTDIR/$bf
	fi
    done
fi
if [ $SaveFirst = "yes" ]; then
    for f in $FILES
    do
	bf=`basename $f`
	if [ -f $ROOT/$DESTDIR/$bf ]; then
	    $ECHO "$MV $ROOT/$DESTDIR/$bf $ROOT/$DESTDIR/OLD$bf"
	    mv $ROOT/$DESTDIR/$bf $ROOT/$DESTDIR/OLD$bf
	fi
    done
fi
for f in $FILES
do
    $ECHO "$RM $ROOT/$DESTDIR/$f"
    $RM $ROOT/$DESTDIR/$f
    if [ "$SRC" = "" -a $HasSource = "yes" ]; then
	if [ "$CMD" = "$INSTALL" ] ; then
	    $ECHO "$CMD -f $ROOT/$DESTDIR $f"
	    $CMD -f $ROOT/$DESTDIR $f 
	else
	    $ECHO "$CMD $preopts $f $ROOT/$DESTDIR $postopts"
	    $CMD $preopts $f $ROOT/$DESTDIR $postopts
	fi
    else
	if [ "$CMD" = "$INSTALL" ] ; then
	    $ECHO "$CMD -f $ROOT/$DESTDIR $SRC $ROOT/$DESTDIR/$f"
	    $CMD -f $ROOT/$DESTDIR $SRC $ROOT/$DESTDIR/$f
	    $ECHO "mv $ROOT/$DESTDIR/$SRC $ROOT/$DESTDIR/$f"
	    mv $ROOT/$DESTDIR/$SRC $ROOT/$DESTDIR/$f
	else
	    $ECHO "$CMD $preopts $SRC $ROOT/$DESTDIR/$f $postopts"
	    $CMD $preopts $SRC $ROOT/$DESTDIR/$f $postopts
	fi
    fi
    $ECHO "$CHOWN $ROOT/$DESTDIR/$f"; $CHOWN $ROOT/$DESTDIR/$f
    $ECHO "$CHGRP $ROOT/$DESTDIR/$f"; $CHGRP $ROOT/$DESTDIR/$f
    $ECHO "$CHMOD $ROOT/$DESTDIR/$f"; $CHMOD $ROOT/$DESTDIR/$f
done
