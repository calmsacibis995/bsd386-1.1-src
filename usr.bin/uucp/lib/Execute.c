/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Optional pipe(), fork(), execve() sequence with stdout selected.
**
**	RCSID $Id: Execute.c,v 1.1.1.1 1992/09/28 20:08:37 trent Exp $
**
**	$Log: Execute.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL
#define	RECOVER
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS

#include	"global.h"

#undef	Extern
#define	Extern
#include	"exec.h"

#define	TOCHILD		1
#define	CHILD0		0


static void	Exit(ExBuf *, char *, int);
static void	ExPath(VarArgs *, char *);
static void	ExShell(VarArgs *, char **);


char *
Execute(
	register ExBuf *buf,
	vFuncp		setup,
	ExType		type,
	int		ofd
)
{
	register int	i;
	register char *	prog;
	int		p[2];

	prog = ARG(&buf->ex_cmd, 0);	/* Allow "setup" to change invoked name */

	TraceT(1, (1, 
		"Execute(%s, %#lx, %s, %d)",
		prog,
		(Ulong)setup,
		(type==ex_exec)?"exec"
			:(type==ex_nowait)?"nowait"
			:(type==ex_nofork)?"nofork"
			:"pipe",
		ofd
	));

	DODEBUG(if(NARGS(&buf->ex_cmd)>MAXVARARGS)Fatal("MAXVARARGS"));

	ARG(&buf->ex_cmd, NARGS(&buf->ex_cmd)) = NULLSTR;

	if ( type == ex_nofork )
	{
		buf->ex_efd = fileno(ErrorFd);
		type = ex_exec;
		goto skip_fork;
	}

	if ( type == ex_pipe )
		while ( pipe(p) == SYSERROR )
			SysError(CouldNot, PipeStr, prog);

	MakeErrFile(&buf->ex_efd);

	for ( ;; )
	{
		switch ( buf->ex_pid = fork() )
		{
		case SYSERROR:
			SysError(CouldNot, ForkStr, prog);
			continue;

		case 0:
			ExInChild = true;
			Recover(ert_exit);
skip_fork:
#			if	DEBUG
			Name = prog;
			if ( Traceflag && !ErrorTty((int*)0) )
				EchoArgs(NARGS(&buf->ex_cmd)-1, &ARG(&buf->ex_cmd, 1));
#			endif	/* DEBUG */

			while ( buf->ex_efd < 2 )
				if ( (buf->ex_efd = dup(buf->ex_efd)) == SYSERROR )
					Exit(buf, DupStr, 2);

			if ( ofd != SYSERROR && ofd < 1 )
				if ( (ofd = dup(ofd)) == SYSERROR )
					Exit(buf, DupStr, 1);

			if
			(
				(type == ex_exec || type == ex_nowait)
				&&
				(p[CHILD0] = open(DevNull, O_RDONLY)) == SYSERROR
			)
				Exit(buf, DevNull, 0);

			if ( p[CHILD0] != 0 )
			{
				(void)close(0);
				if ( dup(p[CHILD0]) != 0 )
					Exit(buf, DupStr, 0);
			}

			if ( ofd != SYSERROR )
			{
				if ( ofd != 1 )
				{
					(void)close(1);
					if ( dup(ofd) != 1 )
						Exit(buf, DupStr, 1);
				}
			}
			else
			{
				(void)close(1);
				if ( dup(buf->ex_efd) != 1 )
					Exit(buf, DupStr, 1);
			}

			if ( buf->ex_efd != 2 )
			{
				(void)close(2);
				if ( dup(buf->ex_efd) != 2 )
					Exit(buf, DupStr, 2);
			}

			for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

			if ( setup != NULLVFUNCP )
				(*setup)(&buf->ex_cmd);

			ExPath(&buf->ex_cmd, prog);	/* NO RETURN */
		}

		if ( type == ex_exec )
			return ExClose(buf, (FILE *)0);

		if ( type == ex_nowait )
			return NULLSTR;

		(void)close(p[CHILD0]);

		buf->ex_sig = (Funcp)signal(SIGPIPE, SIG_IGN);

		return (char *)fdopen(p[TOCHILD], "w");
	}
}



char *
ExClose(
	register ExBuf *buf,
	FILE *		fd
)
{
	register int	i;
	int		status;
#	if	DEBUG >= 1
	char *		errfile;
#	endif	/* DEBUG >= 1 */

	TraceT(2, (2, "ExClose(%s, %d)", ARG(&buf->ex_cmd, 0), (fd==0)?-1:fileno(fd)));

	if ( fd != (FILE *)0 )
	{
		(void)fclose(fd);
		(void)signal(SIGPIPE, (void *) buf->ex_sig);
	}

	while ( (i = wait(&status)) != buf->ex_pid )
		if ( i == SYSERROR )
		{
			SysError("Lost child");
			return NULLSTR;
		}

	ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);

	if ( ErrorTty((int *)0) && (status & 0x80) == 0 )
		return status?newstr(NULLSTR):NULLSTR;

	if ( status )
		return GetErrFile(&buf->ex_cmd, status, buf->ex_efd);

#	if	DEBUG >= 1
	if
	(
		Traceflag > 0
		&&
		(errfile = GetErrFile(&buf->ex_cmd, 0, buf->ex_efd)) != NULLSTR
	)
	{
		TraceT(1, (1, ExpandString(errfile, -1)));
		free(errfile);
	}
	else
#	endif	/* DEBUG >= 1 */

	if ( buf->ex_efd != fileno(ErrorFd) )
		(void)close(buf->ex_efd);

	return NULLSTR;
}



void
ExIgnSigs()
{
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
}



static void
Exit(
	ExBuf *	buf,
	char *	reason,
	int	fd
)
{
	char	errs[256];

	(void)sprintf
	(
		errs,
		"%s: system error setting up FD %d: \"%s\" - %s\n",
		ARG(&buf->ex_cmd, 0),
		fd,
		reason,
		ErrnoStr(errno)
	);

	(void)write(buf->ex_efd, errs, strlen(errs));	/* `stderr' not yet set up */

	exit(fd + EX_FDERR);
}


static void
ExPath(
	VarArgs *	vap,
	char *		prog
)
{
	register char *	cp;
	register char *	np;
	char *		path;
	char **		ep = StripEnv(&path);
	char		progp[PATHNAMESIZE];

	for ( ;; )
	{
		if ( prog[0] == '/' )
			(void)execve(prog, &ARG(vap, 0), ep);
		else
			for ( cp = path ; cp != NULLSTR ; cp = np )
			{
				if ( (np = strchr(cp, ':')) != NULLSTR )
					*np = '\0';
				if ( *cp == '\0' )
					cp = ".";
				cp = strcpyend(progp, cp);
				*cp++ = '/';
				(void)strcpy(cp, prog);
				if ( np != NULLSTR )
					*np++ = ':';
				if ( access(progp, X_OK) == SYSERROR )
					continue;
				(void)execve(progp, &ARG(vap, 0), ep);
				if ( errno == ENOEXEC )
					break;
			}

		if ( errno == ENOEXEC )
			ExShell(vap, ep);

		SysError(CouldNot, "execve", prog);
	}
}



static void
ExShell(
	VarArgs *	vap,
	char **		ep
)
{
	register char **cpp;
	register char **npp;
	char *		newargs[MAXVARARGS+2];

	npp = newargs;
	cpp = &ARG(vap, 0);

	*npp++ = SHELL;

	while ( (*npp++ = *cpp++) != NULLSTR );

	(void)execve(newargs[0], newargs, ep);
}
