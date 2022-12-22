/***************************************************************************
 * This program is Copyright (C) 1986, 1987, 1988 by Jonathan Payne.  JOVE *
 * is provided to you without charge, and with no warranty.  You may give  *
 * away copies of JOVE, including sources, provided that this notice is    *
 * included in all the files.                                              *
 ***************************************************************************/

/* tune.c is mechanically derived from tune.template */

#include "jove.h"

#ifndef	MAC

char
	*d_tempfile = "joveXXXXXX",	/* buffer lines go here */
	*p_tempfile = "jrecXXXXXX",	/* line pointers go here */
#ifdef	ABBREV
	*a_tempfile = "jabbXXXXXX",	/* abbreviation editing tempfile */
#endif
	*Recover = "/usr/contrib/lib/jove/recover",
#ifdef	MSDOS
	CmdDb[FILESIZE] = "/usr/contrib/lib/jove/cmds.doc",
		/* copy of "cmds.doc" lives in the doc subdirectory */
#else
	*CmdDb = "/usr/contrib/lib/jove/cmds.doc",
		/* copy of "cmds.doc" lives in the doc subdirectory */
#endif

	*Joverc = "/usr/contrib/lib/jove/system.rc",

#if defined(IPROCS) && defined(PIPEPROCS)
	*Portsrv = "/usr/contrib/lib/jove/portsrv",
#endif

/* these are variables that can be set with the set command, so they are
   allocated more memory than they actually need for the defaults */

	TmpFilePath[FILESIZE] = "/var/tmp";

#ifdef	SUBSHELL
char
	Shell[FILESIZE] = "/bin/csh",
	ShFlags[16] = "-c";
#endif

#endif	/* !MAC */
