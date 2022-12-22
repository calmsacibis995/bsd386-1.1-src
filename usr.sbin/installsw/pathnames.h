/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: pathnames.h,v 1.3 1994/01/06 04:32:49 polk Exp $	*/

#include <paths.h>

/* XXX - use relative pathnames and expect them to be in 
	 the users path.  Security problem? */

#define _PATH_ECHO		"echo"
#define _PATH_RSH		"rsh"
#define _PATH_DD		"dd"
#define _PATH_UNZIP		"gzcat"
#ifndef _PATH_UNCOMPRESS
#define _PATH_UNCOMPRESS	"zcat"
#endif
#define _PATH_MT		"mt"
#define _PATH_TAR		"tar"
#define _PATH_PAX		"pax"
#define _PATH_CAT		"cat"
#define _PATH_RM		"rm"
