#!/bin/sh

#	BSDI	$Id: areacode.sh,v 1.1.1.1 1994/01/13 20:50:28 polk Exp $
# 	inspired by Dan Jacobson

PATH=/bin:/usr/bin

case $# in 0)
	cat 1>&2 << EOF
	$0: usage:
	$ `basename $0` areacode [...]
	$ `basename $0` city [...]
	The arguments are egrep regular expressions.
EOF
	exit 2
esac

expression=$1
shift
for arg in "$@"
do
	expression="$expression|$arg"
done
egrep -i "$expression" /usr/share/misc/na.phone | \
	awk -F: '{ print $1"\t"$3" ("$4")\t"$2; }' | sort -k 2
egrep -i "$expression" /usr/share/misc/inter.phone | \
	awk -F: '{ print $1"\t"$2"\t"$4"\t"$3; }' | sort -k 3
