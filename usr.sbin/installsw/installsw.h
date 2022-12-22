/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: installsw.h,v 1.3 1994/01/06 04:32:30 polk Exp $	*/

#include <sys/types.h>
#include <stdio.h>

#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif

#ifdef sun
extern char *strdup();
extern char *malloc();
extern char *ctime();
extern char *getenv();
extern char *strcpy();
extern time_t time();
#endif

#include "pathnames.h"

#define USE_PAX		1	/* 1 means use pax instead of tar by default */

#ifndef DEFAULT_MEDIA
#define DEFAULT_MEDIA	"cdrom"
#endif

#ifndef DEF_MOUNTPT
#define DEF_MOUNTPT	"/cdrom"
#endif

#ifndef DEF_ROOT
#define DEF_ROOT	"/"
#endif

#define PKG_VAR		"CORE_VAR"
#define DEFROOT_VAR	"/var"

#define F_NOFILE	'X'

#define T_NORMAL	'N'
#define T_COMPRESSED	'Z'
#define T_GZIPPED	'z'

#define P_REQUIRED	'R'
#define P_DESIRABLE	'D'
#define P_OPTIONAL	'O'

#define M_CDROM		1
#define M_TAPE		2

#define L_LOCAL		1
#define L_REMOTE	2

#define TMPFILE		"/var/tmp/pkgtmp"
#define TMPMAP		"/var/tmp/maptmp"

#define STATUS		"/tmp/status"
#define RSTATUS		"/tmp/rstatus"

#define CMD_TEST	_PATH_ECHO
#define DD_BS		"bs=10k"

/* arg strings to tar and pax should specify stdin as archive file */
#define TAR_ARGS	"-x -p -f -"
#define PAX_ARGS	"-r -p e"

#define TAPESLEEP	5	/* sleep for some seconds between tape ops */

#define PACKAGEDIR	"PACKAGES"
#define PACKAGEFILE	"PACKAGES"

#define MAXPACKAGES	32
struct package {
	int  file;	/* file number on the tape */
	char *name;	/* name of this package */
	long size;	/* size (in kbytes) */
	int  type;	/* normal or compressed */
	int  pref;	/* required, desirable, optional */
	char *root;	/* root of the tree */
	char *desc;	/* nice printable name of the package */
	int  present;	/* 1 -> probably already installed */
	char *sentinel;	/* existence -> this package is installed */
	int  selected;	/* 1 -> chosen to install */
};
extern struct package pkgs[MAXPACKAGES];
extern int numpkgs;

extern char *progname;
extern int  errno;

#define LINELEN 512

#define COMMAND0 18
#define COMMAND1 (COMMAND0+1)
#define COMMAND2 (COMMAND1+1)
#define COMMAND3 (COMMAND2+1)
#define COMMAND4 (COMMAND3+1)
#define WARNLINE 23
