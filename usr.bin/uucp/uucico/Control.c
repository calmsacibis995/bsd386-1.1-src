/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to control virtual circuit conversation.
**
**	RCSID $Id: Control.c,v 1.3 1994/01/31 01:26:37 donn Exp $
**
**	$Log: Control.c,v $
 * Revision 1.3  1994/01/31  01:26:37  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.2  1992/11/20  19:40:00  trent
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
 * Revision 1.1.1.1  1992/09/28  20:08:55  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.6  1992/05/19  10:09:48  ziegast
 * RCS cleanup, reapplied bugfix after diff with 1.4
 *
 * Revision 1.4  1992/04/24  16:17:32  piers
 * nochange
 *
 * Revision 1.3  1992/04/22  22:18:20  piers
 * HUP exchanges encapsulated into one routine
 * MASTER role switch now done at end of C. file sequence
 * (rather than maybe in the middle).
 *
 * Revision 1.2  1992/04/17  21:51:51  piers
 * add RESCAN check for MASTER mode (to return to SPOOLDIR)
 * sort routines lexically
 * change name_prog
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	CTYPE
#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	STDIO
#define	SETJMP
#define	SYSEXITS
#define	SYS_STAT
#define TERMIOCTL

#include	"global.h"
#include	"cico.h"
#include	"Protocol.h"


/*
**	Protocol choices.
*/

char		Protocol;

static CallType	dflt_rd_msg(char *, int);
static CallType	dflt_wr_msg(char, char *, int);
static CallType	null_func(void);

Proto		Ptbl[] =
{
	{'t', null_func, trdmsg, twrmsg, trddata, twrdata, null_func},
#ifdef	X25_PAD
	{'f', fturnon, frdmsg, fwrmsg, frddata, fwrdata, fturnoff},
#endif	/* X25_PAD */
#if	USE_CAP_G_PROTO
	{'G', Gturnon, grdmsg, gwrmsg, grddata, gwrdata, gturnoff},
#endif	/* USE_CAP_G_PROTO */
	{'g', gturnon, grdmsg, gwrmsg, grddata, gwrdata, gturnoff},
	{'\0', null_func, dflt_rd_msg, dflt_wr_msg, 0, 0, null_func}	/* Null protocol */
};

#define	DFLTPROTO	(ARR_SIZE(Ptbl)-2)	/* `g' is default */
#define	NULLPROTO	(ARR_SIZE(Ptbl)-1)

#define	Rdmsg		Ptbl[NULLPROTO].P_rdmsg
#define	Wrmsg		Ptbl[NULLPROTO].P_wrmsg
#define	Rddata		Ptbl[NULLPROTO].P_rddata
#define	Wrdata		Ptbl[NULLPROTO].P_wrdata
#define	Turnon		Ptbl[NULLPROTO].P_turnon
#define	Turnoff		Ptbl[NULLPROTO].P_turnoff

/*
**	Failure messages.
*/

struct
{
	char	message[3];
	char *	explain;
}
	Failures[] =
{
	{"N0", "COPY FAILED (reason not given by remote)"},
	{"N1", "local access to file denied"},
	{"N2", "remote access to path/file denied"},
	{"N3", "system error - bad uucp command generated"},
	{"N4", "remote system can't create temp file"},
	{"N5", "can't copy to file/directory - file left in PUBDIR/user/file"},
	{"N6", "can't copy to file/directory on local system  - file left in PUBDIR/user/file"}
};

#define NFAILURES	ARR_SIZE(Failures)

#define EM_LOCACC	Failures[1].message
#define EM_RMTACC	Failures[2].message
#define EM_BADUUCP	Failures[3].message
#define EM_NOTMP	Failures[4].message
#define EM_RMTCP	Failures[5].message
#define EM_LOCCP	Failures[6].message

/*
**	Message types.
*/

#define XUUCP		'X'	/* execute uucp (string) */
#define SLTPTCL		'P'	/* select protocol  (string)  */
#define USEPTCL		'U'	/* use protocol (character) */
#define RCVFILE		'R'	/* receive file (string) */
#define SNDFILE		'S'	/* send file (string) */
#define RQSTCMPT	'C'	/* request complete (string - yes | no) */
#define HUP    		'H'	/* ready to hangup (string - yes | no) */
#define RESET		'X'	/* reset line modes */

/*
**	Fields of a command.
*/

#define W_TYPE		wrkvec[0]
#define W_FILE1		wrkvec[1]
#define W_FILE2		wrkvec[2]
#define W_USER		wrkvec[3]
#define W_OPTNS		wrkvec[4]
#define W_DFILE		wrkvec[5]
#define W_MODE		wrkvec[6]
#define W_NUSER		wrkvec[7]

#define	MAXCMDFIELDS	20	/* XXX ??! */

/*
**	Various macros.
*/

#define	XFRRATE		35000L
#define PACKSIZE	64

static char	Wfile[PATHNAMESIZE];
static char	Dfile[PATHNAMESIZE];

/*
**	To avoid a huge backlog of X. files, start uuxqt every so often.
*/

static int	nXfiles;	/* Number of X files since last uuxqt start */

/*
**	Miscellaneous variables.
*/

static Time_t	LastCheckedNoLogin;
static Time_t	LastTurned;
static char	NOT[]			= "N";
static char	RECEIVING[]		= "RECEIVING";
static char	SENDING[]		= "SENDING";
static char	send_or_receive;
static bool	TransferSucceeded	= true;
static int	willturn;
static char	YES[]			= "Y";

static struct stat	StatBuf;

bool		HaveSentHup;	/* Global */

/*
**	Local routines.
*/

