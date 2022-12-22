/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	C-library definitions.
**
**	RCSID $Id: libc.h,v 1.1.1.1 1992/09/28 20:08:32 trent Exp $
**
**	$Log: libc.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:32  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

extern char *	strcpy(char *, char *);
extern char *	strcat(char *, char *);
extern long	atol(char *);
extern double	atof(char *);
extern char *	malloc(int);
extern int	strlen(char *);
extern int	strcmp(char *, char *);
extern char *	strchr(char *, char);
extern char *	strrchr(char *, char);

/*
**	Args-library definitions.
*/

extern char *	Malloc(int);
extern char *	strcpyend(char *, char *);
