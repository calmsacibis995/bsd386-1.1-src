/* $Header: /bsdi/MASTER/BSDI_OS/contrib/gpatch/version.c,v 1.1.1.1 1994/01/13 21:15:32 polk Exp $
 *
 * $Log: version.c,v $
 * Revision 1.1.1.1  1994/01/13  21:15:32  polk
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

void my_exit();

/* Print out the version number and die. */

void
version()
{
    fprintf(stderr, "Patch version %s\n", PATCH_VERSION);
    my_exit(0);
}
