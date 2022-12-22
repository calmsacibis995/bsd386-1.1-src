/*
**  Get some memory or die trying.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/lmem.c,v 1.1.1.1 1993/02/23 18:00:01 polk Exp $";
#endif	/* RCSID */


align_t
getmem(i, j)
    unsigned int	 i;
    unsigned int	 j;
{
    extern char		*calloc();
    align_t		 p;

    /* Lint fluff:  "possible pointer alignment problem." */
    if ((p = (align_t)calloc(i, j)) == NULL) {
	/* Print the unsigned values as int's so ridiculous values show up. */
	Fprintf(stderr, "Can't Calloc(%d,%d), %s.\n", i, j, Ermsg(errno));
	exit(1);
    }
    return(p);
}