static bool	anyread(char *);
static char *	BuildProtocols(char *);
static bool	checklist(char *, char **);
static bool	checkperm(char *, char *);
static void	corrupt(char *, int);
static bool	exchange_hups(char *, char *);
static char *	ExplainMesg(char *);
static void	filecat(char *, char *);
static bool	FindCmds(char *, Find_t);
static int	GetCmdArgs(char *, char **, int);
static int	GetNextCmd(char *, char **, int);
static bool	isdir(char *);
static char *	lastpart(char *);
static void	mail_arrived(char *, char *, char *, char *);
static void	mail_err(char *, char *, char *);
static void	mail_notice(int, char *, char *, char *, char *);
static void	newstatus(Role, char);
static void	orphan_uuxqt(void);
static void	MakeTmp(char);
static char	MatchProtocols(char *);
static bool	putinpub(char *, char *, char *);
static bool	receive_msg(char, char *, int);
static void	rescan(void);
static bool	send_msg(char, char *);
static void	setupline(char);
static CallType	StartProtocol(char);
static void	unlinkdf(char *);
static void	update_turntime(void);
static void	wrong_role(char *);


/*
**      Check if anybody can read.
*/

static bool
anyread(
	char *		file
)
{
	struct stat	statb;

	if ( stat(file, &statb) == SYSERROR )
		return true;	/* for security check a non existant file is readable */

	if ( !(statb.st_mode & 04) )
		return false;

	return true;
}

/*
**	Build a string of the letters of the available protocols.
*/

static char *
BuildProtocols(
	char *		str
)
{
	register Proto *p;
	register char *	s;

	for ( p = Ptbl, s = str ; p->P_id != '\0' ; p++ )
		if
		(
			SysProtocols == NULLSTR
			||
			strchr(SysProtocols, p->P_id) != NULLSTR
		)
			*s++ = p->P_id;

	*s = '\0';

	Trace((1, "BuildProtocols => %s", str));

	return str;
}

/*
**	Check a list of strings for a match.
*/

static bool
checklist(
	char *		str,
	char *		list[]
)
{
	register char *	cp;

	for ( cp = *list++ ; cp != NULLSTR ; cp = *list++ )
		if ( strcmp(str, cp) == STREQUAL )
		{
			Debug((1, "checklist() matches %s", str));
			return true;
		}

	return false;
}

/*
**	Check write permission.
*/

static bool
checkperm(
	char *		file,
	char *		dirs
)
{
	register char *	cp;
	struct stat	statb;
	int		ret;
	bool		val;
	static char	fmt[]	= "%s is not writable: mode %o";

	if ( stat(file, &statb) != SYSERROR )
	{
		if ( (statb.st_mode & 02) == 0 )
		{
			Debug((4, fmt, "file", statb.st_mode));
			return false;
		}

		return true;
	}

	if ( (cp = strrchr(file, '/')) == NULLSTR )
		file = ".";
	else
		*cp = '\0';

	if ( (ret = stat(file, &statb)) == SYSERROR && dirs == NULLSTR )
	{
		Debug((4, "can't stat directory %s", file));

		if ( cp != NULLSTR )
			*cp = '/';

		return false;
	}

	if ( cp != NULLSTR )
		*cp = '/';

	if ( ret != SYSERROR )
	{
		if ( (statb.st_mode & 02) == 0 )
		{
			Debug((4, fmt, "directory", statb.st_mode));
			return false;
		}

		return true;
	}

	ret = DIR_MODE;
	DIR_MODE = 0777;	/* Public write needed for these */

	val = MkDirs(file, NetUid, NetGid);

	DIR_MODE = ret;
	return val;
}

/*
**	This routine will execute the conversation between
**	the two machines after both programs are running.
*/

