/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`do_uuxqt' - real subroutine to execute commands.
**
**	RCSID $Id: do_uuxqt.c,v 1.3 1994/01/31 01:27:23 donn Exp $
**
**	$Log: do_uuxqt.c,v $
 * Revision 1.3  1994/01/31  01:27:23  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:59  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:43:09  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:41:02  pace
 * Execute uuxqthook at end of uuxqt; don't think that LCK files are names of
 * systems; fix some malloc size bugs
 *
 * Revision 1.1.1.1  1992/09/28  20:09:04  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	NDIR
#define	PARAMS
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYS_STAT

#include	"global.h"
#include	"uuxqt.h"

/*
**	Lock codes.
*/

typedef enum
{
	locked, lockfail, lockmax
}
		Lcode;

/*
**	Legal commands list.
*/

typedef enum
{
	yes, never, on_error
}
		Notify;

typedef struct Cmd	Cmd;
struct Cmd
{
	Cmd *	next;
	char *	cmd;
	Notify	notify;
};

static Cmd *	Head;

static char *	DfltCmds[] =
{
	"rmail",
	"rnews",
	"ruusend",
	NULLSTR
};

#define	RMAIL	DfltCmds[0]
#define	RNEWS	DfltCmds[1]

/*
**	Execute files list.
*/

typedef struct XFile	XFile;
struct XFile
{
	XFile *	next;
	char	file[PATHNAMESIZE];
	char	tmp[PATHNAMESIZE];
	bool	tmpmade;
};

static XFile *	XFiles;
static XFile *	XFreelist;

/*
**	Lookup table for node names.
*/

#define	HASH_SIZE	67

typedef struct Entry	Entry;
struct Entry
{
	Entry *	great;
	Entry *	less;
	char	name[NODENAMEMAXSIZE+1];
};

static Entry *	NodeTable[HASH_SIZE];

/*
**	Environment for commands.
*/

static char	Home[PATHNAMESIZE]	= "HOME=";
#define		HOMEP				5
static char	Machine[PATHNAMESIZE]	= "UU_MACHINE=";
#define		MACHINEP			11
static char	Path[PATHNAMESIZE]	= "PATH=/bin:/usr/bin:/usr/ucb";
#define		PATHP				5
static char	Shell[PATHNAMESIZE]	= "SHELL=";
#define		SHELLP				6

static char *	Xenvs[] = {
	Path,
	Shell,
	Home,
	Machine,
	NULLSTR
};

/*
**	Per-node lockfiles.
*/

static char	LockName[]	= X_LOCK;
static char	LockFmt[]	= "%s.%s";
static char	LockNFmt[]	= "%s.%d";
static char	Ilockfile[LOCKNAMESIZE+1];			/* This process */
static char	Nlockfile[LOCKNAMESIZE+NODENAMEMAXSIZE+1];	/* This node */

/*
**	Miscellaneous definitions.
*/

#define	FILEACTIVETIME	2		/* File must be this old before `received' */

static char	IllegalChars[]	= ";&^|(`'<>{}\\\n\"";
char		MaxGrade = '\177';	/* Used by FindFile() */
static bool	NoClean;		/* Set to prevent unlinks on error exit */
static bool	StartUusched;

char *		SourceAddress;		/* Used by ExpandArgs() */
char *		SenderName;		/* Used by ExpandArgs() */

static char *	notify1;
static char *	notify2;
static void	notify_body(FILE *);

static char *	run_fin;
static void	run_setup(VarArgs *);

#if	DEBUG
extern int	WorkFiles;
#endif	/* DEBUG */

static void	addcmd(char *);
static void	addxfile(char *, char *);
static bool	canread(char *);
static bool	canwrite(char *);
static void	catch(int);
static bool	checkarg(Cmd **, char *);
static bool	enter(char *);
static bool	filesrecvd(char *, char *);
static void	getlegalcmds(void);
static bool	get_cmds(char *, char **, char *, char *, char **, bool *, bool *);
static void	ignore(int);
static bool	mvxfiles(char *, char *);
static void	mv_corrupt(char *, char *);
static void	notify(char *, char *);
static void	rmxfiles(bool);
static char *	run(char *, char *, int);
static void	scan_nodes(void);
static void	sig_catch(int);
static void	test_nologin(void);
static Lcode	x_node(void);
static Lcode	x_lock(void);
static void	x_unlock(void);

