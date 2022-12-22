/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	String definitions.
**
**	Redefine strings from "site.h" to point to variables.
**
**	Variables are declared in "lib/strings.c".
**
**	RCSID $Id: strings.h,v 1.2 1993/02/28 15:27:45 pace Exp $
**
**	$Log: strings.h,v $
 * Revision 1.2  1993/02/28  15:27:45  pace
 * Add recent uunet changes; make site.h more generic; add uuxqthook support.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:34  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#undef	ACCESS_REMOTE_FILES
#define	ACCESS_REMOTE_FILES	AxsRmtFiles
extern int		AxsRmtFiles;

#undef	ACTIVEFILE
#define	ACTIVEFILE	ActivefileStr
extern char *		ActivefileStr;

#undef	ALIASFILE
#define	ALIASFILE	AliasfileStr
extern char *		AliasfileStr;

#undef	ALLACUINOUT
#define	ALLACUINOUT	Allacuinout
extern int		Allacuinout;

#undef	ANON_USERS
#define	ANON_USERS	AnonusersStr
extern char **		AnonusersStr;

#undef	CMDSFILE
#define	CMDSFILE	CmdsfileStr
extern char *		CmdsfileStr;

#undef	CMDTIMEOUT
#define	CMDTIMEOUT	Cmdtimeout
extern int		Cmdtimeout;

#undef	CNNCT_ACCT_FILE
#define	CNNCT_ACCT_FILE	CnnctacctfileStr
extern char *		CnnctacctfileStr;

#undef	CORRUPTDIR
#define	CORRUPTDIR	CorruptdirStr
extern char *		CorruptdirStr;

#undef	DEADLOCKTIME
#define	DEADLOCKTIME	Deadlocktime
extern int		Deadlocktime;

#undef	DEFAULT_CICO_TURN
#define	DEFAULT_CICO_TURN	Dfltcicoturn
extern int		Dfltcicoturn;

#undef	DEVFILE
#define	DEVFILE		DevfileStr
extern char *		DevfileStr;

#undef	DIALFILE
#define	DIALFILE	DialfileStr
extern char *		DialfileStr;

#undef	DIALINOUT
#define	DIALINOUT	DialinoutStr
extern char *		DialinoutStr;

#undef	DIRCHECKTIME
#define	DIRCHECKTIME	Dirchecktime
extern int		Dirchecktime;

#undef	DIR_MODE
#define	DIR_MODE	Dirmode
extern int		Dirmode;

#undef	EXPLAINDIR
#define	EXPLAINDIR	ExplaindirStr
extern char *		ExplaindirStr;

#undef	FILE_MODE
#define	FILE_MODE	Filemode
extern int		Filemode;

#undef	LOCK_MODE
#define	LOCK_MODE	Lockmode
extern int		Lockmode;

#undef	LOCKDIR
#define	LOCKDIR		LockdirStr
extern char *		LockdirStr;

#undef	LOCKNAMEISDEV
#define	LOCKNAMEISDEV	Locknameisdev
extern int		Locknameisdev;

#undef	LOCKPIDISSTR
#define	LOCKPIDISSTR	Lockpidisstr
extern int		Lockpidisstr;

#undef	LOCKPRE
#define	LOCKPRE		LockpreStr
extern char *		LockpreStr;

#undef	LOGDIR
#define	LOGDIR		LogdirStr
extern char *		LogdirStr;

#undef	LOGIN_MUST_MATCH
#define	LOGIN_MUST_MATCH	LoginmustmatchStr
extern char **		LoginmustmatchStr;

#undef	LOG_MODE
#define	LOG_MODE	Logmode
extern int		Logmode;

#undef	LOG_BAD_SYS_NAME
#define	LOG_BAD_SYS_NAME	LogBadSysName
extern int		LogBadSysName;

#undef	MAILPROG
#define	MAILPROG	MailprogStr
extern char *		MailprogStr;

#undef	MAILPROGARGS
#define	MAILPROGARGS	MailprogargsStr
extern char **		MailprogargsStr;

#undef	MAXUUSCHEDS
#define	MAXUUSCHEDS	Maxuuscheds
extern int		Maxuuscheds;

#undef	MAXXQTS
#define	MAXXQTS		Maxxqts
extern int		Maxxqts;

#undef	MINSPOOLFSFREE
#define	MINSPOOLFSFREE	Minspoolfsfree
extern int		Minspoolfsfree;

#undef	NODENAME
#define	NODENAME	NodeNameStr
extern char *		NodeNameStr;

#undef	NODESEQFILE
#define	NODESEQFILE	NodesqfileStr
extern char *		NodesqfileStr;