CallType
Control(
	Role		role
)
{
	register FILE *	fp;
	register int	i;
	register int	narg;
	int		filemode;
	bool		ok;
	int		mailopt;
	int		ret;
	bool		val;
	char		wrktype;

	char *		wrkvec[MAXCMDFIELDS];
	char		filename[PATHNAMESIZE];
	char		msg[BUFSIZ];

	static int	tmpnum;
	static char	user[10];

	Wfile[0] = '\0';
	msg[0] = '\0';
	willturn = TurnTime > 0;
	fp = NULL;

remaster:
	if ( nXfiles )
		orphan_uuxqt();
	update_turntime();
	send_or_receive = RESET;
	HaveSentHup = false;
	rescan();		/* Force re-scan for work files */

top:
	if ( fp != NULL )
	{
		Debug((1, "forced fclose of fd=%d", fileno(fp)));
		(void)fclose(fp);
	}

	(void)strclr((char *)wrkvec, sizeof(wrkvec));

	Debug((2, "*** TOP ***  -  role=%s", (role==MASTER) ? MasterStr : SlaveStr));

	setupline(RESET);

	if ( Time > LastCheckedNoLogin )
	{
		LastCheckedNoLogin = Time + 60;

		if ( access(NOLOGIN, 0) != SYSERROR )
		{
			Log("UUCICO SHUTDOWN %s", NOLOGIN);
			if ( !RemoteDebug && Debugflag > 4 )
				Log("continuing anyway DEBUGGING");
			else
			{
				if ( !exchange_hups(YES, msg) )
					goto out;
				goto process;
			}
		}
	}

#ifdef SPRINT_HACK
	if ( Is900 && Time > HangupTime ) {
		Log("UUCICO SHUTDOWN (SPRINT_HACK)");
		(*Turnoff)();
		goto out;
	}
#endif /* SPRINT_HACK */

	if ( role == MASTER )
	{
		register char *	cp;

		/** Get work **/

		for ( i = 0 ; (narg = GetNextCmd(Wfile, wrkvec, MAXCMDFIELDS)) == 0 ; i++ )
		{
			if ( ReverseRole )
			{
				ReverseRole = false;
				if ( !exchange_hups(EmptyStr, msg) )
					goto out;
				goto process;
			}

			if
			(
				SpoolAltDirs > 1
				&&
				Time > (LastTurned+TurnTime)
				&&
				strncmp(InvokeDir, SPOOLDIR, SpoolDirLen) != STREQUAL
			)
			{
				Log("RESCAN %s", SPOOLDIR);
				goto remaster;	/* Too long in this directory */
			}

			if ( i > 3 || !FindCmds(Wfile, FINDFILE) )
			{
				if ( !exchange_hups(EmptyStr, msg) )
					goto out;
				goto process;
			}
		}

		wrktype = W_TYPE[0];

		for ( cp = msg, i = 1 ; i < narg ; i++ )
		{
			*cp++ = ' ';
			cp = strcpyend(cp, wrkvec[i]);
		}
		*cp = '\0';

		if ( wrktype == XUUCP )
		{
			Log("REQUEST X %s", msg);
			goto sendmsg;
		}

		if ( narg < 5 || W_TYPE[1] != '\0' )
		{
			corrupt(Wfile, narg);
			goto top;
		}

		mailopt = strchr(W_OPTNS, 'm') != NULLSTR;

		UserName = strncpy(user, W_USER, 9);	user[9] = '\0';

		Log("REQUEST (%s %s %s %s)", W_TYPE, W_FILE1, W_FILE2, W_USER);

		if ( wrktype == SNDFILE  )
		{
			(void)strcpy(filename, W_FILE1);

			if
			(
				!(val = ExpFn(filename))
				&&
				!CheckUserPath(UserName, NULLSTR, filename)
			)
			{
				Debug((1, "Path check denied for: %s", filename));
				goto e_access;
			}

			(void)strcpy(Dfile, W_DFILE);

			if ( strchr(W_OPTNS, 'c') == NULLSTR )
			{
				if ( (fp = fopen(Dfile, "r")) != NULL )
					val = true;
				else
					Debug((1, CouldNot, ReadStr, Dfile));
			}

			if ( fp == NULL )
				while ( (fp = fopen(filename, "r")) == NULL )
				{
					if ( SysRetry(errno) )
						continue;

					Log("%s CAN'T READ DATA (%s)", FAILED, filename);
					unlinkdf(Dfile);

					mail_err(UserName, filename, "can't access");
					TransferSucceeded = true;
					goto top;
				}

			/** If file exists but is not generally readable... **/

			if
			(
				!val
				&&
				fstat(fileno(fp), &StatBuf) == 0
				&&
				(StatBuf.st_mode & 04) == 0
			)
			{
				Debug((1, CouldNot, ReadStr, filename));

				if ( fp != NULL )
				{
					(void)fclose(fp);
					fp = NULL;
				}

e_access:			/** Access denied **/

				Log("ACCESS DENIED %s", filename);
				unlinkdf(W_DFILE);

				mail_err(UserName, filename, "access denied");
				TransferSucceeded = true;
				goto top;
			}

			setupline(SNDFILE);
		}

		if ( wrktype == RCVFILE )
		{
			(void)strcpy(filename, W_FILE2);

			(void)ExpFn(filename);

			if
			(
				!CheckUserPath(UserName, NULLSTR, filename )
				||
				!checkperm(filename, strchr(W_OPTNS, 'd'))
			)
				goto e_access;

			MakeTmp('T');

			while ( (fp = fopen(Dfile, "w")) == NULL )
			{
				if ( SysWarn(CouldNot, CreateStr, Dfile) )
					continue;

				Log("%s CAN'T CREATE TM (%s)", FAILED, Dfile);
				unlinkdf(Dfile);

				/* TransferSucceeded = true; TRY AGAIN LATER ? */
				goto top;
			}

			setupline(RCVFILE);
		}
sendmsg:
		Debug((4, "wrktype - %c", wrktype));

		if ( !send_msg(wrktype, msg) || !receive_msg(wrktype, msg, 1) )
			goto out;
		goto process;
	}

	/** Role is slave **/

	if ( !receive_msg('\0', msg, 1) )
		goto out;

	if ( willturn < 0 )
		willturn = msg[0] == HUP;

process:
	Debug((2, "PROCESS: msg - %s", msg));

	switch ( msg[0] )
	{
	case RQSTCMPT:
		Debug((2, "RQSTCMPT:"));

		if ( msg[1] == 'N' )
			Log("REQUEST FAILED %s", ExplainMesg(&msg[2]));

		TransferSucceeded = true;

		if ( role == MASTER )
		{
			mail_notice(mailopt, W_USER, W_FILE1, RemoteNode, &msg[1]);

			if ( msg[2] == 'M' )
			{
turn:
				if ( !exchange_hups(EmptyStr, msg) )
					break;
				Log("TURNAROUND %s", RemoteNode);
				update_turntime();
				rescan();	/* Force re-scan for work files */
				goto process;
			}
		}

		goto top;

	case HUP:
		Debug((1, "HUP:"));

		HaveSentHup = true;

		if ( msg[1] == 'Y' )
		{
			if ( role == MASTER )
				(void)send_msg(HUP, YES);

			(*Turnoff)();
			Rdmsg = dflt_rd_msg;
			Wrmsg = dflt_wr_msg;

			if ( nXfiles )
				orphan_uuxqt();
			return SUCCESS;
		}

		if ( msg[1] == 'N' )
		{
			if ( role != MASTER )
				wrong_role("HUP");

			role = SLAVE;
			goto remaster;
		}

		/** Get work **/

		rescan();	/* Force re-scan for work files */

		if ( !FindCmds(Wfile, CHECKFILE) )
		{
			if ( !exchange_hups(YES, msg) )
				break;
			goto process;
		}

		if ( !send_msg(HUP, NOT) )
			break;

		role = MASTER;
		goto remaster;

	case XUUCP:
		if ( role == MASTER )
			goto top;

		/** Slave part **/

		i = GetCmdArgs(msg, wrkvec, MAXCMDFIELDS);

		(void)strcpy(filename, W_FILE1);

		if
		(
			strchr(filename, ';') != NULLSTR
			||
			strchr(W_FILE2, ';') != NULLSTR
			||
			i < 3
		)
		{
			if ( !send_msg(XUUCP, NOT) )
				break;
			goto top;
		}

		(void)ExpFn(filename);

		if ( !CheckUserPath(NULLSTR, RemoteNode, filename) )
		{
			if ( !send_msg(XUUCP, NOT) )
				break;
			Log("%s XUUCP DENIED", filename);
			goto top;
		}

		Debug((1, "XUUCP %s %s", filename, W_FILE2));
		Uucp(filename, W_FILE2);
		if ( !send_msg(XUUCP, YES) )
			break;
		goto top;

	case SNDFILE:
		/** MASTER section of SNDFILE **/

		Debug((1, "SNDFILE:"));

		if ( msg[1] == 'N' )
		{
			Log("REQUEST FAILED %s", ExplainMesg(&msg[2]));

			(void)fclose(fp);
			fp = NULL;

			/** Dont send him files he can't save **/

			if ( strcmp(&msg[1], EM_NOTMP) == STREQUAL  )
			{
				if ( !exchange_hups(EmptyStr, msg) )
					break;
				goto process;
			}

			if ( strcmp(&msg[1], EM_RMTACC) == STREQUAL )
			{
				Log("REQUEST DELETED %s %s -> %s", W_TYPE, W_FILE1, W_FILE2);
				TransferSucceeded = true;	/* We want to remove the job */
			}

			mail_notice(mailopt, W_USER, W_FILE1, RemoteNode, &msg[1]);

			if ( role != MASTER )
				wrong_role("SN");

			unlinkdf(W_DFILE);
			goto top;
		}

		if ( msg[1] == 'Y' )
		{
			/** Send file **/

			if ( role != MASTER )
				wrong_role("SY");

			if ( fstat(fileno(fp), &StatBuf) == SYSERROR )
			{
				(void)SysWarn(CouldNot, StatStr, filename);
				break;
			}

			i = 1 + (int)(StatBuf.st_size / XFRRATE);

			newstatus(role, SNDFILE);

			ret = (*Wrdata)(fp, Ofd);

			(void)fclose(fp);
			fp = NULL;

			if ( ret != SUCCESS )
			{
				(*Turnoff)();
				return FAIL;
			}

			TransferSucceeded = true;

			if ( !receive_msg(RQSTCMPT, msg, i) )
				break;

			unlinkdf(W_DFILE);
			goto process;
		}

		/** SLAVE section of SNDFILE **/

		if ( role != SLAVE )
			wrong_role("SLAVE");

		/** Request to receive file **/

		/** Check permissions **/

		if ( (i = GetCmdArgs(msg, wrkvec, MAXCMDFIELDS)) < 5 )
		{
			corrupt(Wfile, i);
			goto top;
		}

		Log("REQUESTED (%s %s %s %s)", W_TYPE, W_FILE1, W_FILE2, W_USER);

		Debug((4, "msg - %s", msg));
		(void)strcpy(filename, W_FILE2);

		/*
		**	Expand filename, returns `true' if this is
		**	is a vanilla spool file, so no stat(2)s are needed
		*/

		i = 0;
		if ( !ExpFn(filename) )
		{
			i = 1;

			if
			(
				!CheckUserPath(NULLSTR, RemoteNode, filename )
				||
				!checkperm(filename, strchr(W_OPTNS, 'd'))
			)
			{
				Log("PERMISSION DENIED %s", filename);
				if ( !send_msg(SNDFILE, EM_RMTACC) )
					break;
				goto top;
			}

			Debug((4, "%s: chkpth ok", filename));

			filecat(filename, W_FILE1);
		}
		else
		if ( checklist(LoginName, ANON_USERS) )
		{
			Log("PERMISSION DENIED %s", filename);
			Warn("Tried to send files as <%s>", LoginName);
			TransferSucceeded = true;
			if ( !send_msg(SNDFILE, EM_RMTACC) )
				break;
			goto top;
		}

		UserName = strncpy(user, W_USER, 9);	user[9] = '\0';

		/*
		**	Speed things up by OKing file before
		**	creating TM file.  If the TM file cannot be created,
		**	then the conversation bombs, but that seems reasonable,
		**	as there are probably serious problems then.
		*/

		if ( !send_msg(SNDFILE, YES) )
			break;

		MakeTmp('t');

		while ( (fp = fopen(Dfile, "w")) == NULL )
		{
			if ( SysWarn(CouldNot, CreateStr, Dfile) )
				continue;
			Log("TM FILE CAN'T OPEN (%s)", Dfile);
			goto df_fail;
		}

		newstatus(role, RCVFILE);

		ret = (*Rddata)(Ifd, fp);

		if ( fclose(fp) )
		{
			(void)SysWarn(CouldNot, WriteStr, Dfile);
			Log("TM FILE CAN'T WRITE (%s)", Dfile);
			ret = FAIL;
		}
		fp = NULL;

		if ( ret != SUCCESS )
		{
df_fail:		unlinkdf(Dfile);
			break;
		}

		TransferSucceeded = true;

		/** Copy to user directory **/

		ok = MoveCp(Dfile, filename);

		if
		(
			willturn
			&&
			Time > (LastTurned+TurnTime)
			&&
			FindCmds(Wfile, CHECKFILE)
		)
		{
			willturn = -1;
			if ( !send_msg(RQSTCMPT, ok == false ? EM_RMTCP : "YM") )
				break;
		}
		else
		if ( !send_msg(RQSTCMPT, ok == false ? EM_RMTCP : YES) )
			break;

		if ( i == 0 )
			;	/* vanilla file, nothing to do */
		else
		if ( ok )
		{
			if ( W_MODE == 0 || sscanf(W_MODE, "%o", &filemode) != 1 )
				filemode = FILE_MODE;
			(void)chmod(filename, (filemode|FILE_MODE)&0777);
		}
		else
		{
			Log("COPY %s", FAILED);
			ok = putinpub(filename, Dfile, W_USER);
			Debug((4, "->PUBDIR %d", ok));
		}

		if ( ok && strchr(W_OPTNS, 'n') != NULLSTR )
			mail_arrived(filename, W_NUSER, RemoteNode, UserName);

		/** Run uuxqt occasionally **/

		if ( lastpart(filename)[0] == EXEC_TYPE && ++nXfiles > 10 )
			orphan_uuxqt();

		goto top;

	case RCVFILE:
		/** MASTER section of RCVFILE **/

		Debug((1, "RCVFILE:"));

		if ( msg[1] == 'N' )
		{
			Log("REQUEST FAILED %s", ExplainMesg(&msg[2]));

			(void)fclose(fp);
			fp = NULL;

			mail_notice(mailopt, W_USER, W_FILE1, RemoteNode, &msg[1]);

			if ( role != MASTER )
				wrong_role("RN");

			unlinkdf(Dfile);
			TransferSucceeded = true;
			goto top;
		}

		if ( msg[1] == 'Y' )
		{
			/** Receive file **/

			if ( role != MASTER )
				wrong_role("RY");

			newstatus(role, RCVFILE);

			ret = (*Rddata)(Ifd, fp);

			if ( fclose(fp) )
				ret = FAIL;
			fp = NULL;

			if ( ret != SUCCESS )
				goto df_fail;

			TransferSucceeded = true;

			/** Copy to user directory **/

			filecat(filename, W_FILE1);

			ok = MoveCp(Dfile, filename);

			if ( !send_msg(RQSTCMPT, ok==false ? EM_RMTCP : YES) )
				break;

			mail_notice(mailopt, W_USER, filename, RemoteNode, ok==false ? EM_LOCCP : YES);

			if ( ok )
			{
				if ( sscanf(&msg[2], "%o", &filemode) != 1 )
					filemode = FILE_MODE;
				(void)chmod(filename, (filemode|FILE_MODE)&0777);

				/** Run uuxqt occasionally **/

				if ( lastpart(filename)[0] == EXEC_TYPE && ++nXfiles > 10 )
					orphan_uuxqt();
			}
			else
			{
				Log("COPY %s", FAILED);
				(void)putinpub(filename, Dfile, W_USER);
			}

			if ( msg[strlen(msg)-1] == 'M' )
				goto turn;

			goto top;
		}

		/** SLAVE section of RCVFILE **/

		if ( role != SLAVE )
			wrong_role("SLAVE RCV");

		/** Request to send file **/

		Log("REQUESTED (%s)", msg);

		/** Check permissions **/

		if ( (i = GetCmdArgs(msg, wrkvec, MAXCMDFIELDS)) < 4 )
		{
			corrupt(Wfile, i);
			goto top;
		}

		Debug((4, "msg - %s, W_FILE1 - %s", msg, W_FILE1));

		(void)strcpy(filename, W_FILE1);

		(void)ExpFn(filename);

		filecat(filename, W_FILE2);

		UserName = strncpy(user, W_USER, 9);	user[9] = '\0';

		if ( !CheckUserPath(NULLSTR, RemoteNode, filename) || !anyread(filename) )
		{
			Log("PERMISSION DENIED %s", filename);
			if ( !send_msg(RCVFILE, EM_RMTACC) )
				break;
			goto top;
		}

		Debug((4, "%s: chkpth ok", filename));

		while ( (fp = fopen(filename, "r")) == NULL )
		{
			if ( SysRetry(errno) )
				continue;
			Log("Couldn't open %s: %s", filename, ErrnoStr(errno));
			if ( !send_msg(RCVFILE, EM_RMTACC) )
				break;
			goto top;
		}

		/** OK to send file **/

		if ( fstat(fileno(fp), &StatBuf) == SYSERROR )
		{
			SysWarn(CouldNot, StatStr, filename);
			break;
		}

		i = 1 + (int)(StatBuf.st_size / XFRRATE);

		if
		(
			willturn && Time > (LastTurned+TurnTime)
			&&
			FindCmds(Wfile, CHECKFILE)
		)
			willturn = -1;

		(void)sprintf
		(
			msg,
			"%s %o%s",
			YES,
			(int)StatBuf.st_mode & 0777,
			willturn < 0 ? " M" : EmptyStr
		);

		if ( !send_msg(RCVFILE, msg) )
			break;

		newstatus(role, SNDFILE);

		ret = (*Wrdata)(fp, Ofd);

		(void)fclose(fp);
		fp = NULL;

		if ( ret != SUCCESS )
			break;

		TransferSucceeded = true;

		if ( !receive_msg(RQSTCMPT, msg, i) )
			break;
		goto process;
	}

	(*Turnoff)();
out:
	if ( nXfiles )
		orphan_uuxqt();

	return FAIL;
}