/*
**	The real thing.
*/

void
do_uuxqt()
{
	if ( Debugflag )
		MesgN("START", NULLSTR);

	Trace((2, "do_uuxqt()"));

	ignore(SIGHUP);
	ignore(SIGINT);
	catch(SIGTERM);

	/** Ensure XQTDIR exists **/

	if ( access(XQTDIR, 0) == SYSERROR && !CheckDirs(XQTDIR) )
		finish(EX_UNAVAILABLE);

	/*
	**	Command processing.
	*/

	getlegalcmds();

	if ( XqtNode == NULLSTR )
		scan_nodes();
	else
		(void)x_node();

	Trace((2, "Used %d work files", WorkFiles));
	if ( Debugflag )
		MesgN("FINISH", NULLSTR);

	if ( StartUusched )
		Uusched();
		/** NO RETURN **/
}

/*
**	Add legal command to list.
*/

static void
addcmd(
	char *		cmd
)
{
	register char *	cp;
	register Cmd *	cmdp;

	Trace((5, "addcmd(%s)", cmd));

	cmdp = Talloc(Cmd);
	cmdp->next = Head;
	Head = cmdp;

	if ( (cp = strchr(cmd, ',')) != NULLSTR )
	{
		*cp++ = '\0';
		if ( strnccmp(cp, "Err", 3) == STREQUAL )
			cmdp->notify = on_error;
		else
		if ( strccmp(cp, "No") == STREQUAL )
			cmdp->notify = never;
		else
			cmdp->notify = yes;
	}
	else
		cmdp->notify = yes;

	cmdp->cmd = newstr(cmd);

	Debug((1, "xcmd = %s", cmd));
}

/*
**	Add a data file for exec to list.
*/

static void
addxfile(
	char *		file,
	char *		file2
)
{
	register XFile *xfp;

	Trace((3, "addxfile(%s)", file));

	if ( (xfp = XFreelist) != (XFile *)0 )
		XFreelist = xfp->next;
	else
		xfp = Talloc(XFile);

	if ( file2 != NULLSTR )
		(void)strcpy(strcpyend(xfp->tmp, XQTDIR), file2);
	else
		xfp->tmp[0] = '\0';

	(void)strcpy(xfp->file, file);

	xfp->tmpmade = false;
	ExpFn(xfp->file);
	xfp->next = XFiles;
	XFiles = xfp;
}

/*
**	Check file has public read.
*/

static bool
canread(
	char *		file
)
{
	struct stat	statb;

	if ( stat(file, &statb) == SYSERROR )
		return true;

	/** Of course it may not be publicly readable if directories aren't. **/

	if ( statb.st_mode & R_OK )
		return true;

	return false;
}

/*
**	Check file (dir) has public write.
*/

static bool
canwrite(
	char *		file
)
{
	int		i;
	char *		cp;
	struct stat	statb;

	if ( stat(file, &statb) != SYSERROR )
	{
		if ( statb.st_mode & W_OK )
			return true;
		return false;
	}

	if ( (cp = strrchr(file, '/')) == NULLSTR )
		return false;

	*cp = '\0';

	i = stat(file, &statb);

	*cp = '/';

	if ( i == SYSERROR )
		return true;

	if ( statb.st_mode & W_OK )
		return true;

	return false;
}

/*
**	Cause signal to be caught.
*/

static void
catch(int sig)
{
	(void)signal(sig, sig_catch);
}

/*
**	Check arg for illegal constructs.
**	First also sets up command pointer.
*/

static bool
checkarg(
	Cmd **		cmd,
	char *		arg
)
{
	register Cmd *	cp;

	if ( strpbrk(arg, IllegalChars) != NULLSTR )
	{
		Warn("INVALID CHARACTER IN %s", arg);
		return false;
	}

	if ( *cmd != (Cmd *)0 )
		return true;

	for ( cp = Head ; cp != (Cmd *)0 ; cp = cp->next )
		if ( strcmp(cp->cmd, arg) == STREQUAL )
		{
			*cmd = cp;
			return true;
		}

	Debug((1, "UNRECOGNISED COMMAND: %s", arg));
	return false;
}

/*
**	Enter name in table, return true if new.
*/

