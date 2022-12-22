/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Header for uucp common routines.
**
**	RCSID $Id: uucp.h,v 1.1.1.1 1992/09/28 20:08:57 trent Exp $
**
**	$Log: uucp.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:57  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

/*
**	Parameters set from arguments.
*/

extern bool	CopyFiles;		/* Copy original files to spool */
extern bool	CreateDirs;		/* Create intermediate directories */
extern char	Grade;			/* Job priority */
extern Ulong	GradeDelta;		/* Files larger than this degrade job priority */
extern bool	JobID;			/* Output ASCII job ID on stdout */
extern Ulong	MinInput;		/* Abort if less than this */
extern bool	NoMail;			/* Send no mail notification */
extern char *	RmtNode;		/* Remote node for file copy */
extern char *	RmtUser;		/* User to notify on reception */
extern bool	StartJob;		/* Start uucico after queuing job */

/*
**	File collection.
*/

extern int	FileCount;
extern char **	Files;			/* The files (last is dest) */

/*
**	Miscellaneous definitions.
*/

extern bool	UUCPReRun;		/* Set true if will be called > once */
extern char *	HomeDir;		/* Invoker's path */
extern int	LastOptn;		/* Index for `Optns' */
extern char *	Invoker;		/* Invoker's name */
extern char	Optns[];		/* Command options */

/*
**	Common subroutines.
*/

extern void	do_uucp();
extern void	uucp_args(int, char **, char*);