/*
**	Put corrupt command file away.
*/

static void
corrupt(
	char *	file,
	int	narg
)
{
	char	path[PATHNAMESIZE];
	char *	cp;

	cp = strcpyend(path, CORRUPTDIR);
	(void)strcpy(cp, lastpart(file));

	MoveCp(file, path);

	Warn("%s CORRUPTED: %d args", file, narg);

	file[0] = '\0';
}

/*
**	Default protocol input routine.
*/

static CallType
dflt_rd_msg(
	char *	msg,
	int	fd
)
{
	return InMesg(BUFSIZ, msg, fd) ? SUCCESS : FAIL;
}

/*
**	Default protocol output routine.
*/

static CallType
dflt_wr_msg(
	char	type,
	char *	msg,
	int	fd
)
{
	return OutMesg(type, msg, fd) > 0 ? SUCCESS : FAIL;
}

/*
**	Exchange hangup messages.
*/

static bool
exchange_hups(
	char *	type,	/* YES or EmptyStr */
	char *	buf
)
{
	if ( !send_msg(HUP, type) || !receive_msg(HUP, buf, 1) )
		return false;

	return true;
}

/*
**	Extract explanation for fail message of form `Nnnn'.
*/

static char *
ExplainMesg(
	char *		m
)
{
	register int	i;

	if
	(
		(i = atoi(m)) < 0
		||
		i >= NFAILURES
	)
		i = 0;

	return Failures[i].explain;
}