static bool
enter(
	char *	name
)
{
	register char		c;
	register char *		cp1;
	register char *		cp2;
	register Entry **	epp;
	register Entry *	ep;

	for
	(
		epp = &NodeTable[HashName(name, HASH_SIZE)] ;
		(ep = *epp) != (Entry *)0 ;
	)
	{
		for ( cp1 = name, cp2 = ep->name ; c = *cp1++ ; )
			if ( c -= *cp2++ )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
		{
			Trace((4, "enter(%s) exists", name));
			return false;
		}

		if ( c > 0 )
			epp = &ep->great;
		else
			epp = &ep->less;
	}

	*epp = ep = Talloc0(Entry);

	(void)strncpy(ep->name, name, NODENAMEMAXSIZE);

	Trace((3, "enter(%s)", name));

	return true;
}

/*
**	Check all required files are present.
**
**	If not, and command file is old, move it to CORRUPTDIR.
*/

static bool
filesrecvd(
	char *		dir,
	char *		xfile
)
{
	register XFile *xfp;
	struct stat	statb;
	bool		allpresent = true;

	Trace((3, "filesrecvd(%s, %s)", dir, xfile));

	for ( xfp = XFiles ; xfp != (XFile *)0 ; xfp = xfp->next )
		if
		(
			stat(xfp->file, &statb) == SYSERROR
			||
			statb.st_mtime > (Time - FILEACTIVETIME)
		)
		{
			Debug((4, "not yet received: %s", xfp->file));
			allpresent = false;
			break;
		}

	if
	(
		!allpresent
		&&
		stat(xfile, &statb) != SYSERROR
		&&
		statb.st_mtime < (Time - CMDTIMEOUT)
	)
	{
		Warn("%s%s: MISSING FILES", dir, xfile);
		mv_corrupt(dir, xfile);
	}

	return allpresent;
}

/*
**	Cleanup for error routines.
*/

void
finish(
	int	error
)
{
	/** Unlink any files here **/

	if ( !ExInChild )
	{
		rmlock(NULLSTR);	/* All */

		if ( error && !NoClean )
			UnlinkAllWf();
	}

	if ( error && Debugflag ) 
		MesgN("ERROR", "code %d", error);

	(void)execl(UUXQTHOOK, NULL);	/* can fail, that's ok */

	(void)exit(error==SIGTERM ? EX_OK : error);
}

/*
**	Extract commands from commands file.
*/

static bool
get_cmds(
	char *		bufp,
	char **		cmd,
	char *		fin,
	char *		fout,
	char **		sysname,
	bool *		nonoti,
	bool *		nonzero
)
{
	register char *	cp;
	register char *	xp;
	register char *	zp;
	register char *	ap;
	bool		valid = true;

	Trace((3, "get_cmds()"));

	while ( (cp = GetLine(&bufp)) != NULLSTR )
	{
		Debug((5, "got cmd: %s", cp));

		if ( cp[1] != '\0' )
		{
			if ( cp[1] != ' ' )
				longjmp(ErrBuf, 1);

			for ( ap = &cp[2] ; *ap == ' ' ; ++ap );

			zp = &ap[strlen(ap)];
			while ( zp > ap && *--zp == ' ' )
				*zp = '\0';	/* Remove trailing spaces */

			if ( (xp = strchr(ap, ' ')) != NULLSTR )
			{
				zp = xp;	/* Remember space */
				while ( *++xp == ' ' );
				if ( *xp == '\0' )
					xp = NULLSTR;
			}
			else
				zp = NULLSTR;
		}
		else
			ap = xp = zp = NULLSTR;

		switch ( cp[0] )
		{
		default:	Debug((4, "ignore cmd: %s", cp));
				continue;

		case X_CMD:	*cmd = ap;
				break;
		case X_NONOTI:	*nonoti = true;;
				break;
		case X_NONZERO:	*nonzero = true;;
				break;
		case X_RETURNTO:UserName = ap;
				break;
		case X_RQDFILE:	if ( zp != NULLSTR )
					*zp = '\0';
				addxfile(ap, xp);
				break;
		case X_SENDFILE:
				break;
		case X_STDIN:	(void)strcpy(fin, ap);
				if
				(
					!ExpFn(fin)
					&&
					(
						!CheckUserPath(NULLSTR, NULLSTR, fin)
						||
						!canread(fin)
					)
				)
				{
					Warn("illegal input file: %s", fin);
					valid = false;
				}
				break;
		case X_STDOUT:	if ( zp != NULLSTR )
					*zp = '\0';
				*sysname = xp;
				(void)strcpy(fout, ap);
				if
				(
					(
						fout[0] == '~'
						&&
						strcmp(*sysname, NODENAME) != STREQUAL
					)
					||
					(
						!ExpFn(fout)
						&&
						(
							!CheckUserPath(NULLSTR, NULLSTR, fout)
							||
							!canwrite(fout)
						)
					)
				)
				{
					Warn("illegal output file: %s", fout);
					valid = false;
				}
				break;
		case X_USER:	if ( zp != NULLSTR )
					*zp = '\0';
				if ( xp != NULLSTR && strcmp(LogNode, xp) != STREQUAL )
				{
					LogClose();
					LogNode = xp;
					(void)strcpy(&Machine[MACHINEP], xp);
				}
				UserName = ap;
				break;
		}
	}

	return valid;
}

