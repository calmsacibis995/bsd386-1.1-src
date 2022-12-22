/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Declare variables used for configurable parameters.
**
**	Intialise with default values from "site.h".
**
**	RCSID $Id: strings.c,v 1.2 1993/02/28 15:28:50 pace Exp $
**
**	$Log: strings.c,v $
 * Revision 1.2  1993/02/28  15:28:50  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:36  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"site.h"
#include	"typedefs.h"

#define	english(S)	S
#define	NULLSTR		(char *)0

/*
**	Parameters (also referenced in InitParams.c).
*/

static char *Anonusers_A[]	= {ANON_USERS,NULLSTR};
static char *Loginmm_A[]	= {LOGIN_MUST_MATCH,NULLSTR};
static char *Mailprog_A[]	= {MAILPROGARGS,NULLSTR};
static char *Nostrangers_A[]	= {NOSTRANGERS,NULLSTR};
static char *Spoolaltdirs_A[]	= {SPOOLALTDIRS,NULLSTR};
static char *Uucico_A[]		= {UUCICOARGS,NULLSTR};
static char *Uucp_A[]		= {UUCPARGS,NULLSTR};
static char *Uusched_A[]	= {UUSCHEDARGS,NULLSTR};
static char *Uux_A[]		= {UUXARGS,NULLSTR};
static char *Uuxqt_A[]		= {UUXQTARGS,NULLSTR};

char *	ActivefileStr		= ACTIVEFILE;
char *	AliasfileStr		= ALIASFILE;
int	Allacuinout		= ALLACUINOUT;
char **	AnonusersStr		= Anonusers_A;
int	AxsRmtFiles		= ACCESS_REMOTE_FILES;
char *	CmdsfileStr		= CMDSFILE;
int	Cmdtimeout		= CMDTIMEOUT;
char *	CnnctacctfileStr	= CNNCT_ACCT_FILE;
char *	CorruptdirStr		= CORRUPTDIR;
int	Deadlocktime		= DEADLOCKTIME;
int	Dfltcicoturn		= DEFAULT_CICO_TURN;
char *	DevfileStr		= DEVFILE;
char *	DialfileStr		= DIALFILE;
char *	DialinoutStr		= DIALINOUT;
int	Dirchecktime		= DIRCHECKTIME;
int	Dirmode			= DIR_MODE;
char *	ExplaindirStr		= EXPLAINDIR;
int	Filemode		= FILE_MODE;
char *	LockdirStr		= LOCKDIR;
int	Lockmode		= LOCK_MODE;
int	Locknameisdev		= LOCKNAMEISDEV;
int	Lockpidisstr		= LOCKPIDISSTR;
char *	LockpreStr		= LOCKPRE;
int	LogBadSysName		= LOG_BAD_SYS_NAME;
char *	LogdirStr		= LOGDIR;
char **	LoginmustmatchStr	= Loginmm_A;
int	Logmode			= LOG_MODE;
char *	MailprogStr		= MAILPROG;
char **	MailprogargsStr		= Mailprog_A;
int	Maxuuscheds		= MAXUUSCHEDS;
int	Maxxqts			= MAXXQTS;
int	Minspoolfsfree		= MINSPOOLFSFREE;
char *	NeeduuxqtStr		= NEEDUUXQT;
char *	NodeNameStr		= NODENAME;
char *	NodesqfileStr		= NODESEQFILE;
char *	NodesqlockStr		= NODESEQLOCK;
char *	NodesqtempStr		= NODESEQTEMP;
char *	NologinStr		= NOLOGIN;
char **	NostrangersStr		= Nostrangers_A;
int	OrigUUCP		= ORIG_UUCP;
char *	ParamsdirStr		= PARAMSDIR;
char *	ParamsfileStr		= PARAMSFILE;
char *	ProgdirStr		= PROGDIR;
char *	ProgparamsdirStr	= PROG_PARAMSDIR;
char *	PubdirStr		= PUBDIR;
char *	ShellStr		= SHELL;
char **	SpoolaltdirsStr		= Spoolaltdirs_A;
char *	SpooldirStr		= SPOOLDIR;
char *	SeqfileStr		= SEQFILE;
char *	StatusdirStr		= STATUSDIR;
char *	StrangerscmdStr		= STRANGERSCMD;
char *	SysfileStr		= SYSFILE;
char *	TelnetdStr		= TELNETD;
char *	TmpdirStr		= TMPDIR;
char *	UserfileStr		= USERFILE;
char *	UucicoStr		= UUCICO;
char **	UucicoargsStr		= Uucico_A;
int	Uucico_only		= UUCICO_ONLY;
char *	UucpStr			= UUCP;
char **	UucpargsStr		= Uucp_A;
char *	UucpgroupStr		= UUCPGROUP;
char *	UucpnameStr		= UUCPUSER;
char *	UuschedStr		= UUSCHED;
char **	UuschedargsStr		= Uusched_A;
char *	UuxStr			= UUX;
char **	UuxargsStr		= Uux_A;
char *	UuxqtStr		= UUXQT;
char **	UuxqtargsStr		= Uuxqt_A;
char *	UuxqtHookStr		= UUXQTHOOK;
int	WarnNameTooLong		= WARN_NAME_TOO_LONG;
int	WorkFileMask		= WORK_FILE_MASK;
int	VerifyTcpAddress	= VERIFY_TCP_ADDRESS;
char *	XqtdirStr		= XQTDIR;

/*
**	Special cases.
*/

static
char	CopyRight[]		= english("@(#) Copyright 1991,'92 UUNET Technologies Inc.");

char	ChdirStr[]		= english("chdir");
char	CouldNot[]		= english("Could not %s \"%s\"");
char	CreateStr[]		= english("create");
char	DevNull[]		= "/dev/null";
char	DupStr[]		= english("dup");
char	EmptyStr[]		= "";
char	ForkStr[]		= english("fork");
char	LockStr[]		= english("lock");
char	NullStr[]		= english("<null>");
char	OpenStr[]		= english("open");
char	PipeStr[]		= english("pipe");
char	ReadStr[]		= english("read");
char	SeekStr[]		= english("seek");
char	Slash[]			= "/";
char	StatStr[]		= english("stat");
char	StringFmt[]		= "%.1023s";
char	Unknown[]		= english("<unknown>");
char	UnlinkStr[]		= english("unlink");
char	WriteStr[]		= english("write");

/*
**	Globals.
*/

char *	ErrString;
