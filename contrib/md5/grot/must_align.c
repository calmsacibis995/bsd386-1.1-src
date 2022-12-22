/* @(#)must_align.c	6.1 1/10/94 22:52:45 */
/*
 * must_align - determine if longs must be aligned
 *
 * This file was written by:
 *
 *	 Landon Curt Noll  (chongo@toad.com)	chongo <was here> /\../\
 *
 * This code has been placed in the public domain.  Please do not 
 * copyright this code.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH  REGARD  TO
 * THIS  SOFTWARE,  INCLUDING  ALL IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS.  IN NO EVENT SHALL  LANDON  CURT
 * NOLL  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM  LOSS  OF
 * USE,  DATA  OR  PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * See shsdrvr.c and md5drvr.c for version and modification history.
 */

#include <stdio.h>

main()
{
    char byte[2*sizeof(unsigned long)];	/* mis-alignment buffer */
    unsigned long *p;	/* mis-alignment pointer */
    int i;

    /* assume the worst */
    printf("#define MUST_ALIGN\n");
    fflush(stdout);

#if !defined(MUST_ALIGN)
    /* mis-align our long fetches */
    for (i=0; i < sizeof(long); ++i) {
	p = (unsigned long *)(byte+i);
	*p = i;
	*p += 1;
    }

    /* if we got here, then we can mis-align longs */
    printf("#undef MUST_ALIGN\n");
#endif
    exit(0);
}
