/*
 *   $Id: fontwidt.c,v 1.1.1.1 1993/02/10 18:05:35 sanders Exp $
 *
 *   This code was written by Craig Southeren whilst under contract
 *   to Computer Sciences of Australia, Systems Engineering Division.
 *   It has been kindly released by CSA into the public domain.
 *
 *   Neither CSA or me guarantee that this source code is fit for anything,
 *   so use it at your peril. I don't even work for CSA any more, so
 *   don't bother them about it. If you have any suggestions or comments
 *   (or money, cheques, free trips =8^) !!!!! ) please contact me
 *   care of geoffw@extro.ucc.oz.au
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fontwidt.h"

/********************************
  defines
 ********************************/
#ifndef True
#define True	1
#define False	0
#endif

/********************************
  imports
 ********************************/
#include "main.h"


/********************************
  exports
 ********************************/


/********************************
  globals
 ********************************/

/* this table contains the width of a space character for each size of font multiplied by 100

  it was produced by the following postscript code

%!
/str 10 string def

/doit { dup /Courier findfont exch scalefont setfont ( ) stringwidth pop 100 mul cvi str cvs print (, \/* ) print str cvs print ( *\/\n) print } def

5 1 30 { doit } for

*/

static int CourierFontWidths[] = {

299, /* 5 */
359, /* 6 */
419, /* 7 */
479, /* 8 */
539, /* 9 */
599, /* 10 */
659, /* 11 */
719, /* 12 */
779, /* 13 */
839, /* 14 */
899, /* 15 */
959, /* 16 */
1019, /* 17 */
1079, /* 18 */
1139, /* 19 */
1199, /* 20 */
1259, /* 21 */
1319, /* 22 */
1379, /* 23 */
1439, /* 24 */
1499, /* 25 */
1559, /* 26 */
1619, /* 27 */
1679, /* 28 */
1739, /* 29 */
1799, /* 30 */
};


/********************************
  function
 ********************************/

int GetFontWidth (fontname, size)

char *fontname;
long  size;

{
  size /= 100;

  if (strncmp (fontname, "Courier", 7) != 0) {
    fprintf (stderr, "%s: can only use Courier - sorry!!\n", progname);
    exit (1);
  }

  if (size < 5 || size > 30) {
    fprintf (stderr, "%s: %i not bwteen valid font sizes of 5 and 30 - sorry!!\n", progname, size);
    exit (1);
  }

  return CourierFontWidths [size-5];
}
