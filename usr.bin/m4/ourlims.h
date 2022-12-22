/*	@(#)BSDI $Id: ourlims.h,v 1.1 1993/12/16 04:44:09 sanders Exp $ */

/*  File   : ourlims.h
    Author : Richard A. O'Keefe
    Defines: UCHAR_MAX, CHAR_BIT, LONG_BIT
*/
/*  If <limits.h> is available, use that.
    Otherwise, use 8-bit byte as the default.
    If the number of characters is a power of 2, you might be able
    to use (unsigned char)(~0), but why get fancy?
*/
#ifdef	__STDC__
#include <limits.h>
#else
#define	UCHAR_MAX 255
#define	CHAR_BIT 8
#endif
#define LONG_BIT 32
