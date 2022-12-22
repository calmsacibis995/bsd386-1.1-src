/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Control subroutine for `uucico'.
**
**	RCSID $Id: do_cico.c,v 1.5 1994/01/31 01:26:47 donn Exp $
**
**	$Log: do_cico.c,v $
 * Revision 1.5  1994/01/31  01:26:47  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.4  1993/02/28  15:36:36  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.3  1993/01/07  18:37:56  sanders
 * moved logging of RemoteName until after we open the log file.
 *
 * Revision 1.2  1992/11/20  19:40:06  trent
 *         - a bug-fix to prevent uucico to go into infinite loops when
 *           unsuccessfully trying to send a uucp job and getting a response
 *           "CN2" (permission denied) from the remote end.  Our uucico does
 *           not delete the job on our end (thinking it's a temporary error)
 *           and tries to send the file again when it rescans for work.
 *           The code for this is found in uucico/Control.c somewhere around
 *           line 730.  See the context below.
 *
 *         - a hack to limit a uucico session to prevent a subscriber on
 *           our 900 lines (tty9*) from being able to request files after
 *           30 minutes of connect time.  This code is optional and is
 *           ifdef'ed as SPRINT_HACK.  It's the phone company's fault,
 *           really.
 *
 *         - We recently got a new MorningStar box on a Sparc for handing
 *           our X.25 lines.  In order to determine that a line is X.25
 *           instead of a normal tty or pty, we access teh environment
 *           variable passed to uucico named "X25_CALLED_ADDRESS" (which
 *           contains the DNIC).  We used to use an ioctl with MorningStar's
 *           older product for the Sequent.  some of the code deals with
 *           locking of ttyx* names to make our connect logs backward
 *           compatable with the Sequent.  I call this a "feature".
 *
 *         - the default tty name is now "notty" instead of "ttyp0".
 *
 * Revision 1.1.1.1  1992/09/28  20:08:56  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.7  1992/09/10  14:46:11  ziegast
 * Removed a debug line for successful startups.
 *
 * Revision 1.6  1992/08/04  13:57:03  ziegast
 * Fix comments.
 * Keep loop from being infinite.
 *
 * Revision 1.5  1992/05/25  09:03:53  ziegast
 * Read would return EIO and cause uucico to exit when reading
 * "Ssystem" from remote site.
 *
 * Revision 1.4  1992/05/19  11:07:21  ziegast
 * Rick moved the StartTime to it's proper place (don't charge them until
 * we get "startup").
 *
 * Revision 1.3  1992/04/22  22:19:59  piers
 * ACTIVEFILEs now have contents (ActFile).
 *
 * Revision 1.2  1992/04/17  21:55:40  piers
 * Is_a_tty starts off TRUE (a bug fix -- otherwise not RAW on calls)
 * StartTime exported (for use by name_prog())
 * fix to opening auditfiles if KeepAudit and remote enabled debug
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS
#define	CTYPE
#define	ERRNO
#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	PARAMS
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYS_STAT
#define	SYS_TIME
#define	TCP_IP
#define	TERMIOCTL
#define	TIME
#define	X25_USED

#include	"global.h"
#include	"cico.h"

/*
**	Defaults.
*/

#define	DFLT_DEBUG_LEVEL	30	/* If set by signal */
#define	LEGAL_NODE_CHARS	".+-_$"	/* +[A-Z][a-z][0-9] */
#define	MAXRMTARGS		10	/* Args from remote */

/*
**	Remote parameters.
*/

bool		Restart;		/* RESTART option supported */
int		Seq;			/* Initial packet sequence number */
bool		SizeLimits;		/* Taylor UUCP extension */
long		Ulimit;			/* Max. 512 byte blocks / file */

static AFuncv	getGrade(PassVal, Pointer, char *);	/* Get remote grade request */
static AFuncv	getDebug(PassVal, Pointer, char *);	/* Get remote debug level */

static Args	RemoteUsage[] =
{
	Arg_any(&RemoteNode, 0, NULLSTR, 0),		/* First must be nodename */
	Arg_bool(N, &SizeLimits, 0),
	Arg_bool(R, &Restart, 0),
	Arg_string(p, 0, getGrade, NULLSTR, OPTARG),
	Arg_string(v, 0, getGrade, NULLSTR, OPTARG),
	Arg_int(Q, &Seq, 0, NULLSTR, OPTARG),
	Arg_number(U, &Ulimit, 0, NULLSTR, OPTARG),
	Arg_int(x, 0, getDebug, NULLSTR, OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Baud rates vs. kernel speed settings.
*/

Speed		Speeds[] =
{
#ifdef	B50
	{   50,	 B50},
#endif
#ifdef	B75
	{   75,	 B75},
#endif
#ifdef	B110
	{  110,	B110},
#endif
#ifdef	B150
	{  150,	B150},
#endif
#ifdef	B200
	{  200,	B200},
#endif
#ifdef	B300
	{  300,	 B300},
#endif
#ifdef	B600
	{ 600,	B600},
#endif
#ifdef	B1200
	{ 1200,	B1200},
#endif
#ifdef	B1800
	{ 1800,	B1800},
#endif
#ifdef	B2000
	{ 2000,	B2000},
#endif
#ifdef	B2400
	{ 2400,	B2400},
#endif
#ifdef	B3600
	{ 3600,	B3600},
#endif
#ifdef	B4800
	{ 4800,	B4800},
#endif
#ifdef	B7200
	{ 7200,	B7200},
#endif
#ifdef	B9600
	{ 9600,	B9600},
#endif
#ifdef	B19200
	{19200,	B19200},
#endif
#ifdef	B38400
	{38400,	B38400},
#endif
#ifdef	EXTA
	{19200,	EXTA},
#endif
#ifdef	EXTB
	{38400,	EXTB},
#endif
	{ 0,	0}
};

Ulong		Baud;			/* Bits/second for VC */

/*
**	Audit file setup.
*/

typedef enum
{
	AUDIT_CLEAN,	/* Cleanup - unlink temp. files */
	AUDIT_TEMP,	/* Open a temp. file `AUDITDIR/pid' */
	AUDIT_PERM	/* Open a perm. file `AUDITDIR/RemoteNode' */
}
		Audit;

/*
**	Map CallType to StatusType:
*/

static char	BADSEQ[]	= "BAD SEQUENCE";

static struct
{
	StatusType	type;
	char *		text;
}
	Stat_Call_Map[] =
{
	SS_OK,		EmptyStr,		/* <unused> */
	SS_OK,		"BAD SYSTEM",		/* CF_SYSTEM */
	SS_WRONGTIME,	"WRONG TIME TO CALL",	/* CF_TIME */
	SS_OK,		"SYSTEM LOCKED",	/* CF_LOCK */
	SS_NODEVICE,	"NO DEVICE",		/* CF_NODEV */
	SS_FAIL,	"CALL FAILED",		/* CF_DIAL */
	SS_FAIL,	"LOGIN FAILED",		/* CF_LOGIN */
	SS_FAIL,	"CHAT FAILED",		/* CF_CHAT */
	SS_BADSEQ,	BADSEQ			/* <UNKNOWN> */
};

