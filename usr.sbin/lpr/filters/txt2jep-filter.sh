#! /bin/sh -
#	BSDI $Id: txt2jep-filter.sh,v 1.1 1993/12/18 22:05:41 sanders Exp $
IFS=""; export IFS
PATH="/bin:/usr/bin"; export PATH
trap "printf '\033E'" 0
printf '\033E'
printf '\033&k2G'
/usr/libexec/lpr/lpf ${1+"$@"} || exit 2
exit 0
