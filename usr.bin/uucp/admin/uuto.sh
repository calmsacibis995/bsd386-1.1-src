#!/bin/sh

# This shell script provides a simpler interface to `uucp(1)'.
#
#	RCSID	$Id: uuto.sh,v 1.1.1.1 1992/09/28 20:08:27 trent Exp $
#
#	$Log: uuto.sh,v $
# Revision 1.1.1.1  1992/09/28  20:08:27  trent
# Latest UUCP from ziegast@uunet
#
# Revision 1.1  1992/04/14  21:50:41  piers
# Initial revision
#

USAGE="Usage: $0 [-m] [-p] file|directory ... [remote]!user"

trap "exit 1" 1 2 13 15

LOCAL=`uuname -l`

export UUDIR

ADDR=
ARGS=
COPY=
DEST=
DIRS=
FILES=
FLGS=
USER=

for i
do
	case "${ADDR}${i}" in
	-m)	ARGS="-m"; FLGS="$FLGS -m"; shift	;;
	-p)	COPY=true; FLGS="$FLGS -p"; shift	;;
	-*)	echo $USAGE; exit 1			;;
	*)	ADDR="$i"				;;
	esac
done

case $# in
0|1)	echo $USAGE; exit 1	;;
esac

USER=`expr $ADDR : '.*!\(.*\)'`
DEST=`expr $ADDR : '\(.*\)!'`

case "$USER" in
'')	echo "destination must specify at least <!user>"
	echo $USAGE; exit 2	;;
esac

for i
do
	case $i in
	$ADDR)	;;
	.|..)	;;
	*)	if   [ -r "$i" -a -f "$i" ]
		then
			FILES="$FILES $i"
		elif [ -r "$i" -a -d "$i" ]
		then
			DIRS="$DIRS $i"
		else
			case "$UUDIR" in
			'')	echo "$i: file/directory not found"
				exit 3
				;;
			esac
		fi
		;;
	esac
done

case "$COPY" in
true)	ARGS="$ARGS -C"	;;
esac

ARGS="$ARGS -d -n$USER"

DONE=

case "$DIRS" in
'')	;;
*)
	for i in $DIRS
	do
		( cd $i
		UUDIR="$UUDIR/$i" export UUDIR
		FILES=
		for j in `ls -a`
		do
			case $j in
			.|..)	;;
			*)	FILES="$FILES $j"
				;;
			esac
		done
		case "$FILES" in
		'')	;;
		*)	$0 $FLGS $FILES $ADDR
			;;
		esac )
		DONE=true
	done
	;;
esac

case "$FILES" in
'')	;;
*)	echo "uucp $ARGS $FILES $DEST!~/receive/$USER/${LOCAL}${UUDIR}/"
	DONE=true
	;;
esac

case "$DONE" in
'')	echo $USAGE
	exit 2
	;;
esac
