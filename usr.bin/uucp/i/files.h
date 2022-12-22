/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Formats for various uucp files.
**
**	RCSID $Id: files.h,v 1.2 1994/01/31 01:25:45 donn Exp $
**
**	$Log: files.h,v $
 * Revision 1.2  1994/01/31  01:25:45  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:22  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:51  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:08:35  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/04/17  21:40:38  piers
 * add SysKeepDebug, exported from lib/SysFile.c
 * remove CallOK -- now internal to uusched and uucico
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

/*
**	SYSFILE/ALIASFILE
*/

extern char *	SysDetails;		/* Parameters for found node if needed */

/*
**	Skip-list cache for details.
**
**	Use probabilistic skip list for list insertion, with P = 1/4.
*/

#define	MAXLEVEL	9	/* 9 => log4(256K) */
#define	MINLEVEL	5	/* Fix dice after this level */

typedef enum { unk_t, sys_t, alias_t }	Syst;

typedef struct sysl	Sysl;
struct sysl
{
	char *	name;		/* Node */
	char *	val;		/* Details */
	bool	free;		/* `val' has been `malloc'ed */
	Syst	type;		/* Alias, or node */
	Sysl *	next[MAXLEVEL];	/* Must be last */
};

extern Sysl	SysHead;

/**	Fields			**/

#define	SYS_NAME	0
#undef	SYS_TIME
#define	SYS_TIME	1
#define	SYS_LINE	2
#define	SYS_CLASS	3	/* [prefix]speed */
#define	SYS_PHONE	4
#define	SYS_LOGIN	5

#define	F_TIME		(SYS_TIME-1)
#define	F_LINE		(SYS_LINE-1)
#define	F_CLASS		(SYS_CLASS-1)
#define	F_PHONE		(SYS_PHONE-1)
#define	F_LOGIN		(SYS_LOGIN-1)

#define MAXFIELDS	100	/* Max. fields in SysDetails */
#define	MAXPH		64	/* Max. bytes in telno. */

typedef enum { NOSYS, NEEDSYS }		Nst;
typedef enum { NOALIAS, NEEDALIAS }	Nat;
typedef enum { Master, Slave }		Mst;

/**	Call fail reasons	**/

typedef enum {
	CF_OK		=  0,
	CF_SYSTEM	= -1,
	CF_TIME		= -2,
	CF_LOCK		= -3,
	CF_NODEV	= -4,
	CF_DIAL		= -5,
	CF_LOGIN	= -6,
	CF_CHAT		= -7
}
		CallType;

#define	FAIL	CF_SYSTEM
#define	SUCCESS	CF_OK

/*
**	USERFILE
*/

#define	MAXUSERFIELDS	50

/*
**	Call status files.
**
**	XXX - keep ../uusnap/uusnap.pl up to date wrt this.
*/

typedef enum {
	SS_OK,
	SS_NODEVICE,
	SS_CALLBACK,
	SS_INPROGRESS,
	SS_FAIL,
	SS_BADSEQ,
	SS_WRONGTIME,
	SS_CHATFAILED,
	SS_CALLING,
	SS_CLOSE	/* Close status file */
}
		StatusType;

/*
**	File name types.
*/

#define CMD_TYPE	'C'
#define DATA_TYPE	'D'
#define EXEC_TYPE	'X'
#define	DUMMY_NAME	"D.0"

typedef enum { FINDFILE, CHECKFILE, ONEPASS } Find_t;

/*
**	Command execution.
*/

#define	X_NONOTI	'N'
#define	X_NONZERO	'Z'
#define X_CMD		'C'
#define X_RETURNTO	'R'
#define X_RQDFILE	'F'
#define X_SENDFILE	'S'
#define X_STDIN		'I'
#define X_STDOUT	'O'
#define X_USER		'U'

/*
**	Locks.
*/

#define	D_LOCK		"UUCPD"
#define X_LOCK		"XQT"
#define X_LOCKTIME	3600L
#define X_UUSCHED	"UUSCHED"

/*
**	Variables set from TIME field.
*/

extern int	SysDebug;
extern bool	SysKeepDebug;
extern int	SysPktSize;
extern char *	SysProtocols;
extern int	SysRetryTime;
extern int	SysWindows;

/*
**	Functions.
*/

extern bool	CallBack(char *);
extern CallType	CheckFieldTime(char *);
extern CallType	CheckSysTime(char *, Mst);
extern void	CommitSeq(void);
extern char *	FindFile(char *, char *, Find_t);
extern bool	FindSlaveGrade(int, char *);
extern bool	FindSysEntry(char **, Nst, Nat);
extern int	GetNxtSeq(char *);
extern char *	GetSysNode(char **, char **);
extern void	LoadSys(Nst);
extern Sysl *	LookupSys(char *);
extern bool	ScanSys(char **, Nst, Nat);
extern void	UnlockSeq(void);
extern void	WalkSys(void(*)(char *, char *, Syst));

#ifndef	NO_VA_FUNC_DECLS
extern bool	Status(StatusType, ...);
#else	/* !NO_VA_FUNC_DECLS */
extern bool	Status();
#endif	/* !NO_VA_FUNC_DECLS */
