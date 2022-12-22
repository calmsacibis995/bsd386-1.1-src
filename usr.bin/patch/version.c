/* $Header: /bsdi/MASTER/BSDI_OS/usr.bin/patch/version.c,v 1.2 1992/01/04 19:03:05 kolstad Exp $
 *
 * $Log: version.c,v $
 * Revision 1.2  1992/01/04  19:03:05  kolstad
 * Finish up initial import of BSDI0.2
 *
 * Revision 2.0  86/09/17  15:40:11  lwall
 * Baseline for netwide release.
 * 
 */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "patchlevel.h"
#include "version.h"

/* Print out the version number and die. */

void
version()
{
    extern char rcsid[];

#ifdef lint
    rcsid[0] = rcsid[0];
#else
    fatal3("%s\nPatch level: %d\n", rcsid, PATCHLEVEL);
#endif
}
