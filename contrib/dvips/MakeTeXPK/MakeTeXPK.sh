#!/bin/sh
#
#   This script file makes a new TeX PK font, because one wasn't
#   found.  Parameters are:
#
#   name dpi bdpi magnification [mode]
#
#   `name' is the name of the font, such as `cmr10'.  `dpi' is
#   the resolution the font is needed at.  `bdpi' is the base
#   resolution, useful for figuring out the mode to make the font
#   in.  `magnification' is a string to pass to MF as the
#   magnification.  `mode', if supplied, is the mode to use.
#
#   Note that this file must execute Metafont, and then gftopk,
#   and place the result in the correct location for the PostScript
#   driver to find it subsequently.  If this doesn't work, it will
#   be evident because MF will be invoked over and over again.
# 
# Modified by karl@cs.umb.edu for (1) trying to build magnifications of
# John Sauter's fonts; (2) trying to build magnifications of Sun's F3
# fonts; (3) running gftopk if we have a gf but no pk.

#DESTDIR=/usr/local/lib/tex/fonts/tmp
DESTDIR=/usr/contrib/lib/tex/fonts/tmp

# TEMPDIR needs to be unique for each process because of the possibility
# of simultaneous processes running this script.
TEMPDIR=/tmp/mtpk.$$

NAME=$1
DPI=$2
BDPI=$3
MAG=$4
MODE=$5

umask 0

if test "$MODE" = ""
then
   if test $BDPI = 300
   then MODE=CanonCX
   
   elif test $BDPI = 118
   then MODE=lview
   
   elif test $BDPI = 85
   then MODE=sun
   
   elif test $BDPI = 1270
   then MODE=LinotypeOneZeroZero

   else
      echo "I don't know the mode to make $BDPI dpi devices."
      echo "Have your TeX wizard update MakeTeXPK."
      exit 1
   fi
fi

GFNAME=$NAME.$DPI'gf'
PKNAME=$NAME.$DPI'pk'

# Clean up on normal or abnormal exit
trap "cd /; rm -rf $TEMPDIR $DESTDIR/pktmp.$$" 0 1 2 15

# Do we have a GF file in the current directory?
if test -r $GFNAME
then
  echo "gftopk ./$GFNAME $PKNAME"
  gftopk ./$GFNAME $PKNAME
  # Don't move the font; if the person knows enough to make fonts, they
  # know enough to have . in the font paths.
  exit 0
fi

# Do we have an F3 font?
# 
dtrg=/u/zapf/dtrg
if [ -r $dtrg/typescaler/fonts/f3b/$NAME.f3b ]
then
  PATH=$dtrg/src/`arch`/bin:$PATH
  export PATH
  cd $dtrg/fonts
  echo "f3topk -r $DPI $NAME"
  f3topk -r $DPI $NAME
  exit 0
fi

# No special case applies.  Try Metafont.
mkdir $TEMPDIR
cd $TEMPDIR

if test -r $DESTDIR/$PKNAME
then
   echo "$DESTDIR/$PKNAME already exists!"
   exit 0
fi

case $NAME in
  cm*) mf=cmmf;;
  *) mf=mf;;
esac

echo $mf "\mode:=$MODE; mag:=$MAG; scrollmode; input $NAME" \</dev/null
$mf "\mode:=$MODE; mag:=$MAG; scrollmode; input $NAME" </dev/null
if test $? -eq 1 -a $mf = cmmf
then
  # Perhaps no such MF source file, and it's CM.  Try Sauter's scripts.
  sauterdir=/usr/local/src/TeX+MF/tex82/utilities/sauter
  cd $sauterdir
  rootfont=`echo $NAME | sed 's/[0-9]*$//'`
  pointsize=`echo $NAME | sed 's/cm[a-z]*//'`
  make-mf $rootfont $pointsize
  echo "(Trying interpolated/extrapolated CM source.)"
  $mf "\mode:=$MODE; mag:=$MAG; scrollmode; input mf/$NAME" < /dev/null
  if test $? -eq 0 -a -r $GFNAME
  then mv $GFNAME $TEMPDIR
  fi
  rm -f $NAME.log mf/$NAME
  cd $TEMPDIR
fi

if test ! -r $GFNAME
then
 echo "MakeTeXPK failed to make $GFNAME."
 exit 1
fi

# Make the PK file.
gftopk ./$GFNAME $PKNAME


# Install the PK file carefully, since others may be doing the same
# as us simultaneously.
#
mv $PKNAME $DESTDIR/pktmp.$$
cd $DESTDIR
mv pktmp.$$ $PKNAME

exit 0
