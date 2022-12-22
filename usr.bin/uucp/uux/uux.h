/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Header for uux common routines.
**
**	RCSID $Id: uux.h,v 1.1.1.1 1992/09/28 20:09:02 trent Exp $
**
**	$Log: uux.h,v $
 * Revision 1.1.1.1  1992/09/28  20:09:02  trent
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
extern char	Grade;			/* Job priority */
extern Ulong	GradeDelta;		/* Files larger than this degrade job priority */
extern bool	LinkFiles;		/* Make *real* link from spool to original files */
extern bool	LocalOnly;		/* Start uucico with `-L' (for local only) */
extern Ulong	MinInput;		/* Abort if less than this */
extern bool	NoMail;			/* Send no mail notification */
extern bool	NotifyOnFail;		/* Mail user only if command fails */
extern bool	PipeIn;			/* Read data from `stdin' (also `-') */
extern bool	StartJob;		/* Start uucico after queuing job */

/*
**	Command concatenation.
*/

extern int	CmdLen;
extern char *	CmdString;		/* The collected command */

/*
**	Miscellaneous definitions.
*/

extern char *	Invoker;		/* Invoker's name */
extern bool	NoClean;		/* Set to prevent unlinks on error exit */
extern bool	UUXReRun;		/* Set true if will be called > once */
extern char *	XqtNode;		/* Nodename of command */

/*
**	Common subroutines.
*/

extern void	do_uux();
extern void	uux_args(int, char **, char*);