/*
**	Miscellaneous definitions.
*/

static char	MesgBuf[MAX_MESG_CHARS];	/* For negotiations */
char		FailMsg[MAX_MESG_CHARS];	/* For failure reasons from ACU library */

#ifdef	USE_X25
#define MAX_X25_VC 40		/* Maximum Tty # one  can use */
#define X25LOCKPRE "ttyx"

static int	DNIC = 0;	/* X.25 Data Network Identification Code */
extern char *	DNIC_STR;	/* Environment variable retrieved at startup */

static char *	X25lock(void);
static int	X25unlock(void);
#endif	/* USE_X25 */

#if	USE_TERMIOS
static struct termios	SaveTty;
#else	/* USE_TERMIOS */
static struct sgttyb	SaveTty;
#endif	/* USE_TERMIOS */

static char	CONVERS[]	= "CONVERSATION %s";
static char	EOTMSG[]	= "\04\n\04\n";
char		FAILED[]	= "FAILED";
static char	HANDSHAKE[]	= "HANDSHAKE %s %s";
char		MasterStr[]	= "MASTER";
char		SlaveStr[]	= "SLAVE";
static char	StatsMsg[]	= "%s (conversation complete %lu sent %lu received %lu bps)";

jmp_buf		AlarmJmp;		/* SIGALRM */
int		Ifd	= SYSERROR;	/* Input side of virtual circuit */
char *		IPHostName;		/* IP circuit remote address */
bool		IsTCP;			/* Virtual circuit via TCP/IP */
bool		IsX25;			/* Circuit is via X.25 */
bool		Is_a_tty	= true;	/* Virtual circuit has `tty' semantics */
static bool	LineSet;		/* Line types setup */
static bool	RemoteGrade;		/* Grade set remotely */
int		Ofd	= SYSERROR;	/* Output side of virtual circuit */
bool		RemoteDebug;		/* Debug enabled from remote */
Time_t		StartTime;		/* Comms. start time */
Stat		Stats;			/* Transmission statistics */
static bool	Terminating;		/* Winding up */
char *		TtyName;		/* Name of virtual circuit, if known */
char *		TtyDevice;		/* Name of virtual circuit device, if known */
char *		VCName	= "?";		/* Virtual cicuit name */

char *		SourceAddress;		/* Used by ExpandArgs() */
char *		SenderName;		/* Used by ExpandArgs() */

#ifdef SPRINT_HACK
bool		Is900;
Time_t		HangupTime;
#endif /* SPRINT_HACK */

extern void	abort(void);
static void	catchsig(int);
static void	check_access(char **, bool);
static void	cnnt_acct(void);
static void	ignore(int);
static bool	master_init(void);
static int	nullerror(char *);
static void	onintr(int);
static void	process(void);
static bool	readline(int);
static void	restoretty(void);
static void	setaudit(Audit);
static void	set_line_types(void);
static void	slave_init(void);
static void	timeout(int);
static void	test_nologin(void);
static void	toggle_trace(int);
static void	validate_node(void);

/*
**	The real thing.
*/

void
do_cico()
{
	int	fd;

#if	BSD4 >= 2
	(void)sigsetmask((long)0);
#endif	/* BSD4 >= 2 */

	onintr(SIGHUP);
	onintr(SIGINT);
	onintr(SIGQUIT);
	onintr(SIGTERM);
	onintr(SIGPIPE);

#ifdef	SIGTTOU
	ignore(SIGTTOU);
#endif
#ifdef	SIGTTIN
	ignore(SIGTTIN);
#endif
#ifdef	SIGTSTP
	ignore(SIGTSTP);
#endif

#ifdef	SIGUSR1
	(void)signal(SIGUSR1, toggle_trace);
#else	/* SIGUSR1 */
	(void)signal(SIGFPE, toggle_trace);
#endif	/* SIGUSR1 */

#ifdef	TIOCNOTTY
	if ( CurrentRole == MASTER )
	{
		(void)signal(SIGALRM, ignore);
		(void)alarm(3);	/* Some kernels are ... */
		if ( (fd = open("/dev/tty", O_RDWR)) != SYSERROR )
		{
			(void)ioctl(fd, TIOCNOTTY, (void *)0);
			(void)close(fd);
		}
		(void)alarm(0);
	}
#endif	/* TIOCNOTTY */

#if	PGRP == 1
	if ( Pid != getpgrp() )
		(void)setpgrp(0, Pid);
#endif	/* PGRP == 1 */

#if	BSD4 >= 3
	unsetenv("TZ");
#endif	/* BSD4 >= 3 */

	name_prog("starting");

	Ifd = Ofd = SYSERROR;

	if ( Debugflag )
		setaudit((CurrentRole==MASTER) ? AUDIT_PERM : AUDIT_TEMP);

	Debug((1, "Remote = %s", RemoteNode));
	Debug((1, "Pid = %d, login name = %s, uid = %d", Pid, LoginName, R_uid));

	if ( CurrentRole == SLAVE )
		slave_init();

	process();
}

/*
**	Catch signal and cleanup.
*/

static void
catchsig(int sig)
{
	static char	fmt[]	= "%sSIGNAL %d %s";

	ignore(sig);
	ignore(SIGHUP);
	ignore(SIGINT);
	ignore(SIGPIPE);

	if ( sig == SIGHUP || sig == SIGPIPE )
		Ofd = SYSERROR;

	if ( Terminating )
		return;
	Terminating = true;

	Log(fmt, "CAUGHT ", sig, FAILED);

	(void)Status(SS_FAIL, RemoteNode, fmt, EmptyStr, sig, FAILED);

	if ( Stats.bps_count == 0 )
		Stats.bps_count = 1;

	Log
	(
		StatsMsg, FAILED,
		Stats.byts_sent, Stats.byts_rcvd,
		Stats.bps_total/Stats.bps_count
	);

	finish(EX_OK);
}

/*
**	Check access against list of name[:line]... tuples.
*/

