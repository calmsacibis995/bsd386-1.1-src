#! /bin/sh -
#	BSDI $Id: dvi2jep-filter.sh,v 1.2 1994/01/30 22:21:05 sanders Exp $
IFS=""; export IFS
PATH="/bin:/usr/bin"; export PATH
printf '\033E'
trap "/bin/rm -f /tmp/dvijep.$$.*; printf '\033E'" 0
cat > /tmp/dvijep.$$.dvi || exit 2
/usr/contrib/bin/dvijep -b -q /tmp/dvijep.$$.dvi || exit 2
cat /tmp/dvijep.$$.dvi-jep || exit 2
exit 0