/*
**	Concatenate directory and filename.
*/

static void
filecat(
	char *	path,
	char *	file
)
{
	char *	cp;

	if ( !isdir(path) )
		return;

	cp = &path[strlen(path)];
	*cp++ = '/';
	(void)strcpy(cp, lastpart(file));
}

/*
**	Find next command file.
*/

static bool
FindCmds(
	char *		file,
	Find_t		reqst
)
{
	char *		newfile;
	static char	prefix[3]	= {CMD_TYPE, '.', '\0'};

	if ( checklist(LoginName, ANON_USERS) )
		return false;	/* Can never find work for anonymous users */

	while ( (newfile = FindFile(InvokeDir, prefix, reqst)) == NULLSTR )
	{
		CreateActive(false);

		if ( (InvokeDir = ChNodeDir(RemoteNode, InvokeDir)) == NULLSTR )
		{
			InvokeDir = ChNodeDir(RemoteNode, NULLSTR);
			CreateActive(true);

			if ( (newfile = FindFile(InvokeDir, prefix, reqst)) == NULLSTR )
				return false;
			break;
		}

		CreateActive(true);
	}

	if ( reqst == CHECKFILE )
		return true;

	(void)strcpy(file, newfile);

	return true;
}

/*
**	Split command string into parts.
**	Error if too many.
*/