/*
**	Parse CMDSFILE for allowed commands.
*/

static void
getlegalcmds()
{
	register char *	cp;

	if ( Head != (Cmd *)0 )
		return;	/* Already setup */

	if ( (cp = ReadFile(CMDSFILE)) == NULLSTR )
	{
		char **	cpp;

		(void)SysWarn(CouldNot, ReadStr, CMDSFILE);

		/** Set up defaults **/

		for ( cpp = DfltCmds ; (cp = *cpp++) != NULLSTR ; )
			addcmd(cp);
	}
	else
	{
		char *		bufp;
		char *		data;
		static char	Space[]	= " \t\n";

		Debug((1, "%s opened", CMDSFILE));

		bufp = data = cp;

		while ( (cp = GetLine(&bufp)) != NULLSTR )
		{
			register char *	sp;

			Debug((8, "got cmd: %s", cp));

			if ( (sp = strpbrk(cp, Space)) != NULLSTR )
			{
				if ( sp[strspn(sp, Space)] != '\0' )
				{
					Warn("illegal command ignored in \"%s\": %s",
					     CMDSFILE, cp);
					continue;
				}

				*sp = '\0';	/* Remove trailing space */
			}

			if ( strncmp(cp, Path, PATHP) == STREQUAL )
			{
				(void)strcpy(&Path[PATHP], cp+PATHP);
				continue;
			}

			addcmd(cp);
		}

		free(data);
	}

	(void)strcpy(&Home[HOMEP], PUBDIR);
	(void)strcpy(&Shell[SHELLP], SHELL);

	NewEnvs = Xenvs;
}

/*
**	Ignore signal.
*/

static void
ignore(int sig)
{
	(void)signal(sig, SIG_IGN);
}

/*
**	Move suspect file to CORRUPTDIR.
*/

static void
mv_corrupt(
	char *	dir,
	char *	file
)
{
	char *	cp;
	char	temp[PATHNAMESIZE];

	if ( (cp = strrchr(file, '/')) != NULLSTR )
		++cp;
	else
		cp = file;

	(void)sprintf(temp, "%s%s", CORRUPTDIR, cp);

	if ( !MoveCp(file, temp) )
		(void)unlink(file);
	else
		Warn("corrupt file \"%s%s\" moved to %s", dir, file, temp);
}

static bool
mvxfiles(
	char *		dir,
	char *		xfile
)
{
	register XFile *xfp;
	char *		cp;
	bool		val = true;

	Trace((3, "mvxfiles(%s, %s)", dir, xfile));

	for ( xfp = XFiles ; xfp != (XFile *)0 ; xfp = xfp->next )
	{
		if ( xfp->tmp[0] == '\0' )
			continue;

		(void)unlink(xfp->tmp);
		if ( !MoveCp(xfp->file, xfp->tmp) )
		{
			Warn("Couldn't read workfile %s for %s%s", xfp->file, dir, xfile);
			val = false;
		}
		else
		{
			Debug((4, "mv %s %s", xfp->file, xfp->tmp));
			xfp->tmpmade = true;
		}
	}

	return val;
}

/*
**	Mail command status to originator.
*/

