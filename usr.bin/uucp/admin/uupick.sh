#!/bin/sh

# This shell script collects files sent by `uuto(1)'.
#
#	RCSID	$Id: uupick.sh,v 1.2 1994/01/31 01:25:39 donn Exp $
#
#	$Log: uupick.sh,v $
# Revision 1.2  1994/01/31  01:25:39  donn
# Latest version from Paul Vixie.
#
# Revision 1.2  1994/01/29  20:57:06  vixie
# 1.1
#
# Revision 1.1  1994/01/28  06:41:33  vixie
# Initial revision
#
# Revision 1.1.1.1  1992/09/28  20:08:28  trent
# Latest UUCP from ziegast@uunet
#
# Revision 1.1  1992/04/14  21:50:41  piers
# Initial revision
#

USAGE="Usage: $0 [-s sysname]"
PUBDIR=/var/spool/uucppublic

case `/bin/echo '\c'` in
\\c)  n="-n" export n; c="" export c; SYS=BSD ;;
*)    n="" export n; c="\c" export c; SYS=V   ;;
esac

EXPECT=
FROM=
USER=

for i
do
	case "$EXPECT$FROM$i" in
	-s)	EXPECT=1
		;;
	-s*)	FROM=`expr $i : '-s\(.*\)'`
		;;
	*)	case "$EXPECT" in
		'')	echo "$USAGE"
			exit 1
			;;
		esac
		FROM=$i
		EXPECT=
		;;
	esac
done

case "$USER" in
'')	case "$LOGNAME" in
	'')	echo "Who are you?"
		;;
	*)	USER=$LOGNAME
		;;
	esac
	;;
esac

PUBDIR=$PUBDIR/receive/$USER

if [ ! -d $PUBDIR ]
then
	exit 0
fi

CWD=`pwd`
TMP=/tmp/uupick$$

trap "rm -f $TMP; exit 1" 1 2 13 15

for i in `ls -f $PUBDIR`
do
	case $i in
	.*)	continue
		;;
	esac

	case "$FROM" in
	'')	;;
	$i)	;;
	*)	continue
		;;
	esac

	if [ ! -d $PUBDIR/$i ]
	then
		continue
	fi

	cd $PUBDIR/$i

	for j in `ls -f`
	do
		case $j in
		.|..)	continue
			;;
		esac

		if [ -d $j ]
		then
			type=directory
		else
			type=file
		fi

		while echo $n "from system $i: $type $j ? "$c
		do
		  	read cmd dir

			case "$cmd" in
			'')	continue 2
				;;
			esac

			trap ": ;;" 1

			case $cmd in
			d)	rm -rf $j
				break
				;;

		        a|m)	case "$dir" in
				'')	dir=$CWD
					;;
				/*)	;;
			 	*)	dir=$CWD/$dir
					;;
				esac

				if [ ! -d $dir -o ! -w $dir ]
				then
					echo "directory \`$dir' doesn't exist or isn't writable"
					continue
				fi

				case $cmd in
				a)	find $j -print	;;
				m)	find . -print	;;
				esac | sort -r | grep -v '^\.$' >$TMP

				cpio -pdmu $dir <$TMP

				for k in `cat $TMP`
				do
					new="$dir/$k"
					old="$PUBDIR/$i/$k"
					if [ -f $old ]
					then
						if cmp $old $new
						then
				    			rm -f $old
						else
							echo "file \`$old' not removed"
						fi
					else
						rmdir $old 2>/dev/null
					fi
				done

				rm -f $TMP

				case $cmd in
				a)	break 2	;;
				m)	break	;;
				esac
				;;

			 p)	if [ -d $j ]
				then
					find $j -print
				else
					cat $j
				fi
				;;

			 q)	exit 0
				;;

			 !*)	cmd=`expr "$cmd $dir" : '!\(.*\)'`
				cd $CWD
				sh -c "$cmd"
				cd $PUBDIR/$i
				echo '!'
				;;

			 *)	echo \
	"Usage: [newline][d][m dir][a dir][p][q][ctrl-d][!cmd]"
				;;
			esac

			trap "exit 1" 1
		done
	done
done
