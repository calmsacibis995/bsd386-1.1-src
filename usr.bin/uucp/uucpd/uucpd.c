/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	TCP/IP login server for uucico.
**	Can be run stand-alone (-d) or via inetd.
**
**	$Log: uucpd.c,v $
 * Revision 1.4  1994/01/31  04:47:06  donn
 * Fix a typo of long standing.
 *
 * Revision 1.3  1994/01/31  01:27:06  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:45  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:43:00  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:37:44  pace
 * Get rid of compiler warnings.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:58  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/04/17  21:58:25  piers
 * bigger filler for children (so uucico can have longer ps description)
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

static char rcsid[]	= "$Id: uucpd.c,v 1.4 1994/01/31 04:47:06 donn Exp $";

#define	ARGS
#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	PASSWD
#define	PARAMS
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYSLOG
#define	SYS_WAIT
#define	TERMIOCTL
#define	TCP_IP

#include	"global.h"

#include	<utmp.h>
#if BSD4 < 4
#include	<lastlog.h>
#endif

#ifndef _PATH_WTMP
#define     _PATH_WTMP      "/var/log/wtmp"
#endif
#ifndef _PATH_LASTLOG
#define     _PATH_LASTLOG   "/var/log/lastlog"
#endif

extern char *	inet_ntoa(struct in_addr);
extern char *	crypt(const char *, const char *);

/*
**	Defaults:
*/

#define	BACKLOG		5		/* ``listen(2)'' parameter */
#define	MAXLOGINTRIES	4		/* How may attempts allowed */
#define	MAXPASSWORD	8		/* Maximum chars in a password */
#define	MAXUSERNAME	8		/* Maximum chars in a login name */
#define	MAX_ACCEPT_ERRS	5		/* Maximum system errors from accept(2) */
#define	TIMEOUT		60		/* Maximum seconds to wait for login */

/*
**	Parameters set from arguments.
*/

#if	NO_INETD
static bool	Daemon;			/* Run listener process */
#endif	/* NO_INETD */
char *		Name	= rcsid;	/* Program invoked name (rcsid => AVersion) */
int		LogLevel;		/* Log level */
int		Traceflag;		/* Trace level */
int		ULogLevel;		/* Uucico log level */

/*
**	Arguments.
**
**	NB: `inetd' may pass no arg 0!
*/

