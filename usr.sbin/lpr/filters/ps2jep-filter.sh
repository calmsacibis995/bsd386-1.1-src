#! /bin/sh -
#	BSDI $Id: ps2jep-filter.sh,v 1.2 1994/01/13 17:54:48 sanders Exp $
#
# PostScript to HP LaserJet Plus filter for use with lpr.
# Requires ghostscript (/usr/contrib/bin/gs and support files)
#
IFS=""; export IFS
PATH="/bin:/usr/bin"; export PATH

IF=/tmp/ps2jep.$$.ps
OF=/tmp/ps2jep.$$.jep
DEV=ljetplus
RES=300x300

gs=/usr/contrib/bin/gs

printf '\033E'
trap "/bin/rm -f /tmp/ps2jep.$$.*; printf '\033E'" 0
cat > /tmp/ps2jep.$$.ps || exit 2
{ echo showpage; echo quit; } | \
$gs -dSAFER -sDEVICE=$DEV -r$RES -sOutputFile=$OF $IF 2>/dev/null >/dev/null || exit 2
cat /tmp/ps2jep.$$.jep || exit 2
exit 0