static void
notify(
	char *	cmd,
	char *	str
)
{
	char *	cp;
	char	buf[64];

	SourceAddress = XqtNode;
	SenderName = "uucp";

	notify1 = cmd;
	notify2 = str;

	if ( strpbrk(UserName, IllegalChars) != NULLSTR )
	{
		Log("%s - INVALID CHARACTER IN USERNAME %s", cmd, UserName);
		UserName = "postmaster";
	}

	if ( strcmp(XqtNode, NODENAME) == STREQUAL )
		cp = UserName;
	else
	{
		(void)sprintf(buf, "%s!%s", XqtNode, UserName);
		cp = buf;
	}

	Debug((1, "notify %s: cmd(%s), str(%s)", cp, notify1, notify2));

	Mail(notify_body, cp, NULLSTR);
}

static void
notify_body(
	FILE *	fd
)
{
	(void)fprintf(fd, "Subject: uuxqt cmd (%s) status (%s)\n\n", notify1, notify2);
}

/*
**	Unlink data files for exec.
*/

static void
rmxfiles(
	bool		orig
)
{
	register XFile *xfp;
	static char	unls[] = "unlink %s";

	Trace((1, "rmxfiles(%s)", orig?"orig":EmptyStr));

	while ( (xfp = XFiles) != (XFile *)0 )
	{
		if ( orig )
		{
			Debug((4, unls, xfp->file));
			(void)unlink(xfp->file);
		}

		if ( xfp->tmpmade )
		{
			Debug((4, unls, xfp->tmp));
			(void)unlink(xfp->tmp);
		}

		XFiles = xfp->next;
		xfp->next = XFreelist;
		XFreelist = xfp;
	}
}

/*
**	Run command.
*/

static char *
run(
	char *	cmd,
	char *	fin,
	int	ofd
)
{
	int	n;
	char *	cp;
	char *	errs;
	ExBuf	buf;

	Trace((1, "run(%s, %s, %d)", cmd, fin, ofd));

	ExStatus = 0;
	NARGS(&buf.ex_cmd) = 0;

	if ( SplitArgs(&buf.ex_cmd, cmd) > MAXVARARGS )
	{
		cp = newstr("too many args");
		goto out;
	}

	run_fin = fin;

	if ( (errs = Execute(&buf, run_setup, ex_exec, ofd)) != NULLSTR )
	{
		Trace((2, errs));

		cp = StripStatusString(errs);
		free(errs);

		/** Write status string to fout **/

		(void)write(ofd, cp, strlen(cp));
	}
	else
		cp = NULLSTR;

	(void)close(ofd);
out:
	for ( n = NARGS(&buf.ex_cmd) ; --n >= 0 ; )
		free(ARG(&buf.ex_cmd, n));

	return cp;
}

/*
**	called from new process after fork().
*/

static void
run_setup(
	VarArgs *	vap
)
{
	ignore(SIGQUIT);

	(void)alarm(3600);	/* Abort after an hour */

	(void)close(0);

	if ( open(run_fin, O_RDONLY) != 0 )
	{
		(void)SysWarn(CouldNot, ReadStr, run_fin);
		exit(EX_FDERR);
	}

	(void)chdir(XQTDIR);
}

/*
**	Scan all nodes for something to do.
*/

static void
scan_nodes()
{
	register struct direct *dp;
	register DIR *		dirp;
	char *			dir;
	char			lockfile[PATHNAMESIZE];

	static char		scfmt[]		= "scanning for work: %s%s";

	Trace((1, "scan_nodes()"));

	/* Establish lock */

	(void)sprintf(lockfile, LockFmt, LockName, "SCANNING");

	if ( !mklock(lockfile) )
	{
		Debug(("%s exists", lockfile));
		return;
	}

	/* Scan SPOOLALTDIRS for nodes */

	for ( dir = NULLSTR ; (dir = ChNodeDir(NULLSTR, dir)) != NULLSTR ; )
	{
		if ( (dirp = opendir(".")) == (DIR *)0 )
		{
			(void)SysWarn(CouldNot, ReadStr, dir);
			continue;
		}

		setproctitle(scfmt, dir, EmptyStr);

		while ( (dp = readdir(dirp)) != (struct direct *)0 )
		{
			int		l;
			Lcode		val;
			struct stat	statb;

			if ( dp->d_name[0] == '.' )
				continue;

			if ( strncmp(dp->d_name, LOCKPRE, strlen(LOCKPRE))
			     == STREQUAL
			    )
				continue;

			if ( (l = strlen(dp->d_name)) > NODENAMEMAXSIZE )
			{
				Debug((1, "name too long: %s", ExpandString(dp->d_name, l)));
				continue;
			}

			if ( SpoolAltDirs > 1 && !enter(dp->d_name) )
				continue;

			if ( stat(dp->d_name, &statb) == SYSERROR )
				continue;

			if ( (statb.st_mode & S_IFMT) != S_IFDIR )
				continue;

			Debug((2, scfmt, dir, dp->d_name));
			setproctitle(scfmt, dir, dp->d_name);

			XqtNode = dp->d_name;
			val = x_node();

			setproctitle(scfmt, dir, EmptyStr);

			if ( val == lockmax )
				break;	/* Too many running */

			/** Return from x_node() directory **/

			while ( chdir(dir) == SYSERROR )
				SysError(CouldNot, ChdirStr, dir);
		}

		closedir(dirp);
	}

	rmlock(lockfile);
}