static void
check_access(
	char **		list,
	bool		matchnode
)
{
	register char **cpp;
	register char *	cp;
	register char *	tn;
	char *		ln;
	bool		user_ok;
	int		ulen;
	int		Ulen;

	TraceT(1, (1, "check_access(%s, %d)", (list==(char **)0||list[0]==NULLSTR)?NullStr:list[0], (int)matchnode));

	if ( (cpp = list) == (char **)0 || cpp[0] == NULLSTR )
		return;		/* Anything goes */

	ln = LoginName;

	ulen = strlen(ln);

	if ( ln[0] == 'U' )
	{
		++ln;
		Ulen = ulen-1;
	}
	else
		Ulen = 0;

	if ( matchnode && strncmp(ln, RemoteNode, Ulen?Ulen:ulen) == STREQUAL )
		return;	/* Is who it claims it is */

	if ( (tn = TtyName) == NULLSTR )
	{
		if ( IsTCP )
			tn = "tcp";
#ifdef	USE_X25
		else
		if ( IsX25 )
			tn = "x25";
#endif	/* USE_X25 */
		else
			tn = "???";
	}

	Debug((5, "check access for %s on %s", ln, tn));

	user_ok = false;

	while ( (cp = *cpp++) != NULLSTR )
	{
		register char *	tp;
		register int	len;

		if ( (tp = strchr(cp, ':')) != NULLSTR )
			len = tp-cp;
		else
			len = strlen(cp);

		Debug((5, "check %.*s <> %s", len, cp, ln));

		if
		(
			(ulen == len && strncmp(LoginName, cp, len) == STREQUAL)
			||
			(Ulen == len && strncmp(ln, cp, len) == STREQUAL)
		)
		{
			register char *	np;

			user_ok = true;

			if ( tp == NULLSTR )
				return;			/* Any line ok */

			do
			{
				if ( (np = strchr(++tp, ':')) != NULLSTR )
					len = np-tp;
				else
					len = strlen(tp);

				if ( len == 0 )
					return;		/* Any line ok */

				Debug((5, "check %.*s <> %s", len, tp, tn));

				if ( strncmp(tn, tp, len) == STREQUAL )
					return;		/* This line valid */
			}
			while
				( (tp = np) != NULLSTR );

			break;
		}
	}

	if ( matchnode )	/* Maybe LoginName is a valid alias */
	{
		char *	user;

		user = newstr(ln);

		if
		(
			FindSysEntry(&user, NEEDSYS, NEEDALIAS)
			&&
			strcmp(user, RemoteNode) == STREQUAL
		)
		{
			free(user);
			return;	/* Is who it claims it is */
		}

		free(user);
	}

	if ( !user_ok )
	{
		Warn("Unknown host: %s (logged in as %s)", RemoteNode, LoginName);
		(void)sprintf(MesgBuf, "You are unknown to me as `%s'", RemoteNode);
	}
	else
	{
		Warn("Illegal access: %s on %s (logged in as %s)", RemoteNode, tn, LoginName);
		(void)strcpy(MesgBuf, "You are not permitted access via this service");
	}

	OutMesg('R', MesgBuf, Ofd);

	if ( STRANGERSCMD != NULLSTR )
	{
		ExBuf		args;
		static char	qs[]	= "'%s'";

		FIRSTARG(&args.ex_cmd) = STRANGERSCMD;
		NEXTARG(&args.ex_cmd) = newprintf(qs, RemoteNode);
		NEXTARG(&args.ex_cmd) = newprintf(qs, LoginName);
		NEXTARG(&args.ex_cmd) = tn;

		(void)Execute(&args, NULLVFUNCP, ex_exec, SYSERROR);
	}

	finish(EX_OK);
}

/*
**	Connect time accounting record generation.
*/

static void
cnnt_acct()
{
	register FILE *		fp;
	register struct tm *	tp;
	register char *		cp;
	int			fd;
	Ulong			et;

	static char	fmt[]	= "%s %d %d%02d%02d %02d%02d %d %.1f %s %ld %ld";

	if ( CNNCT_ACCT_FILE == NULLSTR || StartTime == 0 )
		return;

	fd = CreateA(CNNCT_ACCT_FILE);

	(void)fchmod(fd, 0664);

	fp = fdopen(fd, "a");

	tp = localtime(&StartTime);

	if ( tp->tm_year >= 100 )
		tp->tm_year -= 100;	/* Take care of anno domini 2000 */

	(void)SetTimes();

	/* In case the clock is adjusted to before the StartTime */
	if ( Time < StartTime )
		et = 1;
	else
		et = Time - StartTime;

	if ( (cp = TtyName) == NULLSTR )
		cp = DFLTTTYNAME;

	(void)fprintf
	(
		fp, fmt,
		RemoteNode,
		InitialRole,
		tp->tm_year, tp->tm_mon+1, tp->tm_mday,
		tp->tm_hour, tp->tm_min,
		tp->tm_wday,
		(float)(et + 5) / 60.0,	/* to 1/10 min. */
		cp,
		Stats.byts_sent,
		Stats.byts_rcvd
	);

#ifdef	USE_X25
	if ( IsX25 )
		(void)fprintf(fp, " %x\n", DNIC>>16);
	else
#endif	/* USE_X25 */
	if ( IsTCP && IPHostName != NULLSTR )
		(void)fprintf(fp, " %s\n", IPHostName);
	else
		putc('\n', fp);

	if ( fclose(fp) == EOF )
		(void)SysWarn(CouldNot, WriteStr, CNNCT_ACCT_FILE);
}

/*
**	Create ACTIVEFILE if necessary.
*/

void
CreateActive(
	bool		create
)
{
	int		fd;
	char *		cp;
	static bool	created;
	static ActFile	data;
	extern char *	RealNodeName;	/* Imported from NodeName.c */

	Trace((1, "CreateActive(%s) [%s]", create?"create":"remove", ACTIVEFILE));

	if ( (cp = ACTIVEFILE) == NULLSTR || cp[0] == '\0' )
		return;

	if ( data.node[0] == '\0' )
		(void)sprintf
			(
				(char *)&data,
				"%-*d\n%-*.*s\n",
				sizeof(data.pid)-1, Pid,
				sizeof(data.node)-1, sizeof(data.node)-1, RealNodeName
			);

	if ( !created )
	{
		if ( !create )
			return;

		if ( (fd = CreateN(cp, true)) != SYSERROR )
		{
			created = true;
			(void)write(fd, (char *)&data, sizeof data);
			(void)close(fd);
		}
		else
			Debug((1, CouldNot, CreateStr, cp));

		return;
	}

	if ( unlink(cp) == SYSERROR )
		Debug((1, CouldNot, UnlinkStr, cp));

	created = false;
}

/*
**	Scan SPOOLALTDIRS to check for work for `RemoteNode'.
**
**	NB: `InvokeDir' left pointing to first directory with work in it.
*/

bool
find_work()
{
	char *	prefix	= WfName(CMD_TYPE, '\0', RemoteNode);

	/*
	**	Step through directories for command files.
	*/

	FreeStr(&InvokeDir);

	while ( (InvokeDir = ChNodeDir(RemoteNode, InvokeDir)) != NULLSTR )
		if ( FindFile(InvokeDir, prefix, CHECKFILE) != NULLSTR )
		{
			CreateActive(true);
			free(prefix);
			return true;
		}

	free(prefix);
	return false;
}

/*
**	Cleanup for error routines.
*/