static int
GetCmdArgs(
	char *	str,
	char *	args[],
	int	max
)
{
	int	val;

	if ( (val = SplitSpace(args, str, max)) > max )
		return 1;	/* Cause reject */

	return val;
}

/*
**	Create a vector of command arguments.
*/

static int
GetNextCmd(
	register char *	file,
	register char **wvec,
	int		size
)
{
	register char *	cp;

	static char	str[MAXRQST+1], nstr[MAXRQST+1], lastfile[PATHNAMESIZE];
	static FILE *	fp;
	static long	nextread, nextwrite;

	/*
	**	If called with a null file,
	**	force a shutdown of the current work file.
	*/

	if ( file[0] == '\0' || strncmp(file, lastfile, PATHNAMESIZE) != STREQUAL )
	{
		if ( fp != NULL )
		{
			Debug((4, "GetNextCmd() => close file"));
			(void)fclose(fp);
			fp = NULL;
		}

		if ( file[0] == '\0' )
			return 0;
	}

	Trace((2, "GetNextCmd(%s) TransferSucceeded=%d", file, (int)TransferSucceeded));

	if ( fp == NULL )
	{
		if ( strncmp(file, lastfile, PATHNAMESIZE) == 0 )
		{
			Debug((1,"Workfilename repeated: %s", file));
			return 0;
		}

		(void)strncpy(lastfile, file, PATHNAMESIZE);

		while ( (fp = fopen(file, "r+w")) == NULL )
		{
			char	rqstr[PATHNAMESIZE];

			(void)sprintf(rqstr, "%s%s", InvokeDir, file);
			if ( SysWarn(CouldNot, WriteStr, rqstr) )
				continue;
			if ( (cp = strrchr(file, '/')) != NULLSTR )
				++cp;
			else
				cp = file;
			(void)sprintf(rqstr, "%s%s", CORRUPTDIR, cp);
			MoveCp(file, rqstr);
			return 0;
		}

		nstr[0] = '\0';
		nextread = nextwrite = 0L;
	}

	if ( nstr[0] != '\0' && TransferSucceeded )
	{
		(void)fseek(fp, nextwrite, 0);
		(void)fputs(nstr, fp);
		(void)fseek(fp, nextread, 0);
	}

	do 
	{
		nextwrite = ftell(fp);

		if ( fgets(str, MAXRQST, fp) == NULL )
		{
			(void)fclose(fp);
			fp = NULL;
			if ( TransferSucceeded )
				(void)unlink(file);
			file[0] = '\0';
			nstr[0] = '\0';
			return 0;
		}
	}
	while
		( !isupper(str[0]) );

	nextread = ftell(fp);

	(void)strncpy(nstr, str, MAXRQST);
	nstr[0] = tolower(nstr[0]);

	TransferSucceeded = false;

	Trace((4, "cmd: %s", str));

	return GetCmdArgs(str, wvec, size);
}

/*
**      Check if name == directory.
*/

static bool
isdir(
	char *	name
)
{
	struct stat	s;

	if ( stat(name, &s) == SYSERROR )
		return false;

	if ( (s.st_mode & S_IFMT) == S_IFDIR )
		return true;

	return false;
}

/*
**	Return last name in path.
*/

char *
lastpart(
	register char *	str
)
{
	register char *	cp;

	if ( str == NULLSTR || str[0] == '\0' )
		return EmptyStr;

	if ( (cp = strrchr(str, '/')) != NULLSTR )
		return ++cp;

	return str;
}

/*
**	Mail receiver of arrived file.
*/

static void
mail_arrived(
	char *	file,
	char *	nuser,
	char *	rmtsys,
	char *	rmtuser
)
{
	char *	notice;

	notice = newprintf("%s from %s!%s arrived\n", file, rmtsys, rmtuser);

	Mail(NULLVFUNCP, nuser, notice);

	free(notice);
}

/*
**	Send user mail about error.
*/

static void
mail_err(
	char *	user,
	char *	file,
	char *	mesg
)
{
	char *	notice;

	notice = newprintf("file %s!%s -- %s\n", NODENAME, file, mesg);

	Mail(NULLVFUNCP, user, notice);

	free(notice);
}

/*
**	Mail results of command.
*/

static void
mail_notice(
	int	mailopt,
	char *	user,
	char *	file,
	char *	sys,
	char *	msgcode
)
{
	char *	msg;
	char *	notice;

	if ( !mailopt && *msgcode == 'Y' )
		return;

	if ( *msgcode == 'Y' )
		msg = "copy succeeded";
	else
		msg = ExplainMesg(&msgcode[1]);

	notice = newprintf("file %s!%s -- %s\n", sys, file, msg);

	Mail(NULLVFUNCP, user, notice);

	free(notice);
}

/*
**	Generate temporary receive filename in `Dfile'.
*/