/*
**	Catch a signal.
*/

static void
sig_catch(int sig)
{
	ignore(sig);
	finish(sig);
}

/*
**	Test if shutdown required.
*/

static void
test_nologin()
{
	if ( access(NOLOGIN, 0) == SYSERROR )
		return;

	if ( !Debugflag )
	{
		Log("UUXQT SHUTDOWN %s", NOLOGIN);
		finish(SIGTERM);
	}

	Debug((1, "%s: continuing anyway DEBUGGING", NOLOGIN));
}

/*
**	Create lock for `node'.
*/

static Lcode
x_lock()
{
	int	i;

	if ( MAXXQTS == 0 )
	{
		if ( mklock(LockName) )	/* One at a time */
			return locked;

		return lockfail;
	}

	/** Get lock for XqtNode **/

	(void)sprintf(Nlockfile, LockFmt, LockName, XqtNode);

	if ( !mklock(Nlockfile) )
	{
		Debug((1, CouldNot, LockStr, Nlockfile));
		return lockfail;
	}

	/** Get lock for this process **/

	for ( i = 1 ; i <= MAXXQTS ; i++ )
	{
		(void)sprintf(Ilockfile, LockNFmt, LockName, i);

		if ( mklock(Ilockfile) )
			return locked;
	}

	rmlock(Nlockfile);

	if ( NEEDUUXQT != NULLSTR )
		(void)close(creat(NEEDUUXQT, 0444));

	Debug((1, "Too many running"));
	return lockmax;
}

/*
**	Execute commands for `node'.
*/