void
finish(int error)
{
	if ( ExInChild )
		exit(error);

	ignore(SIGHUP);
	ignore(SIGINT);
	ignore(SIGPIPE);

	name_prog("finishing");

	CreateActive(false);

	(void)sleep(5);	/* Magic ? */

	CloseACU();

	if ( CurrentRole == SLAVE )
	{
		restoretty();

		if ( TtyDevice != NULLSTR && TtyDevice[0] == '/' )
			(void)chmod(TtyDevice, 0600);
	}

	rmlock(NULLSTR);	/* AFTER CloseACU()! */

#ifdef USE_X25
	if (IsX25)
		X25unlock();
#endif /* USE_X25 */

	Reenable();		/* Logins */

	if ( error )
		(void)Status(SS_FAIL, RemoteNode, CONVERS, FAILED);

	(void)Status(SS_CLOSE);

	cnnt_acct();

	if ( Ofd != SYSERROR )
	{
		if ( CurrentRole == MASTER )
			(void)write(Ofd, EOTMSG, sizeof EOTMSG);

		(void)close(Ifd);
		(void)close(Ofd);
	}

	if ( Stats.byts_rcvd != 0 )
		Uuxqt(RemoteNode);

	if ( error == EX_OK )
		Uuxqt(NULLSTR);

	DODEBUG(if(error&&Debugflag)MesgN("ERROR", "exit code %d", error));

	Debug((1, "finished"));
	setaudit(AUDIT_CLEAN);
	LogClose();

	exit(error);
}

/*
**	Get remote grade request.
*/

static AFuncv
getGrade(PassVal val, Pointer ptr, char * str)
{
	if ( strncmp(val.p, "grade=", 6) == STREQUAL )
		val.p += 6;

	MaxGrade = DefMaxGrade = *val.p;
	RemoteGrade = true;

	return ACCEPTARG;
}

/*
**	Get remote trace level.
*/

