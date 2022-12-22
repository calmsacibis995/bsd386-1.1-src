case $CONFIG in
'')
    if test ! -f config.sh; then
	ln ../config.sh . || \
	ln ../../config.sh . || \
	ln ../../../config.sh . || \
	(echo "Can't find config.sh."; exit 1)
    fi
    . ./config.sh
    ;;
esac
case "$0" in
*/*) cd `expr X$0 : 'X\(.*\)/'` ;;
esac

echo "Extracting utils/checkalias (with variable substitutions)"
echo $startsh > checkalias
cat >>checkalias <<!DO!SUBS!
# checkalias: part of the Elm mail system
# @(#)Id: checkalias.SH,v 5.2 1993/04/12 02:11:05 syd Exp 
#		Copyright (c) 1988-1992 USENET Community Trust
# 		Copyright (c) 1986,1987 Dave Taylor
 

if [ \$# -eq 0 ] ; then
	echo "usage: \$0 alias ..." 1>&2
	exit 1
fi

elmalias -en \$@ | \\
	$lib/prlong -w 78 -1 "Expands to: " -l "	" -f ", "
exit \$?

!DO!SUBS!