static Lcode
x_node()
{
	register char *	cp;
	register char *	xp;
	char *		arg;
	char *		pcmd;
	Cmd *		cmdp;
	Lcode		lockval;

	char *		cmd;
	char		fin[PATHNAMESIZE];	/* Allow for expansion */
	char		fout[PATHNAMESIZE];	/* Allow for expansion */
	char *		sysname;

	bool		nonoti;
	bool		nonzero;
	bool		valid;

	char *		xdata;
	char *		xfile;
	char		prefix[4];

	Debug((1, "Rmtname set to %s", XqtNode));

	if ( (lockval = x_lock()) != locked )
		return lockval;

	if ( strcmp(LogNode, XqtNode) != STREQUAL )
	{
		LogClose();
		LogNode = XqtNode;
	}

	prefix[0] = EXEC_TYPE;
	prefix[1] = '.';
	prefix[2] = '\0';

	/*
	**	Step through directories for command files.
	*/

	InvokeDir = NULLSTR;

	while ( (InvokeDir = ChNodeDir(XqtNode, InvokeDir)) != NULLSTR )
	{
		while ( (xfile = FindFile(InvokeDir, prefix, ONEPASS)) != NULLSTR )
		{
			if ( strcmp(xfile, prefix) == STREQUAL )
				continue;	/* XXX seems unlikely */

			test_nologin();
			(void)SetTimes();	/* Update `Time' */

			while ( (cp = ReadFile(xfile)) == NULLSTR )
				if ( RdFileSize == 0 || !SysWarn(CouldNot, ReadStr, xfile) )
				{
					mv_corrupt(InvokeDir, xfile);
					break;
				}

			if ( (xdata = cp) == NULLSTR )
				continue;

			/** Set up defaults **/

			cmd = NULLSTR;
			(void)strcpy(fin, DevNull);
			(void)strcpy(fout, DevNull);
			sysname = NODENAME;
			UserName = Invoker;
			(void)strcpy(&Machine[MACHINEP], XqtNode);

			nonoti = false;
			nonzero = false;

			/** Parse command file contents **/

			if ( Recover(ert_here) )
			{
				Debug((1, "\"%s%s\" contains garbage", InvokeDir, xfile));
				mv_corrupt(InvokeDir, xfile);
				free(xdata);
				continue;
			}

			rmxfiles(false);

			valid = get_cmds(xdata, &cmd, fin, fout, &sysname, &nonoti, &nonzero);

			/** Parse command **/

			pcmd = xp = Malloc(RdFileSize+1024);
			arg = Malloc(RdFileSize+PATHNAMESIZE);
			cmdp = (Cmd *)0;

			if ( valid )
			for ( cp = cmd ; (cp = NextArg(cp, arg)) != NULLSTR ; )
			{
				if ( !(valid = checkarg(&cmdp, arg)) )
					break;
				if ( arg[0] == '~' )
					(void)ExpFn(arg);
				xp = strcpyend(xp, arg);
				*xp++ = ' ';
			}

			if ( !valid )
			{
				Warn("%s: XQT DENIED: %s", XqtNode, cmd);
				notify(cmd, "DENIED");
				rmxfiles(true);
				(void)unlink(xfile);
			}
			else
			if ( xp == pcmd )
			{
				Warn("\"%s%s\" has null command", InvokeDir, xfile);
corrupt:
				mv_corrupt(InvokeDir, xfile);
			}
			else
			if ( filesrecvd(InvokeDir, xfile) )
			{
				char *	dfile;
				char *	dpath;
				int	fd;

				*--xp = '\0';

				/** Execute command **/

				Log("%s XQT %s", UserName, pcmd);
				Debug((4, "cmd: %s <%s >%s", pcmd, fin, fout));

				if ( strcmp(fout, DevNull) == STREQUAL )
				{
					dfile = NULLSTR;
					dpath = DevNull;
					while ( (fd = open(dpath, O_WRONLY)) == SYSERROR )
						SysError(CouldNot, WriteStr, dpath);
				}
				else
				{
					dpath = UniqueWf(&dfile, DATA_TYPE, 'O', sysname, sysname);
					fd = CreateWf(dpath);
				}

				if ( !mvxfiles(InvokeDir, xfile) )
					goto corrupt;

				cp = run(pcmd, fin, fd);

				(void)unlink(xfile);	/* Committed */

				if ( strcmp(cmdp->cmd, RMAIL) == STREQUAL )
					nonoti = true;
				else
				if ( strcmp(cmdp->cmd, RNEWS) == STREQUAL )
					nonzero = true;

				if
				(
					!nonoti
					&&
					cmdp->notify != never
					&&
					(
						(xp = cp) != NULLSTR
						||
						(!nonzero && cmdp->notify == yes && (xp = "0"))
					)
				)
					notify(pcmd, xp);
				else
				if ( cp != NULLSTR && strcmp(cmdp->cmd, RMAIL) == STREQUAL )
				{
					Log("%s (%s) from %s!%s MAIL FAIL", pcmd, cp, XqtNode, UserName);
					notify(pcmd, cp);
				}

				if ( cp != NULLSTR )
				{
					Debug((1, "ERR exit cmd => %s", cp));
					free(cp);
				}

				rmxfiles(true);

				if ( strcmp(fout, DevNull) != STREQUAL )
				{
					if ( strcmp(sysname, NODENAME) == STREQUAL )
					{
						MoveCp(dpath, fout);
						(void)chmod(fout, FILE_MODE);
					}
					else
					{
						char *	cfile;
						char *	cpath;

						if ( (cp = strchr(UserName, '!')) != NULLSTR )
							++cp;
						else
							cp = UserName;

						cfile = WfName(CMD_TYPE, 'O', sysname);
						cpath = WfPath(cfile, CMD_TYPE, sysname);
						QueueCmd(cpath, "S %s %s %s - %s 0666\n", dfile, fout, cp, dfile);
						WriteCmds();

						StartUusched = true;
					}
				}

				WfFree();	/* ...file + ...path */
			}

			free(arg);
			free(pcmd);
			free(xdata);
		}
	}

	x_unlock();
}

/*
**	Undo `x_lock()'.
*/

static void
x_unlock()
{
	if ( MAXXQTS == 0 )
	{
		rmlock(LockName);
		return;
	}

	rmlock(Ilockfile);
	rmlock(Nlockfile);
}