static Args	Usage[] =
{
	{STRING, OPTARG, {NONFLAG}, 0, (Pointer)0, getName},
#	if	NO_INETD
	Arg_bool(d, &Daemon, 0),
#	endif	/* NO_INETD */
	Arg_int(l, &LogLevel, getInt1, "log level", OPTARG|OPTVAL),
	Arg_string(P, 0, 0, "paramsfile", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_int(x, &ULogLevel, getInt1, "cico log level", OPTARG|OPTVAL),
	Arg_ignnomatch,			/* `inetd(8)' sometimes passes peer address */
	Arg_end
};

/**	Booleans are repeated because ignored parameters can't be elided	**/

static Args	PUsage[] =
{
	{STRING, OPTARG, {NONFLAG}, 0, (Pointer)0, (AFuncp)0},
#	if	NO_INETD
	Arg_bool(d, 0, 0),
#	endif	/* NO_INETD */
	Arg_bool(?, 0, 0),
	Arg_bool(\043, 0, 0),
	Arg_string(P, 0, getPARAMSFILE, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Environment for shells.
*/

#define		USERP					5
static char	User[MAXUSERNAME+USERP+1]	= "USER=";
static char	Filler[128];	/* For arg-list mods */

static char *	Xenvs[] = {
	User,
	Filler,
	NULLSTR
};

extern char **	environ;

/*
**	Miscellaneous definitions.
*/

int		Debugflag;		/* Debug level */
static bool	DoWtmp;			/* Can't write WtmpFile */
char *		Invoker;		/* used in Log() */
char *		UserName;		/* Used in Log() */
static char	WtmpFile[]	= _PATH_WTMP;
static char	Lastlog[]	= _PATH_LASTLOG;

#if	NO_INETD
bool		ExInChild;		/* Child process */
static char	LockName[]	= D_LOCK;
static bool	LockSet;		/* Server running */
static int	One		= 1;
jmp_buf		WaitJmp;
#endif	/* NO_INETD */

static struct sockaddr_in	MyAddr;
static struct sockaddr_in	TheirAddr;

#define	LOGON	SIGUSR1			/* Signal to increase log level */
#define	LOGOFF	SIGUSR2			/* Signal to disable logging */
#define	LOG(L,A)	if(LogLevel>=L)Log A


void	ignore(int);
void	inlog(int, char *, char *);
void	outlog(int, char *, char *);
void	service(void);
void	uucpd_args(int, char **);
void	waitchild(void);

#if	NO_INETD
void	logon(int);
void	logoff(int);
void	server(void);
#endif	/* NO_INETD */



int
main(
	int	argc,
	char *	argv[]
)
{
#	if	DEBUG
	(void)close(1);
	(void)dup(0);
	(void)close(2);
	(void)dup(0);
#	endif	/* DEBUG */

	uucpd_args(argc, argv);		/* Process all args */

#	if	NO_INETD
	if ( Daemon )
		server();
	else
#	endif	/* NO_INETD */
	{
		int	addrlen = sizeof(TheirAddr);

		while ( getpeername(0,
				    (struct sockaddr *)&TheirAddr,
				    &addrlen) == SYSERROR )
			SysError("getpeername");

		if ( DoWtmp )
			switch ( fork() )
			{
			case 0:
				Pid = getpid();
				break;
			case SYSERROR:
				SysError(ForkStr);
				finish(EX_OSERR);
			default:
				waitchild();
				finish(EX_OK);
			}

		service();
	}

	finish(EX_OK);
}


#if	NO_INETD
int
catch(
	int	sig
)
{
	ignore(sig);
	longjmp(WaitJmp, 1);
}


int
catchterm(
	int	sig
)
{
	ignore(sig);
	finish(EX_OK);
}
#endif	/* NO_INETD */


/*
**	Cleanup for error routines.
*/

void
finish(
	int	error
)
{
#	if	NO_INETD
	if ( !ExInChild && LockSet )
		rmlock(NULLSTR);
#	endif	/* NO_INETD */

	(void)exit(error);
}


void
ignore(
	int	sig
)
{
	(void)signal(sig, SIG_IGN);
}


/*
**	Write login records to "wtmp" and "lastlog".
*/

void
inlog(
	int		uid,
	char *		user,
	char *		host
)
{
	register int	fd;
	struct lastlog	ll;

	if ( !DoWtmp )
		return;

	Trace((1, "inlog(%d, %s, %s) Pid=%d", uid, user, host, Pid));

	outlog(Pid, user, host);

	if ( (fd = open(Lastlog, O_WRONLY)) == SYSERROR )
		return;

	if ( lseek(fd, (long)(uid * sizeof(ll)), SEEK_SET) != SYSERROR )
	{
		(void)time(&ll.ll_time);
		(void)strncpy(ll.ll_line, host, sizeof(ll.ll_line));
		(void)strncpy(ll.ll_host, host, sizeof(ll.ll_host));

		(void)write(fd, (char *)&ll, sizeof(ll));
	}

	(void)close(fd);
}

#if	NO_INETD
void
logon(
	int	sig
)
{
	(void)signal(LOGON, logon);
	LogLevel++;
	LOG(1, ("Logging enabled (%d)", LogLevel));
}

void
logoff(
	int	sig
)
{
	(void)signal(LOGOFF, logoff);
	LOG(1, ("Logging disabled"));
	LogLevel = 0;
}


/*
**	Disconnect `tty'.
*/

void
no_tty()
{
#	ifdef	TIOCNOTTY
	register int	fd;

	(void)signal(SIGALRM, ignore);
	(void)alarm(20);	/* Some kernels are ... */

	if ( (fd = open("/dev/tty", O_RDWR)) != SYSERROR )
	{
		(void)ioctl(fd, TIOCNOTTY, (void *)0);
		(void)close(fd);
	}

	ignore(SIGALRM);
	(void)alarm(0);
#	endif	/* TIOCNOTTY */

#	if	PGRP == 1
	if ( Pid != getpgrp(0) )
		(void)setpgrp(0, Pid);
#	endif	/* PGRP == 1 */
}
#endif	/* NO_INETD */


/*
**	Write log record to "wtmp".
*/

void
outlog(
	int		pid,
	char *		user,
	char *		host
)
{
	register int	fd;
	struct utmp	wtmp;

	Trace((1, "outlog(%d, %s, %s)", pid, user, host));

	if ( !DoWtmp )
		return;

	if ( (fd = open(WtmpFile, O_WRONLY|O_APPEND)) == SYSERROR )
		return;

	(void)sprintf(wtmp.ut_line, "uucp%.4d", pid);
	(void)strncpy(wtmp.ut_name, user, sizeof(wtmp.ut_name));
	(void)strncpy(wtmp.ut_host, host, sizeof(wtmp.ut_host));
	(void)time(&wtmp.ut_time);

	(void)write(fd, (char *)&wtmp, sizeof(wtmp));
	(void)close(fd);
}


/*
**	Read a line from fd 0.
*/

readline(
	char *		l,
	int		nc,
	char *		prompt,
	bool		passwd
)
{
	register int	n;
	register char *	p;
	char		c;
	static char	last_c;

	c = last_c;

	for ( ;; )
	{
		(void)write(1, prompt, strlen(prompt));

		p = l;
		n = nc;

		for ( ;; )
		{
			last_c = c;

			switch ( read(0, &c, 1) )
			{
			case 0:
				errno = EIO;		/* well ... */
			case SYSERROR:
				return SYSERROR;
			}

			switch ( c &= 0177 )
			{
			case '\0':
				continue;
			case '\n': case '\r': case '\004':
				*p = '\0';
				LOG(3, ("readline: \"%s\"", ExpandString(l, p-l)));
				if ( p > l )
				{
					last_c = c;
					return 0;
				}
				if ( c == '\n' && last_c == '\r' )
					continue;
				break;
			case '#': case '\b': case '\177': /* DEL */
				if ( p > l && n++ > 0 )
					p--;
				continue;
			case '@': case '\003': /* ^C */ case '\025': /* ^U */
				if ( p > l )
				{
					p = l;
					n = nc;
				}
				continue;
			default:
				if ( !passwd && (c < ' ' || c >= '\177') )
					continue;	/* ignore rubbish */
				if ( --n > 0 )		/* ignore overflow */
					*p++ = c;
				continue;
			}

			break;
		}
	}
}


#if	NO_INETD
/*
**	Set up a listener socket, and process requests.
*/

void
server()
{
	register int	s;
	register int	tcp_s;
	int		accept_errs;
	int		addrlen = sizeof(TheirAddr);
	struct servent *sp;

	(void)signal(LOGON, logon);
	(void)signal(LOGOFF, logoff);
	(void)signal(SIGTERM, catchterm);

	if ( (sp = getservbyname("uucp", "tcp")) == (struct servent *)0 )
	{
		ErrVal = EX_UNAVAILABLE;
		Error("uucp/tcp service undefined");
	}

	switch ( fork() )
	{
	case 0:
		break;
	case SYSERROR:
		finish(EX_OSERR);
	default:
		exit(EX_OK);	/* Fork off parent */
	}

	Pid = getpid();

	no_tty();

	if ( !mklock(LockName) )
	{
		Warn("%s already running", Name);
		finish(EX_UNAVAILABLE);
	}

	LockSet = true;

	LOG(1, ("Starting"));

	MyAddr.sin_family = AF_INET;
	MyAddr.sin_port = sp->s_port;

	while ( (tcp_s = socket(AF_INET, SOCK_STREAM, 0)) == SYSERROR )
		SysError(CouldNot, "socket", "tcp");

	while ( setsockopt(tcp_s, SOL_SOCKET, SO_REUSEADDR, (char *)&One, sizeof(One)) == SYSERROR )
		SysError(CouldNot, "setsockopt", "REUSEADDR");

	while ( bind(tcp_s, (char *)&MyAddr, sizeof(MyAddr)) == SYSERROR )
		SysError(CouldNot, "bind", "my address");

	while ( listen(tcp_s, BACKLOG) == SYSERROR )
		SysError("listen");

#	ifdef	SIGCHLD
	(void)signal(SIGCHLD, waitchild);
#	endif	/* SIGCHLD */

	for ( accept_errs = 0 ;; accept_errs = 0 )
	{
#		ifndef	SIGCHLD
		waitchild();
#		endif	/* !SIGCHLD */

		while ( (s = accept(tcp_s, &TheirAddr, &addrlen)) == SYSERROR )
			if
			(
				!SysWarn(CouldNot, "accept", EmptyStr)
				&&
				++accept_errs > MAX_ACCEPT_ERRS
			)
				finish(EX_OSERR);

		switch ( fork() )
		{
		case SYSERROR:
			(void)SysWarn(CouldNot, ForkStr, EmptyStr);
			break;
		case 0:
			ExInChild = true;
			if ( s != 0 )
			{
				(void)close(0);
				(void)dup(s);
				(void)close(s);
			}
			if ( tcp_s != 0 )
				(void)close(tcp_s);
			Pid = getpid();
			service();
			exit(EX_OK);
		}

		(void)close(s);
	}
}
#endif	/* NO_INETD */


/*
**	Start shell on socket (fd0).
*/

void
service()
{
	struct hostent *hep;
	struct passwd *	pwp;
	int		count;

	char *		hostinet;
	char *		hostname;
	char		password[MAXPASSWORD+1];
	char		username[MAXUSERNAME+1];

	static char	loginerr[]	= "Login incorrect";
	static char	loginerrfmt[]	= "%s.\n";
	static char	loginerrlogfmt[]= "%s (%s@%s)";
	static char	loginp[]	= "login: ";
	static char	lreaderr[]	= "read error from %s: %s";
	static char	passwordp[]	= "Password: ";
	static char	readerr[]	= "%s read error.\n";

	(void)close(1);
	(void)dup(0);
	(void)close(2);
	(void)dup(0);

	hostinet = inet_ntoa(TheirAddr.sin_addr);

	if
	(
		(hep = gethostbyaddr
			(
				(char *)&TheirAddr.sin_addr,
				sizeof(TheirAddr.sin_addr),
				AF_INET
			)
		) == (struct hostent *)0
	)
		hostname = hostinet;
	else
		hostname = hep->h_name;

	LOG(1, ("connection from %s [%s]", hostname, hostinet));

	(void)alarm(TIMEOUT);

	for ( count = 0 ;; )
	{
		if ( ++count > MAXLOGINTRIES )
			exit(EX_DATAERR);

		if ( readline(username, sizeof(username), loginp, false) == SYSERROR )
		{
			(void)fprintf(stderr, readerr, loginp);
			LOG(1, (lreaderr, hostname, ErrnoStr(errno)));
			continue;
		}

		if
		(
			(pwp = getpwnam(username)) != (struct passwd *)0
			&&
			(pwp->pw_passwd == NULLSTR || pwp->pw_passwd[0] == '\0')
		)
			break;	/* No password for valid user */

		if ( readline(password, sizeof(password), passwordp, true) == SYSERROR )
		{
			(void)fprintf(stderr, readerr, passwordp);
			LOG(1, (lreaderr, hostname, ErrnoStr(errno)));
			continue;
		}

		if ( pwp == (struct passwd *)0 )
		{
			(void)crypt(password, password);	/* Take same time */
			LOG(1, (loginerrlogfmt, "Unknown user", username, hostname));
			(void)fprintf(stderr, loginerrfmt, loginerr);
			continue;
		}

		if ( strcmp(crypt(password, pwp->pw_passwd), pwp->pw_passwd) != STREQUAL )
		{
			LOG(1, (loginerrlogfmt, "Bad password", username, hostname));
			(void)fprintf(stderr, loginerrfmt, loginerr);
			continue;
		}

		break;
	}

	(void)alarm(0);

	Trace((
		1,
		"Login(%s) shell=%s, UUCICO=%s, UUCICO_ONLY=%d, PARAMSFILE=%s",
		username,
		pwp->pw_shell,
		UUCICO,
		UUCICO_ONLY,
		PARAMSFILE
	));

	(void)strcpy(&User[USERP], username);
	UserName = username;

	if ( (UUCICO_ONLY || TELNETD != NULLSTR) && strcmp(pwp->pw_shell, UUCICO) != STREQUAL ) 
	{
		if ( UUCICO_ONLY )
		{
			LOG(1, (loginerrlogfmt, "Invalid uucp login attempt", username, hostname));
			fprintf(stderr, loginerrfmt, loginerr);
			exit(EX_NOUSER);
		}

		LOG(1, ("Rlogin(%s) from %s [%s]", username, hostname, hostinet));

		(void)fprintf(stderr, loginerrfmt, loginerr);

		if ( DoWtmp )
		{
			/* Kill waiting parent */
			(void)kill(getppid(), SIGKILL);
		}
		(void)sleep(1);

		pwp->pw_shell = TELNETD;
	}
	else
	{
		LOG(1, ("Login(%s) from %s [%s]", username, hostname, hostinet));

		inlog(pwp->pw_uid, pwp->pw_name, hostname);

		while ( setgid(pwp->pw_gid) == SYSERROR )
			SysError("setgid");

		initgroups(pwp->pw_name, pwp->pw_gid);

		while ( chdir(pwp->pw_dir) == SYSERROR )
			SysError(CouldNot, ChdirStr, pwp->pw_dir);

		while ( setuid(pwp->pw_uid) == SYSERROR )
			SysError("setuid");
	}

	for ( ;; )
	{
		register char *	cp;
		register char **cpp;
		char *		args[3];

		if ( (cp = strrchr(pwp->pw_shell, '/')) == NULLSTR )
			cp = pwp->pw_shell;
		else
			++cp;

		cpp = args;

		if ( strcmp(pwp->pw_shell, UUCICO) == STREQUAL )
		{
			if ( NewParamsFile )
				*cpp++ = concat("-P", PARAMSFILE, NULLSTR);

			if ( ULogLevel )
			{
				(void)close(2);
				(void)dup(LogFd());
				*cpp++ = newprintf("-x%d", ULogLevel);
			}
		}

		*cpp = NULLSTR;

		(void)execl(pwp->pw_shell, cp, args[0], args[1], args[3]);

		SysError(CouldNot, "execl", pwp->pw_shell);
	}
}


/*
**	Argument processing.
*/

void
uucpd_args(
	int	argc,
	char *	argv[]
)
{
	register char *	cp;
	register int	i;

	DoArgs(argc, argv, PUsage);		/* Possible change to PARAMSFILE */

	InitParams();

	(void)NodeName();

	/*
	**	Process all args.
	*/

	DoArgs(argc, argv, Usage);

	LogNode = Name = "uucpd";		/* Override invoked name */
	Invoker = UUCPUSER;

	DoWtmp = (access(WtmpFile, W_OK) == SYSERROR) ? false : true;

	Trace((1, "DoWtmp => %d", DoWtmp));

	environ = Xenvs;

	OpenLog(Name, LOG_PID, LOG_UUCP);

	for ( i = (sizeof Filler)-1, cp = Filler ; --i >= 0 ; )
		*cp++ = ' ';
}


void
waitchild()
{
	register int	pid;
	union wait	status;

#	if	!defined(WNOHANG) && NO_INETD
	if ( Daemon )
	{
		if ( setjmp(WaitJmp) )
			return;

		(void)signal(SIGALRM, catch);
		(void)alarm((unsigned)2);
	}
#	endif	/* !defined(WNOHANG) && NO_INETD */

	for ( ;; )
	{
#		if	defined(WNOHANG) && NO_INETD
		if ( Daemon )
			pid = wait3(&status, WNOHANG, 0);
		else
#		else	/* defined(WNOHANG) && NO_INETD */
			pid = wait((int *)&status);

#		if	NO_INETD
		if ( Daemon )
			(void)alarm((unsigned)0);
#		endif	/* NO_INETD */
#		endif	/* defined(WNOHANG) && NO_INETD */

		if ( pid == SYSERROR )
			return;

		LOG(2, ("pid %d exited", pid));

		outlog(pid, EmptyStr, EmptyStr);

		DODEBUG(sleep(2));
	}
}