static void
MakeTmp(
	char	c
)
{
	register int	i;
	register char *	cp;
	static int	count;

/**	(void)sprintf(Dfile, "TM.%05d.%03d", Pid, count++);	**/

	cp = Dfile;
	*cp++ = c;

	/*
	**	`count' wraps every VC_size, so update time.
	*/

	if ( count >= VC_size )
	{
		Timeusec++;
		count = 0;
	}

	/*
	**	Next 5 chars reflect seconds.
	*/

	(void)EncodeNum(cp, (Ulong)Time, 5);

	/*
	**	Next 4 chars reflect micro seconds.
	*/

	(void)EncodeNum(cp+5, (Ulong)Timeusec, 4);

	/*
	**	Next 3 chars reflect process ID.
	*/

	(void)EncodeNum(cp+5+4, (Ulong)Pid, 3);

	/*
	**	Last 1 chars increment every call.
	*/

	(void)EncodeNum(cp+5+4+3, (Ulong)count, 1);

	cp[5+4+3+1] = '\0';
	count++;
}

/*
**	Choose a protocol from `str' that matches available.
*/

static char
MatchProtocols(
	register char *	str
)
{
	register Proto *p;

	Trace((
		1,
		"MatchProtocols(%s) [SysProtocols=%s, LineType=%s, IsTCP=%d, IsX25=%d]",
		str,
		SysProtocols,
		LineType,
		IsTCP,
		IsX25
	));

	for ( p = Ptbl ; p->P_id != '\0' ; p++ )
	{
		if ( p->P_id == 't' && !IsTCP )
			continue;	/* Only use 't' on TCP/IP */

#ifdef	X25_PAD
#ifdef	USE_X25
		/* Allow using 'f' on 'non-X25' connections, */
		/* if X25_PAD is defined and USE_X25 is not */
		if ( p->P_id == 'f' && !IsX25 )
			continue;	/* Only use 'f' on X25_PAD */
#endif	/* USE_X25 */
#endif	/* X25_PAD */

		if
		(
			strchr(str, p->P_id) != NULLSTR
			&&
			(
				SysProtocols == NULLSTR
				||
				strchr(SysProtocols, p->P_id) != NULLSTR
			)
		)
			return p->P_id;
	}

	return '\0';
}

/*
**	Change status and programme name send/receive.
*/

static void
newstatus(
	Role	role,
	char	mode
)
{
	Ulong	rate;
	char *	k;
	char	modestr[32];

	if
	(
		(rate = StartTime) == 0
		||
		(rate = Time-rate) == 0
		||
		(rate = (Stats.byts_sent+Stats.byts_rcvd)/rate) == 0
	)
		rate = Baud/10;

	if ( rate > 9999 )
	{
		rate /= 1024;
		k = "k";
	}
	else
		k = EmptyStr;

	(void)sprintf
	(
		modestr, "%s %s %3lu %4lu%s",
		role ? "MST" : "SLV",
		(mode==SNDFILE) ? "SND" : "RCV",
		Stats.msgs_sent+Stats.msgs_rcvd,
		rate, k
	);

	name_prog(modestr);

	if ( send_or_receive == mode )
		return;

	send_or_receive = mode;

	(void)sprintf
	(
		modestr, "%s %s",
		role ? MasterStr : SlaveStr,
		(mode==SNDFILE) ? SENDING : RECEIVING
	);

	(void)Status(SS_INPROGRESS, RemoteNode, modestr);
}

/*
**	Null ACU function.
*/

static CallType
null_func()
{
	return SUCCESS;
}

/*
**	Want to create an orphan uuxqt,
**	so a double-fork is needed.
*/

static void
orphan_uuxqt()
{
	int	pid;

	Debug((1, "Start uuxqt (X. file count = %d)", nXfiles));

	if ( (pid = fork()) == 0 )
	{
		Uuxqt(RemoteNode);
		_exit(0);
	}

	if ( pid != SYSERROR )
	{
		(void)wait((int *)0);
		nXfiles = 0;
	}
}

/*
**	Put file in public place.
**	If successful, filename is modified.
*/

static bool
putinpub(
	char *	file,
	char *	tmp,
	char *	user
)
{
	char	fullname[PATHNAMESIZE];
	bool	ok;
	int	save;

	Trace((2, "putinpub(%s, %s, %s)", file, tmp, user));

	(void)sprintf(fullname, "%s%s/", PUBDIR, user);

	save = DIR_MODE;
	DIR_MODE = 0777;	/* Public write needed for these */

	if ( !MkDirs(fullname, NetUid, NetGid) )
	{
		Debug((1, "Cannot mkdirs(%s)", fullname));
		DIR_MODE = save;
		return false;
	}

	DIR_MODE = save;

	(void)strcat(fullname, lastpart(file));
	
	if ( ok = MoveCp(tmp, fullname) )
	{
		(void)strcpy(file, fullname);
		(void)chmod(file, FILE_MODE);
	}

	return ok;
}

/*
**	Read message 'c'.
**	Try 'n' times.
*/

static bool
receive_msg(
	char			c,
	register char *		msg,
	register int		n
)
{
	char			cstr[4];

	static char		got[] = "got %s";

	if ( (cstr[0] = c) == '\0' )
		strcpy(cstr, "ANY");
	else
		cstr[1] = '\0';

	Debug((4, "receive_msg '%s'", cstr));

	msg[0] = '\0';	/* In case FAIL */

	while ( (*Rdmsg)(msg, Ifd) != SUCCESS )
	{
		if ( --n > 0 )
		{
			Log("PATIENCE %d", n);
			continue;
		}

		Debug((4, got, FAILED));

		Log
		(
			"BAD READ %s expected '%s' got FAIL (%s)",
			TtyName==NULLSTR?EmptyStr:TtyName,
			cstr,
			ErrnoStr(errno)
		);

		(*Turnoff)();
		return false;
	}

	if ( c != '\0' && msg[0] != c )
	{
		Debug((4, got, msg));

		Log
		(
			"BAD READ %s expected '%s' got %s",
			TtyName==NULLSTR?EmptyStr:TtyName,
			cstr,
			msg
		);

		(*Turnoff)();
		return false;
	}

	Debug((4, got, msg));
	return true;
}

