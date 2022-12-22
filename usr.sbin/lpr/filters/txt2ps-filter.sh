#! /bin/sh -
#	BSDI $Id: txt2ps-filter.sh,v 1.1 1993/12/18 22:05:42 sanders Exp $
IFS=""; export IFS
PATH="/bin:/usr/bin"; export PATH
/usr/contrib/bin/textps || exit 2
exit 0