#undef	NODESEQLOCK
#define	NODESEQLOCK	NodesqlockStr
extern char *		NodesqlockStr;

#undef	NODESEQTEMP
#define	NODESEQTEMP	NodesqtempStr
extern char *		NodesqtempStr;

#undef	NOLOGIN
#define	NOLOGIN		NologinStr
extern char *		NologinStr;

#undef	NEEDUUXQT
#define	NEEDUUXQT	NeeduuxqtStr
extern char *		NeeduuxqtStr;

#undef	NOSTRANGERS
#define	NOSTRANGERS	NostrangersStr
extern char **		NostrangersStr;

#undef	ORIG_UUCP
#define	ORIG_UUCP	OrigUUCP
extern int		OrigUUCP;

#undef	PARAMSDIR
#define	PARAMSDIR	ParamsdirStr
extern char *		ParamsdirStr;

#undef	PARAMSFILE
#define	PARAMSFILE	ParamsfileStr
extern char *		ParamsfileStr;

#undef	PROGDIR
#define	PROGDIR		ProgdirStr
extern char *		ProgdirStr;

#undef	PROG_PARAMSDIR
#define	PROG_PARAMSDIR	ProgparamsdirStr
extern char *		ProgparamsdirStr;

#undef	PUBDIR
#define	PUBDIR		PubdirStr
extern char *		PubdirStr;

#undef	SHELL
#define	SHELL		ShellStr
extern char *		ShellStr;

#undef	SPOOLDIR
#define	SPOOLDIR	SpooldirStr
extern char *		SpooldirStr;

#undef	SPOOLALTDIRS
#define	SPOOLALTDIRS	SpoolaltdirsStr
extern char **		SpoolaltdirsStr;

#undef	SEQFILE
#define	SEQFILE		SeqfileStr
extern char *		SeqfileStr;

#undef	STATUSDIR
#define	STATUSDIR	StatusdirStr
extern char *		StatusdirStr;

#undef	STRANGERSCMD
#define	STRANGERSCMD	StrangerscmdStr
extern char *		StrangerscmdStr;

#undef	SYSFILE
#define	SYSFILE		SysfileStr
extern char *		SysfileStr;

#undef	TELNETD
#define	TELNETD		TelnetdStr
extern char *		TelnetdStr;

#undef	TMPDIR
#define	TMPDIR		TmpdirStr
extern char *		TmpdirStr;

#undef	USERFILE
#define	USERFILE	UserfileStr
extern char *		UserfileStr;

#undef	UUCICO
#define	UUCICO		UucicoStr
extern char *		UucicoStr;

#undef	UUCICOARGS
#define	UUCICOARGS	UucicoargsStr
extern char **		UucicoargsStr;

#undef	UUCP
#define	UUCP		UucpStr
extern char *		UucpStr;

#undef	UUCPARGS
#define	UUCPARGS	UucpargsStr
extern char **		UucpargsStr;

#undef	UUCICO_ONLY
#define	UUCICO_ONLY	Uucico_only
extern int		Uucico_only;

#undef	UUCPGROUP
#define	UUCPGROUP	UucpgroupStr
extern char *		UucpgroupStr;

#undef	UUCPUSER
#define	UUCPUSER	UucpnameStr
extern char *		UucpnameStr;

#undef	UUSCHED
#define	UUSCHED		UuschedStr
extern char *		UuschedStr;

#undef	UUSCHEDARGS
#define	UUSCHEDARGS	UuschedargsStr
extern char **		UuschedargsStr;

#undef	UUX
#define	UUX		UuxStr
extern char *		UuxStr;

#undef	UUXARGS
#define	UUXARGS		UuxargsStr
extern char **		UuxargsStr;

#undef	UUXQT
#define	UUXQT		UuxqtStr
extern char *		UuxqtStr;

#undef	UUXQTARGS
#define	UUXQTARGS	UuxqtargsStr
extern char **		UuxqtargsStr;

#undef	UUXQTHOOK
#define	UUXQTHOOK	UuxqtHookStr
extern char *		UuxqtHookStr;

#undef	VERIFY_TCP_ADDRESS
#define	VERIFY_TCP_ADDRESS	VerifyTcpAddress
extern int		VerifyTcpAddress;

#undef	WARN_NAME_TOO_LONG
#define	WARN_NAME_TOO_LONG	WarnNameTooLong
extern int		WarnNameTooLong;

#undef	WORK_FILE_MASK
#define	WORK_FILE_MASK	WorkFileMask
extern int		WorkFileMask;

#undef	XQTDIR
#define	XQTDIR		XqtdirStr
extern char *		XqtdirStr;
