/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Definitions for using:
**		``(char *)Execute(ExBuf *, vFuncp, ExType, int)''
**	and:
**		``(char *)ExClose(ExBuf *, FILE *)''
**
**	Passed function is called from within child with one arg:-
**		a pointer to `ExBuf.ex_cmd' (execve() argument vector).
**
**	RCSID $Id: exec.h,v 1.1.1.1 1992/09/28 20:08:35 trent Exp $
**
**	$Log: exec.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:35  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

typedef struct
{
	VarArgs		ex_cmd;		/* Commands for exec() */
	int		(*ex_sig)();	/* Old signal value */
	int		ex_efd;		/* Error file descriptor */
	int		ex_pid;		/* Pid of command */
}
		ExBuf;

typedef enum
{
	ex_exec,			/* Don't make pipe, and "wait()" */
	ex_nowait,			/* Don't make pipe, and don't "wait()" */
	ex_nofork,			/* Don't fork, just exec */
	ex_pipe,			/* Make pipe, and return write end */
}
		ExType;

Extern bool	ExInChild;		/* True after a fork for child */
Extern int	ExStatus;		/* Byte-swapped `status' from wait() */

extern char *	Execute(ExBuf *, vFuncp, ExType, int);	/* Returns FILE * for ex_pipe */
extern char *	ExClose(ExBuf *, FILE *);		/* Does wait() after ex_pipe */
extern void	ExIgnSigs(void);			/* Ignore INT/HUP/QUIT for child */
extern char *	GetErrFile(VarArgs *, int, int);	/* Return string from stderr */
extern void	MakeErrFile(int *);			/* Create file for stderr output */
extern char **	StripEnv();				/* Sanitises environment */
