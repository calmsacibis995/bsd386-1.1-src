/*	BSDI	$Id: scandir.h,v 1.3 1993/12/21 19:13:12 polk Exp $	*/

/***************************************************************************
 * This program is Copyright (C) 1986, 1987, 1988 by Jonathan Payne.  JOVE *
 * is provided to you without charge, and with no warranty.  You may give  *
 * away copies of JOVE, including sources, provided that this notice is    *
 * included in all the files.                                              *
 ***************************************************************************/

extern int
	alphacomp proto((UnivConstPtr, UnivConstPtr));

#ifdef __bsdi__
#define scandir Jscandir
#endif

extern int
	scandir proto((char *dir, char ***nmptr,
		int (*qualify) proto((char *)),
		int (*sorter)(UnivConstPtr, UnivConstPtr)));

extern void
	freedir proto((char * * *nmptr,int nentries));

#ifdef MSDOS
extern unsigned int	fmask;
#endif
