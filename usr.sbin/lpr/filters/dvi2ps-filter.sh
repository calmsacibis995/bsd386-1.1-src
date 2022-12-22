#! /bin/sh -
#	BSDI $Id: dvi2ps-filter.sh,v 1.2 1994/01/13 17:54:47 sanders Exp $
IFS=""; export IFS
PATH="/bin:/usr/bin"; export PATH
trap "/bin/rm -f /tmp/dvips.$$.dvi" 0
cat > /tmp/dvips.$$.dvi || exit 2
/usr/contrib/bin/dvips -f /tmp/dvips.$$.dvi 2>/dev/null | /usr/contrib/bin/lprps "$@" || exit 2
exit 0