static AFuncv
getDebug(PassVal val, Pointer ptr, char * str)
{
	if ( Debugflag )
	{
		Debug((1, "Remote debug request ignored"));
		return ACCEPTARG;
	}

	if ( (Debugflag = val.l) <= 0 )
		Debugflag = 1;

	RemoteDebug = true;

	return ACCEPTARG;
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
**	Perform MASTER role.
*/

static bool
master_init()
{
	register char *	cp;
	register char *	sp;
	char		buf[ULONG_SIZE+2];
	static char	callmsg[]	= "%s (call to %s via %s%s%s)";

	test_nologin();

	ignore(SIGHUP);

	if ( !Status(SS_CALLING, RemoteNode, "CALLING") )
	{
		Debug((1, "LOCKED: call to %s", RemoteNode));
		return false;
	}

	name_prog("starting call");

	FailMsg[0] = '\0';

	Ofd = Ifd = Connect();	/* Uses RemoteNode, sets SysDetails */

	cp = EmptyStr;

	if ( Ifd < 0 )
	{
		if ( devSel[0] != '\0' )
		{
			Debug((3, "Set ttyn to `devSel' %s", devSel));
			TtyName = cp = devSel;
		}
	}
	else
	{
		set_line_types();
		if ( TtyName != NULLSTR )
			cp = TtyName;
		Debug((3, "ttyn set to %s", cp));
	}

	if ( cp != EmptyStr )
		sp = " ";
	else
		sp = EmptyStr;

	if ( Ofd < 0 )
	{
		StatusType	typ;

		if ( (typ = Stat_Call_Map[-Ofd].type) != SS_WRONGTIME )
		{
			Log(callmsg, FAILED, RemoteNode, LineType, sp, cp);

			if ( Ofd == (int)CF_DIAL && FailMsg[0] != '\0' )
				(void)Status(typ, RemoteNode, "CALL %s: %s", FAILED, FailMsg);
			else
				(void)Status(typ, RemoteNode, Stat_Call_Map[-Ofd].text);
		}
		else
			Debug((1, "call failed: %s", Stat_Call_Map[-Ofd].text));

		return false;
	}

	Log(callmsg, "SUCCEEDED", RemoteNode, LineType, sp, cp);

	InitialRole = MASTER;

	if ( !readline('S') )
	{
		Log("%s imsg 1", FAILED);
		(void)Status(SS_FAIL, RemoteNode, CONVERS, FAILED);
		return false;
	}

	/*
	**	MesgBuf should now contain "Shere=<node>".
	**
	**	Should we check <node>? XXX
	*/

	Seq = GetNxtSeq(RemoteNode);

	if ( DebugRemote )
	{
		(void)sprintf(buf, "-x%d", DebugRemote);
		cp = buf;
	}
	else
		cp = EmptyStr;

	if ( MaxGrade != '\177' )
	{
		Debug((2, "Max Grade this transfer is %c", MaxGrade));
		(void)sprintf(MesgBuf, "%s -Q%d -p%c -vgrade=%c %s", NODENAME, Seq, MaxGrade, MaxGrade, cp);
	}
	else
		(void)sprintf(MesgBuf, "%s -Q%d %s", NODENAME, Seq, cp);

	OutMesg('S', MesgBuf, Ofd);

	if ( !readline('R') )
	{
		UnlockSeq();
		Log("%s imsg 2", FAILED);
		(void)Status(SS_FAIL, RemoteNode, CONVERS, FAILED);
		return false;
	}

	if ( MesgBuf[1] == 'B' )
	{
		UnlockSeq();
		Log(HANDSHAKE, FAILED, BADSEQ);
		(void)Status(SS_BADSEQ, RemoteNode, BADSEQ);
		return false;
	}

	if ( strcmp(&MesgBuf[1], "OK") != STREQUAL )
	{
		UnlockSeq();
		if ( strcmp(&MesgBuf[1], "CB") == STREQUAL )
			cp = "AWAITING CALLBACK";
		else
			cp = HANDSHAKE;
		Log(cp, FAILED, EmptyStr);
		(void)Status(SS_INPROGRESS, RemoteNode, cp, FAILED, EmptyStr);
		return false;
	}

	CommitSeq();

	if ( InvokeDir == NULLSTR )
	{
		InvokeDir = ChNodeDir(RemoteNode, NULLSTR);
		CreateActive(true);
	}

	return true;
}

/*
**	Interface to `setproctitle(lib)'.
*/

void
name_prog(
	char *		desc
)
{
	static char *	vcname	= EmptyStr;
	static char *	address	= EmptyStr;

	if ( vcname == EmptyStr && LineSet )
	{
		vcname = VCName;

		if ( IsTCP )
			address = IPHostName;
#ifdef	USE_X25
		else
		if ( IsX25 )
			address = newprintf("%x", (int)(DNIC>>16));
#endif	/* USE_X25 */
	}

	if ( RemoteNode == NULLSTR )
		setproctitle(desc);
	else
		setproctitle(" %-9s %-5s %-16s %s", RemoteNode, vcname, desc, address);
}

/*
**	Dummy error print routine for remote args processing.
*/

static int
nullerror(
	char *	s
)
{
	return 0;
}

/*
**	Arrange to catch signal.
*/

static void
onintr(int sig)
{
	(void)signal(sig, catchsig);
}

/*
**	Main process loop.
*/

static void
process()
{
	Ulong		elapsed;
	Ulong		total;
	CallType	ok;
	char		tbuf[ISODATETIMESTRLEN];

	Trace((1, "process() RemoteNode=%s, CurrentRole=%d", RemoteNode, CurrentRole));

	StartTime = 0;	/* Don't charge until we get started */

	ignore(SIGQUIT);

	if ( CurrentRole == MASTER )
	{
		StatusType	typ;

		if ( (typ = CallOK(RemoteNode)) != SS_OK )
		{
			if ( typ != SS_INPROGRESS && typ != SS_WRONGTIME )
				Log("CAN NOT CALL SYSTEM STATUS");

			Debug((1, "CallOK failed"));
			return;
		}

		if ( !master_init() )
			return;

		LoginName = RemoteNode;
	}
	else
		ignore(SIGINT);	/* Caught in MASTER mode */

	Debug((1,
		"Rmtname %s, Role %s, Ifn - %d, Loginuser - %s, Time - %s",
		RemoteNode,
		(CurrentRole == MASTER) ? MasterStr : SlaveStr,
		Ifd,
		LoginName,
		ISODateTimeStr(Time, Timeusec, tbuf)
	));

	name_prog((CurrentRole == MASTER) ? MasterStr : SlaveStr);

	if ( setjmp(AlarmJmp) )
	{
		(void)Status(SS_FAIL, RemoteNode, CONVERS, FAILED);
		return;
	}

	(void)signal(SIGALRM, timeout);
	(void)alarm(IsTCP ? MAXMSGTIME*4 : MAXMSGTIME);

	ok = Startup(CurrentRole);

	(void)alarm(0);

	if ( ok != SUCCESS )
	{
		Log("%s (startup)", FAILED);
		(void)Status(SS_FAIL, RemoteNode, "STARTUP %s", FAILED);
		return;
	}

	{
		char *	cp;
		char	bps_msg[PATHNAMESIZE+16];
		char	grade_msg[9];
		char	proto_msg[12];
		char	tcp_msg[66];
		char	x25_msg[15];

		if ( (cp = TtyName) == NULLSTR )
			cp = DFLTTTYNAME;
		(void)sprintf(bps_msg, " %s %d bps", cp, Baud);

		if ( Protocol != 'g' )
			(void)sprintf(proto_msg, " %c protocol", Protocol);
		else
			proto_msg[0] = '\0';

		if ( MaxGrade != '\177' )
			(void)sprintf(grade_msg, " grade %c", MaxGrade);
		else
			grade_msg[0] = '\0';

#ifdef	USE_X25
		if ( IsX25 )
			(void)sprintf(x25_msg, " dnic %x", DNIC>>16);
		else
#endif	/* USE_X25 */
			x25_msg[0] = '\0';

		if ( IsTCP )
			(void)sprintf(tcp_msg, " tcp %s", IPHostName);
		else
			tcp_msg[0] = '\0';

		Log("OK (startup%s%s%s%s%s)", bps_msg, proto_msg, grade_msg, x25_msg, tcp_msg);
	}

	StartTime = Time;

#ifdef SPRINT_HACK
	if ( Is900 )
		HangupTime = StartTime + 30*60;	/* Must hangup after 30 minutes */
#endif /* SPRINT_HACK */

	(void)Status(SS_INPROGRESS, RemoteNode, "TALKING");

	ok = Control(CurrentRole);

	(void)alarm(0);
	ignore(SIGINT);
	ignore(SIGHUP);

	Debug((1, "cntrl - %d", (int)ok));

	name_prog("terminating");

	if ( Stats.bps_count == 0 )
		Stats.bps_count = 1;

	Log
	(
		StatsMsg, (ok==SUCCESS) ? "OK" : FAILED,
		Stats.byts_sent, Stats.byts_rcvd,
		Stats.bps_total/Stats.bps_count
	);

	if
	(
		(elapsed = Time - StartTime) > 60	/* Log() does SetTimes() */
		&&
		(total = Stats.byts_sent + Stats.byts_rcvd) / elapsed < 10
	)
		Warn("low throughput: %.1f minutes, %ld bytes", (float)elapsed/60.0, total);

	if ( ok == SUCCESS )
		(void)Status(SS_OK, RemoteNode, CONVERS, "COMPLETE");
	else
		(void)Status(SS_FAIL, RemoteNode, CONVERS, FAILED);

	Debug((4, "send OO %d", ok));

	Terminating = true;	/* Stop catchsig() whingeing */
	HaveSentHup = true;	/* Stop timeout() whingeing */

	if ( !setjmp(AlarmJmp) )
	{
		(void)signal(SIGALRM, timeout);
		(void)alarm(IsTCP ? MAXMSGTIME*4 : MAXMSGTIME);

		for ( ;; )
		{
			OutMesg('O', "OOOOO", Ofd);

			if ( !InMesg(sizeof MesgBuf, MesgBuf, Ifd) )
				break;

			if ( MesgBuf[0] == 'O' )
				break;
		}

		(void)alarm(0);
	}
}

/*
**	Read line from slave.
*/

static bool
readline(
	int	expectchar
)
{
	int errcnt=0;

	(void)signal(SIGALRM, timeout);

	if ( setjmp(AlarmJmp) )
		return false;	/* Read timed out */

	(void)alarm(IsTCP ? MAXMSGTIME*4 : MAXMSGTIME);

	for ( ;; )
	{
		if ( !InMesg(sizeof MesgBuf, MesgBuf, Ifd) )
		{
			int     saverr = errno;
 
			Debug((4, "\nInMesg failed: errno = %s", ErrnoStr(saverr)));
			if ( saverr == EIO && ++errcnt < 3 )
			{
				(void)sleep(1); /* Ignore these */
				continue;
			}

			(void)alarm(0);
			return false;
		}

		if ( MesgBuf[0] == expectchar )
			break;
	}

	(void)alarm(0);
	return true;
}

/*
**	Restore tty modes.
*/

static void
restoretty()
{
	if ( !Is_a_tty || Ifd == SYSERROR )
		return;

#ifdef	DROPDTR
#if	USE_TERMIOS

	SaveTty.c_oflag |= HUPCL;
	(void)tcsetattr(Ifd, TCSANOW, &SaveTty);

#else	/* USE_TERMIOS */

#ifdef	TIOCSDTR

	(void)ioctl(Ifd, TIOCCDTR, (void *)0);
	(void)sleep(2);
	(void)ioctl(Ifd, TIOCSDTR, (void *)0);

#else	/* TIOCSDTR */
	{
		struct sgttyb	ttyp;

		ttyp = SaveTty;
		ttyp.sg_ispeed = ttyp.sg_ospeed = 0;
		(void)ioctl(Ifd, TIOCSETP, &ttyp);
	}
#endif	/* TIOCSDTR */

	(void)sleep(2);
	(void)ioctl(Ifd, TIOCSETP, &SaveTty);

#endif	/* USE_TERMIOS */
#endif	/* DROPDTR */
}

/*
**	Setup debugging files.
*/

static void
setaudit(Audit type)
{
	register char *	cp;
	struct stat	statb;

	static bool	AuditOpen;
	static char *	AuditDir;
	static char *	AuditTemp;
	static char	time_stamp[ISODATETIMESTRLEN+1];

	Trace((1, "setaudit(%d)", (int)type));

	if ( KeepAudit && time_stamp[0] == '\0' )
	{
		time_stamp[0] = '.';
		ISODateTimeStr(Time, (Ulong)0, &time_stamp[1]);
		time_stamp[13] = '\0';	/* Discard usecs */

		if ( AuditOpen && AuditTemp == NULLSTR && RemoteNode != NULLSTR )
			AuditTemp = concat(AuditDir, RemoteNode, NULLSTR);
	}

	if ( type == AUDIT_PERM && AuditTemp != NULLSTR )
	{
		char *	AuditPerm;
			
		AuditPerm = concat(AuditDir, RemoteNode, time_stamp, NULLSTR);

		(void)unlink(AuditPerm); Link(AuditTemp, AuditPerm);

		free(AuditPerm);

		type = AUDIT_CLEAN; 
	}

	if ( type == AUDIT_CLEAN )
	{
		if ( AuditTemp != NULLSTR )
		{
			(void)unlink(AuditTemp);
			FreeStr(&AuditTemp);
		}

		return;
	}

	if ( Debugflag == 0 || AuditOpen )
		return;

	while ( CurrentRole == MASTER && !KeepAudit )
	{
		static bool	should_return;

		if ( should_return )
			return;

		if ( isatty(fileno(DebugFd)) )
		{
			should_return = true;
			return;
		}

		if ( fstat(fileno(DebugFd), &statb) == SYSERROR )
			break;

		if
		(
#if	SYSV > 0
			(statb.st_mode & 060000) == 0		/* Regular or FIFO file */
#else	/* SYSV > 0 */
#if	BSD4 >= 2
			(statb.st_mode & S_IFMT) == S_IFREG	/* Regular file */
			||
			statb.st_mode == 0			/* Pipe */
#else	/* BSD4 >= 2 */
			(statb.st_mode & S_IFMT) == S_IFREG	/* Regular file or pipe */
#endif	/* BSD4 >= 2 */
#endif	/* SYSV > 0 */
		)
		{
			should_return = true;
			return;
		}

		break;
	}

	if ( AuditDir == NULLSTR )
		AuditDir = concat(LOGDIR, "audit", Slash, NULLSTR);

	if ( stat(AuditDir, &statb) == SYSERROR )
	{
		Debug((1, CouldNot, StatStr, AuditDir));
		Debugflag = 0;		/* Debugging disabled */
		return;
	}

	if
	(
		statb.st_uid != NetUid			/* Don't own it */
		||
		(statb.st_mode & 0170700) != 040700	/* Not dir, mode rwx */
	)
	{
		Warn("%s: invalid directory mode: %o", AuditDir, statb.st_mode);
		Debugflag = 0;		/* Debugging disabled */
		return;
	}

	if ( AuditTemp != NULLSTR )
	{
		(void)unlink(AuditTemp);
		FreeStr(&AuditTemp);
	}

	if ( type == AUDIT_TEMP )
	{
		char	pidstr[ULONG_SIZE];

		(void)sprintf(pidstr, "%d", Pid);

		cp = concat(AuditDir, pidstr, NULLSTR);
		AuditTemp = newstr(cp);
	}
	else
	if ( RemoteNode == NULLSTR )
		return;
	else
		cp = concat(AuditDir, RemoteNode, time_stamp, NULLSTR);

	(void)unlink(cp);

	if ( (DebugFd = freopen(cp, "w", DebugFd)) == (FILE *)0 )
	{
		Debugflag = 0;		/* Debugging disabled */
		SysError(CouldNot, WriteStr, cp);
		free(cp);
		return;
	}

	AuditOpen = true;
	Debug((1, "DEBUG level %d", Debugflag));
	free(cp);
}

/*
**	Turn a `tty' line into true RAW mode,
**	and set `Baud'.
*/

void
SetRaw(
	int		fd,
#if USE_TERMIOS
	struct termios *saved
#else
	struct sgttyb *saved
#endif
)
{
	register Speed *sp;
#if USE_TERMIOS
	register speed_t speed;
	struct termios	tbuf;
	register Ulong	save_cflag;
#else
	register int	speed;
	struct sgttyb	tbuf;
#endif

	if ( !Is_a_tty )
	{
		Baud = DFLT_BAUD;
		return;
	}

	Debug((4, "SetRaw"));

	tbuf = *saved;

#if	USE_TERMIOS

	speed = cfgetispeed(&tbuf);
	save_cflag = tbuf.c_cflag & (CLOCAL|CRTS_IFLOW|CCTS_OFLOW|MDMBUF|CRTSCTS);
	tbuf.c_iflag = 0;
	tbuf.c_oflag = 0;
	tbuf.c_lflag = 0;
	tbuf.c_cflag = (CS8|CREAD|save_cflag);
	cfsetispeed(&tbuf, speed);
	cfsetospeed(&tbuf, speed);	/* we don't do split speeds */
	tbuf.c_cc[VMIN] = 6;
	tbuf.c_cc[VTIME] = 1;

	(void)tcsetattr(fd, TCSANOW, &tbuf);

#else	/* USE_TERMIOS */

	tbuf.sg_flags = ANYP|RAW;
	speed = tbuf.sg_ispeed;

	(void)ioctl(fd, TIOCSETP, &tbuf);

#endif	/* USE_TERMIOS */

	for ( sp = Speeds ; sp->speed ; sp++ )
		if ( sp->speed == speed )
		{
			Baud = sp->baud;
			Debug((1, "incoming baudrate is %d", Baud));
			return;
		}

#ifdef USE_TERMIOS
	Baud = speed;
	Debug((1, "guessing that incoming baudrate is %ld", Baud));
	return;
#else
	ErrVal = EX_DATAERR;
	Error("Unrecognised kernel tty speed: %d", speed);
#endif
}

/*
**	Determine what Ifd represents.
*/

static void
set_line_types()
{
	int		type;
	int		size;

	static char	conn[] = "tty connection [%s]";

	extern char *	ttyname(int);
	extern int	isatty(int);

	if ( (TtyDevice = ttyname(Ifd)) != NULLSTR )
	{
		if ( strncmp(TtyDevice, DevNull, 5) == STREQUAL )
			TtyName = &TtyDevice[5];
		else
			TtyName = TtyDevice;
		VCName = TtyName;
		Is_a_tty = true;
		Debug((4, conn, TtyDevice));
#ifdef SPRINT_HACK
		Is900 = (strncmp(TtyName, "tty9", 4) == STREQUAL);
#endif /* SPRINT_HACK */
	}
	else
	if ( isatty(Ifd) )
	{
		TtyName = NullStr;
		Is_a_tty = true;
		Debug((4, conn, TtyName));
	}
	else
	{
		TtyName = NULLSTR;
		Is_a_tty = false;
		Debug((4, "NOT a TTY -- ioctl-s disabled"));
	}

	/** VC via TCP/IP or X.25? **/

	IsTCP = false;
	IsX25 = false;
	size = sizeof(type);

	if ( getsockopt(Ifd, SOL_SOCKET, SO_TYPE, &type, &size) != SYSERROR )
	{
		IsTCP = true;
		VCName = "TCP";
		Debug((4, "TCP/IP connection"));
	}
#ifdef	USE_X25
	else
	if ( *DNIC_STR != 0)
	{
		if (sscanf(DNIC_STR, "%8x", &DNIC))
		{
			IsX25 = true;
			VCName = "X25";
			if (! (TtyName = X25lock()))
				Warn("Cannot get X25 lock\n");
		}
		else
			Warn("Improperly formatted DNIC: %s\n", DNIC_STR);
	}
#endif	/* USE_X25 */

	LineSet = true;
}

/*
**	Do initial checking for slave.
*/

static void
slave_init()
{
	int		type;
	int		size;
	int		argc;
	char *		argv[MAXRMTARGS];

	test_nologin();

	Ifd = 0;
	Ofd = 1;

	set_line_types();

	if (TtyDevice == NULLSTR)
		TtyDevice = TtyName = DFLTTTYNAME;

	if (Is_a_tty) {
#if USE_TERMIOS
		if (tcgetattr(Ifd, &SaveTty) == SYSERROR) {
			(void)SysWarn(CouldNot, "tcgetattr", TtyDevice);
			Is_a_tty = false;
		} else {
			SaveTty.c_cflag = (SaveTty.c_cflag & ~CS8) | CS7;
			SaveTty.c_oflag |= OPOST;
			SaveTty.c_oflag |= (ISIG|ICANON|ECHO);
		}
#else	/* USE_TERMIOS */ 
		if (ioctl(Ifd, TIOCGETP, &SaveTty) == SYSERROR) {
			(void)SysWarn(CouldNot, "TIOCGETP", TtyDevice);
			Is_a_tty = false;
		} else {
			SaveTty.sg_flags |= ECHO;
			SaveTty.sg_flags &= ~RAW;
		}
#endif	/* USE_TERMIOS */ 
	}

	SetRaw(Ifd, &SaveTty);

	(void)sprintf(MesgBuf, "here=%s", NODENAME);

	OutMesg('S', MesgBuf, Ofd);

	if (!readline('S')) {
		Log("startup failure");
		finish(EX_OK);
		return;
	}

	/** Process remote parameters **/

	RemoteNode = NULLSTR;

	if ((argc = SplitSpace(argv, &MesgBuf[1], MAXRMTARGS)) > 0)
		(void)EvalArgs(argc, argv, RemoteUsage, nullerror);

	validate_node();

	name_prog("startup");

	LogClose();
	LogNode = RemoteNode;

	/** Check for SYSFILE `SLAVE' entry **/

	if (FindSlaveGrade(Baud, VCName)
	    && (SysDebug > Debugflag
		|| (SysDebug && RemoteDebug)
		)
	    ) {
		RemoteDebug = false;

		if (SysDebug < 0) {
			Log("DEBUG %d remote request ignored", Debugflag);
			Debugflag = 0;
		} else {
			KeepAudit = SysKeepDebug;
			Debugflag = SysDebug;
			Debug((1, "DEBUG level %d", Debugflag));
			setaudit(AUDIT_PERM);
			Log("DEBUG %d L.sys enabled", Debugflag);
			if (Traceflag == 0)
				Traceflag = Debugflag;
		}
	} else if (RemoteDebug) {
		Log("DEBUG %d remote enabled", Debugflag);
		if (Traceflag == 0)
			Traceflag = Debugflag;
	} else if (Debugflag) {
		Log("DEBUG %d local enabled", Debugflag);
		if (Traceflag == 0 && Debugflag == 9)
			Traceflag = Debugflag;
	}

	if (Traceflag == 0)
		Traceflag = Debugflag;

	if (RemoteGrade)
		Debug((4, "MaxGrade set to %c", MaxGrade));

	if (!Status(SS_CALLING, RemoteNode, "CALLING")) {
		OutMesg('R', "LCK", Ofd);
		finish(EX_OK);
		return;
	}

	if (CallBack(LoginName))
	{
		int	fd;
		char	*name;
		char	*path;

		ignore(SIGHUP);
		ignore(SIGINT);

		OutMesg('R', "CB", Ofd);

		Log("REQUIRED CALLBACK");
		(void)Status(SS_CALLBACK, RemoteNode, "CALLING BACK");

		name = WfName(CMD_TYPE, 'C', RemoteNode);
		path = WfPath(name, CMD_TYPE, RemoteNode);

		if ((fd = CreateN(path, false)) != SYSERROR)
			(void)close(fd);

		(void)Uucico(RemoteNode, NOWAIT);
		finish(EX_OK);
		return;
	}

	/** XXX What if not SS_OK ? **/

	if (CallOK(RemoteNode) == SS_BADSEQ) {
		/** XXX THIS NEVER HAPPENS,
		 ** because CallOK returns SS_OK in this case!
		 **/

		Log("PREVIOUS %s", BADSEQ);
		OutMesg('R', BADSEQ, Ofd);
		finish(EX_OK);
		return;
	}

	if (GetNxtSeq(RemoteNode) != Seq) {
		UnlockSeq();
		(void)Status(SS_BADSEQ, RemoteNode, BADSEQ);
		Log(HANDSHAKE, FAILED, BADSEQ);
		OutMesg('R', BADSEQ, Ofd);
		finish(EX_OK);
		return;
	}


#if CAN_RESTART_MSG_XFER
	OutMesg('R', (Restart ? "OK -R" : "OK"), Ofd);	/* XXX */
#else
	OutMesg('R', "OK", Ofd);
#endif

	CommitSeq();

	if (TtyDevice != NULLSTR && TtyDevice[0] == '/')
		(void)chmod(TtyDevice, 0600);

	if (InvokeDir == NULLSTR) {
		InvokeDir = ChNodeDir(RemoteNode, NULLSTR);
		CreateActive(true);
	}
}

/*
**	Test if shutdown required.
*/

static void
test_nologin()
{
	if ( access(NOLOGIN, 0) == SYSERROR )
		return;

	if ( CurrentRole == SLAVE )
		Log("UUCICO SHUTDOWN %s", NOLOGIN);

	if ( !Debugflag || RemoteDebug )
		finish(EX_UNAVAILABLE);

	if ( CurrentRole == SLAVE )
		Log("continuing anyway DEBUGGING");
}

/*
**	Catch ALARM signal.
*/

static void
timeout(int sig)
{
	Debug((1, "TIMEOUT"));

	if ( !HaveSentHup )
	{
		static char fmt[] = "TIMEOUT %s %s";

		(void)Status(SS_FAIL, RemoteNode, fmt, FAILED, EmptyStr);
		Log(fmt, FAILED, RemoteNode);
	}

	longjmp(AlarmJmp, 1);
}

/*
**	Catch TRACE signal.
*/

static void
toggle_trace(int sig)
{
	(void)signal(sig, toggle_trace);

	if ( Debugflag )
		Debugflag = 0;
	else
	{
		Debugflag = DFLT_DEBUG_LEVEL;
		Log("DEBUG %d enabled by signal", Debugflag);
	}

	setaudit(AUDIT_PERM);

	DebugRemote = 0;
}

/*
**	Execute UUCP for local file copy.
*/

int
Uucp(
	char *		f1,
	char *		f2
)
{
	register int	n;
	ExBuf		args;

	if ( UUCP == NULLSTR )
		return EX_OK;

	FIRSTARG(&args.ex_cmd) = UUCP;

#if	DEBUG
	if ( Traceflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-T%d", Traceflag);
#endif	/* DEBUG */

	if ( Debugflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-x%d", Debugflag);

	ExpandArgs(&args.ex_cmd, UUCPARGS);

	if ( NewParamsFile )
		NEXTARG(&args.ex_cmd) = concat("-P", PARAMSFILE, NULLSTR);

	n = NARGS(&args.ex_cmd);	/* These should be free'd */

	NEXTARG(&args.ex_cmd) = "-r";
	NEXTARG(&args.ex_cmd) = f1;
	NEXTARG(&args.ex_cmd) = f2;

	if ( (f1 = Execute(&args, ExIgnSigs, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(f1);
		free(f1);
	}

	while ( --n > 0 )		/* Free all except first */
		free(ARG(&args.ex_cmd, n));

	return ExStatus;
}

/*
**	Validate remote node.
**
**	No return if invalid.
*/

static void
validate_node()
{
	register char *		cp;
	register char *		dp;
	register int		c;
	bool			invalid;
	struct sockaddr_in	peer;
	struct hostent *	InetPeer;
	int			peer_len	= sizeof(peer);
	char *			msg;
	char *			sysdetails;

	extern char *		inet_ntoa(struct in_addr);

	DebugT(1, (1,
		"call from \"%s\", login name \"%s\"",
		ExpandString(RemoteNode, -1),
		LoginName
	));

	if ( RemoteNode == NULLSTR || RemoteNode[0] == '\0' )
		RemoteNode = "UNKNOWN";
	else
	{
		for ( invalid = false, cp = RemoteNode ; c = *cp++ ; )
			if ( !isalnum(c) && strchr(LEGAL_NODE_CHARS, c) == NULLSTR )
			{
				cp[-1] = '?';
				invalid = true;
			}

		RemoteNode = newstr(RemoteNode);	/* Take copy out of MesgBuf[] */
	}

	if ( invalid )
	{
		(void)sprintf(MesgBuf, "Invalid remote name: %s", RemoteNode);

		Warn("%s Login: %s", MesgBuf, LoginName);

		OutMesg('R', MesgBuf, Ofd);
		finish(EX_OK);
		return;
	}

	setaudit(AUDIT_PERM);

	Debug((4, "sys-%s", RemoteNode));

	if ( !FindSysEntry(&RemoteNode, NEEDSYS, NEEDALIAS) )
		check_access(NOSTRANGERS, false);	/* Check anonymous attempt */

	check_access(LOGIN_MUST_MATCH, true);

	if ( !IsTCP )
		return;

	/*
	**	Verify IP address.
	*/

	FreeStr(&IPHostName);

	if ( getpeername(Ifd, (struct sockaddr *) &peer, &peer_len) == SYSERROR )
	{
		Log("NOT A TCP CONNECTION %s", RemoteNode);
		OutMesg('R', "NOT TCP", Ofd);
		finish(EX_OK);
		return;
	}

	if
	(
		(InetPeer = gethostbyaddr
				(
					(char *)&peer.sin_addr,
					sizeof(peer.sin_addr),
					peer.sin_family
				)) == (struct hostent *)0
	)
	{
		IPHostName = newstr(inet_ntoa(peer.sin_addr));

		if ( VERIFY_TCP_ADDRESS )
		{
			Log("UNKNOWN IP-HOST Name = %s, Number = %s", RemoteNode, IPHostName);

			(void)sprintf(MesgBuf, "%s/%s isn't in my host table", RemoteNode, IPHostName);
			OutMesg('R', MesgBuf, Ofd);
			finish(EX_OK);
			return;
		}
	}
	else
		IPHostName = newstr(InetPeer->h_name);

	Debug((1, "Request from IP-Host name = %s", IPHostName));

	if ( !VERIFY_TCP_ADDRESS )
		return;

	sysdetails = dp = newstr(SysDetails);	/* Preserve `SysDetails' */

	for ( msg = NULLSTR ; dp != NULLSTR ; dp = cp )
	{
		char *	fields[SYS_PHONE];
		char *	mp;

		if ( (cp = strchr(dp, '\n')) != NULLSTR )
			*cp++ = '\0';

		if ( SplitSpace(fields, dp, SYS_PHONE) < SYS_PHONE )
			continue;

		dp = fields[SYS_PHONE-1];

		if ( strccmp(dp, IPHostName) == STREQUAL )
		{
			Debug((1, "%s found in %s", IPHostName, SYSFILE));

			free(sysdetails);
			FreeStr(&msg);
			return;
		}

		if ( (mp = msg) == NULLSTR )
			msg = newstr(dp);
		else
		{
			msg = concat(mp, " | ", dp, NULLSTR);
			free(mp);
		}
	}

	free(sysdetails);

	if ( msg == NULLSTR )
		return;		/* Nothing to verify */

	Log
	(
		"FORGED HOSTNAME %s ORIGINATED AT %s SHOULD BE %s",
		RemoteNode,
		IPHostName,
		msg
	);

	(void)sprintf
	(
		MesgBuf,
		"You're not who you claim to be: %s != %.*s",
		IPHostName,
		MAX_MESG_CHARS-50,
		msg
	);

	free(msg);

	OutMesg('R', MesgBuf, Ofd);

	finish(EX_OK);
}

#ifdef USE_X25
/* A quick alphanumeric index generator */
#define TtyIndex(a)	("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")[a]

static char *
X25lock()
{
	register int i, fd;
	static char lockfile[PATHNAMESIZE];

	for (i = 0; i < MAX_X25_VC && i < 62; i++) {
		(void) sprintf(lockfile, "%s%s%c", LOCKDIR, X25LOCKPRE, TtyIndex(i));
		if ((fd = open(lockfile, O_CREAT|O_EXCL|O_WRONLY,00600)) > 1)
			break;	/* We found one */
		else
			if (fd == -1 && errno != EEXIST)
				SysWarn("open: %s", lockfile);
	}
	if (fd < 1)
		return(NULL);
	else
	{
		char *cp;

		i = strlen(cp = &TtyDevice[5]);
		if (write(fd, cp, i) < i)
			SysWarn("write: %s", lockfile);
		close(fd);

		cp = strrchr(lockfile, '/');
		return(cp++ ? cp : NULL);
	}
}


static int
X25unlock()
{
	char lockfile[PATHNAMESIZE];

	(void) sprintf(lockfile, "%s%s", LOCKDIR, TtyName);
	if (unlink(lockfile) == -1) {
		SysWarn("unlink: %s", lockfile);
		return(-1);
	}
	else
		return(0);
}
#endif /* USE_X25 */
