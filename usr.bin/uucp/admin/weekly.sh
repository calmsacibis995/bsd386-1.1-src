#!/bin/sh

# vixie 01dec93 [original]
#
# $Id: weekly.sh,v 1.2 1994/01/31 04:42:07 donn Exp $

Nvers=4

#
# age log files
#

cd /var/log/uucp
for subdir in *
do
	if [ -d $subdir ]; then
	  ( cd $subdir
	    for file in *
	    do
		case $file in
		*.[0-9]) ;;
		*) /usr/libexec/uuage $file $Nvers ;;
		esac
	    done
	  )
	fi
done

#
# should generate some kind of report here
#

exit 0