/*
**	Report rate for transfer.
*/

void
ReportRate(
	Rate		dir,
	TimeBuf *	start,
	long		databytes,
	long		circuitbytes,
	int		retries
)
{
	TimeBuf		now;
	long		bps;
	float		fsecs;
	char		text[128];

	(void)SetTimes();
	now = TimeNow;

	now.secs -= start->secs;
	if ( now.usecs < start->usecs )
	{
		--now.secs;
		now.usecs += 1000000;
	}
	fsecs = (float)(now.secs) + (now.usecs - start->usecs)/1000000.0;

	if ( fsecs == 0.0 )
		bps = databytes * 8;
	else
		bps = (databytes * 8.0) / fsecs;

	(void)sprintf
	(
		text,
		"%s data %ld bytes %.2f secs %ld bps",
		(dir==SENT)?"sent":"received",
		databytes,
		fsecs,
		bps
	);

	sysacct(circuitbytes, now.secs);

	if ( dir == SENT )
	{
		Stats.byts_sent += databytes;
		Stats.msgs_sent++;
	}
	else
	{
		Stats.byts_rcvd += databytes;
		Stats.msgs_rcvd++;
	}

	if ( databytes > 10000 )	/* `bps' is only accurate on big files */
	{
		Stats.bps_count++;
		Stats.bps_total += bps;
	}

	if ( retries > 0 )
		(void)sprintf(&text[strlen(text)], " %d retries", retries);

	Debug((1, text));
	LogStats(text);
}

/* 
**	Force re-scan for work files.
*/

static void
rescan()
{
	if ( strncmp(InvokeDir, SPOOLDIR, SpoolDirLen) == STREQUAL )
		return;	/* No need */

	FindFlush();
	CreateActive(false);
	InvokeDir = ChNodeDir(RemoteNode, NULLSTR);
	CreateActive(true);
}

/*
**	Write a message (type `m').
*/

static bool
send_msg(
	char	m,
	char *	s
)
{
	Debug((4, "send_msg '%c' <%s>", m, s));

	if ( (*Wrmsg)(m, s, Ofd) != SUCCESS )
	{
		(*Turnoff)();
		return false;
	}

	return true;
}

/*
**	Optimize line setting for sending or receiving files.
*/

static void
setupline(
	char			type
)
{
#ifdef USE_TERMIOS
	struct termios		tbuf;
	static struct termios	sbuf;
	static int		set;

	if (!Is_a_tty)
		return;

	Debug((2, "setline - %c", type));

	switch (type) {
	case SNDFILE:
		break;

	case RCVFILE:
		(void)tcgetattr(Ifd, &tbuf);
		sbuf = tbuf;
		tbuf.c_cc[VMIN] = PACKSIZE;
		(void)tcsetattr(Ifd, TCSADRAIN, &tbuf);
		set++;
		break;

	case RESET:
		if (set == 0)
			break;
		set = 0;
		(void)tcsetattr(Ifd, TCSADRAIN, &sbuf);
		break;
	}
#endif	/* USE_TERMIOS */
}

/*
**	This routine will set up the six routines
**	(Rdmsg, Wrmsg, Rddata, Wrdata, Turnon, Turnoff)
**	for the desired protocol.
*/

static CallType
StartProtocol(
	register char	c
)
{
	register Proto *p;
	static char	fmt[]	= "Protocol start%s '%c'";

	Trace((1, "StartProtocol(%c)", c));

	for ( p = Ptbl ; p->P_id != '\0' ; p++ )
	{
		if ( c == p->P_id )
		{
			Rdmsg = p->P_rdmsg;
			Wrmsg = p->P_wrmsg;
			Rddata = p->P_rddata;
			Wrdata = p->P_wrdata;
			Turnon = p->P_turnon;
			Turnoff = p->P_turnoff;

			if ( (*Turnon)() != SUCCESS )
				break;

			Protocol = c;

			Debug((1, fmt, "ed", c));
			return SUCCESS;
		}
	}

	Debug((1, fmt, "-fail", c));
	return FAIL;
}

/*
**	Converse with the remote machine, agree upon a protocol (if possible)
**	and start the protocol.
*/

CallType
Startup(
	Role	role
)
{
	char	str[BUFSIZ];	/* (Minimum size for receive_msg()) */

	Rdmsg = dflt_rd_msg;
	Wrmsg = dflt_wr_msg;

	Trace((1, "Startup(%s)", (role==MASTER)?MasterStr:SlaveStr));

	if ( role == MASTER )
	{
		if ( !receive_msg(SLTPTCL, str, 1) )
			return FAIL;

		if ( (str[1] = MatchProtocols(&str[1])) == '\0' )
		{
			/** No protocol match **/

			(void)send_msg(USEPTCL, NOT);
			return FAIL;
		}

		str[2] = '\0';
		if ( !send_msg(USEPTCL, &str[1]) )
			return FAIL;
	}
	else
	{
		if ( !send_msg(SLTPTCL, BuildProtocols(str)) )
			return FAIL;

		if ( !receive_msg(USEPTCL, str, 1) )
			return FAIL;

		if ( str[1] == 'N' )
			return FAIL;
	}

	if ( StartProtocol(str[1]) != 0 )
		return FAIL;

	return SUCCESS;
}

/*
**	Unlink D. file.
*/

static void
unlinkdf(
	char *	file
)
{
	if ( strlen(file) <= (3+SEQLEN) )
		return;

	(void)unlink(file);
}

/*
**	Update `last turned' time.
*/

static void
update_turntime()
{
	(void)SetTimes();
	LastTurned = Time;
}

/*
**	Deal with `wrong role' error.
*/

static void
wrong_role(
	char *	desc
)
{
	(*Turnoff)();

	if ( nXfiles )
		orphan_uuxqt();

	ErrVal = EX_PROTOCOL;
	Error("Wrong Role - %s", desc);
}
