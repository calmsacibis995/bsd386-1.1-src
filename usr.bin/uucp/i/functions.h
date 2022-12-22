/*	BSDI $Id: functions.h,v 1.4 1994/01/31 01:25:47 donn Exp $	*/

/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	C-library definitions.
**
**	RCSID $Id: functions.h,v 1.4 1994/01/31 01:25:47 donn Exp $
**
**	$Log: functions.h,v $
 * Revision 1.4  1994/01/31  01:25:47  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:22  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:51  vixie
 * Initial revision
 *
 * Revision 1.3  1993/12/19  23:18:16  donn
 * Use libutil setproctitle() routine.
 *
 * Revision 1.2  1993/02/28  15:27:41  pace
 * Add recent uunet changes; make site.h more generic; add uuxqthook support.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:34  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#ifndef __bsdi__

extern double	atof(char *);
extern long	atol(char *);
extern char *	bsearch(char *, char *, int, int, int(*)(char *, char*));
extern char *	getenv(char *);
extern char *	malloc(int);
extern char *	qsort(char *, int, int, int(*)(char *, char*));
extern long	random(void);
extern void	srandom(int);
extern char *	strcat(char *, char *);
extern char *	strchr(char *, char);
extern int	strncmp(char *, char *, int);
extern char *	strncpy(char *, char *, int);
extern char *	strpbrk(char *, char *);
extern char *	strrchr(char *, char);
extern int	strspn(char *, char *);
extern char *	strstr(char *, char *);

#if	__GNUC__ >= 2
extern int	strcmp(const char *, const char *);
extern char *	strcpy(char *, const char *);
extern int	strlen(const char *);
#else	/* __GNUC__ >= 2 */
extern int	strcmp(char *, char *);
extern char *	strcpy(char *, char *);
extern int	strlen(char *);
#endif	/* __GNUC__ >= 2 */

#else

#include <stdlib.h>
#include <string.h>

#if defined(__bsdi__) && (_BSDI_VERSION >= 199312)
#define	NameProg	setproctitle
#endif

#endif

/*
**	Uucp-library definitions.
*/

#ifdef	STDIO
extern FILE *	ErrorFd;
#endif	/* STDIO */

#ifndef	NO_VA_FUNC_DECLS
extern char *	concat(char *, char *, char *, ...);
extern void	Error(char *, ...);
extern void	ErrorLog(char *, ...);
extern void	ErrorLogN(char *, ...);
extern void	Fatal(char *, ...);
extern void	Log(char *, ...);
extern void	LogStats(char *, ...);
extern void	MesgN(char *, ...);
extern void	MesgV(char *, va_list);
#if !defined(__bsdi__) || (_BSDI_VERSION < 199312)
extern void	setproctitle(char *, ...);
#endif
extern char *	newprintf(char *, ...);
extern char *	newvprintf(char *, ...);
extern void	QueueCmd(char *, ...);
extern void	SysError(char *, ...);
extern bool	SysWarn(char *, ...);
extern void	Warn(char *, ...);
#else	/* !NO_VA_FUNC_DECLS */
extern char *	concat();
extern void	Error();
extern void	ErrorLog();
extern void	ErrorLogN();
extern void	Fatal();
extern void	Log();
extern void	LogStats();
extern void	MesgN();
extern void	MesgV();
extern void	NameProg();
extern char *	newprintf();
extern char *	newvprintf();
extern void	QueueCmd();
extern void	SysError();
extern bool	SysWarn();
extern void	Warn();
#endif	/* !NO_VA_FUNC_DECLS */

extern bool	CheckDirs(char *);
extern void	Checkeuid(void);
extern bool	CheckUserPath(char *, char *, char *);
extern char *	ChNodeDir(char *, char *);
extern bool	CopyFdToFd(int, char *, int, char *, Ulong *);
extern bool	CopyFdToFile(int, char *, char *, Ulong *);
extern bool	CopyFileToFd(char *, int, char *, Ulong *);
extern bool	CopyFileToFile(char *, char *);
extern int	Create(char *);
extern int	CreateN(char *, bool);
extern int	CreateR(char *);
extern int	CreateWf(char *);
extern Ulong	DecodeNum(char *, int);
extern int	EncodeNum(char *, Ulong, int);
extern char *	ErrnoStr(int);
extern bool	ErrorLogV(char *, va_list);
extern bool	ErrorTty(int *);
extern void	ExpandArgs(VarArgs *, char **);
extern char *	ExpandString(char *, int);
extern bool	ExpFn(char *);
extern void	FindFlush(void);
extern void	finish(int);
extern void	FreeStr(char **);
extern bool	FSFree(char *, Ulong);
extern char *	GetLine(char **);
extern bool	GetUid(int *, char **, char **);
extern bool	GetUser(int *, char **, char **);
extern int	HashName(char *, int);
extern void	InitVC(void);
extern char *	ISODateTimeStr(Time_t, Time_t, char *);
extern void	listsort(char **, int(*)(char *, char*));
extern void	Link(char *, char *);
extern void	LogClose(void);
extern int	LogFd(void);
extern bool	LookupAlias(char **);
extern char *	Mail(vFuncp, char *, char *);
extern char *	Malloc(int);
extern char *	Malloc0(int);
extern bool	MkDirs(char *, int, int);
extern bool	mklock(char *);
extern void	Move(char *, char *);
extern bool	MoveCp(char *, char *);
extern bool	MoveN(char *, char *);
extern void	NewCount(void);
extern char *	newnstr(char *, int);
extern char *	newstr(char *);
extern char *	NextArg(char *, char *);
extern char *	NodeName(void);
extern long	otol(char *);
extern char *	ReadFd(int);
extern char *	ReadFile(char *);
extern void	ResetErrTty(void);
extern void	RestoreUid(void);
extern void	rmlock(char *);
#if defined(__bsdi__) && (_BSDI_VERSION < 199312)
extern void	SetNameProg(int, char **, char **);
#endif
extern void	SetNetUid(void);
extern bool	SetTimes(void);
extern void	Set_ALTDIRS(void);
extern void	Set_SPOOLDIR(char *);
extern int	SplitArgs(VarArgs *, char *);
extern bool	SplitSysName(char *, char **, char *);
extern void	StdError(void);
extern char *	strclr(char *, int);
extern char **	StripEnv(char **);
extern char *	StripErrString(char *);
extern char *	StripStatusString(char *);
extern char *	strcpyend(char *, char *);
extern char *	strncpyend(char *, char *, int);
extern bool	SysRetry(int);
extern void	touchlock(void);
extern char *	UniqueWf(char **, char, char, char *, char *);
extern void	UnlinkAllWf(void);
extern void	Uucico(char *, Wait);
extern char *	UUCPDateTimeStr(Time_t, char *);
extern void	Uuxqt(char *);
extern void	Uusched(void);
extern void	WfFree(void);
extern char *	WfName(char, char, char *);
extern char *	WfPath(char *, char, char *);
extern int	WriteCmds(void);
extern long	xtol(char *);

/*
**	String definitions.
*/

extern char	ChdirStr[];
extern char	CouldNot[];
extern char	CreateStr[];
extern char	DevNull[];
extern char	DupStr[];
extern char	EmptyStr[];
extern char	ForkStr[];
extern char	LockStr[];
extern char	NullStr[];
extern char	OpenStr[];
extern char	PATH[];
extern char	PipeStr[];
extern char	ReadStr[];
extern char	SeekStr[];
extern char	Slash[];
extern char	StatStr[];
extern char	Unknown[];
extern char	UnlinkStr[];
extern char	Version[];
extern char	WriteStr[];

/*
**	Global variables.
*/

extern bool	ErrIsatty;
extern char *	ErrString;
extern int	ErrVal;
extern int	E_uid;
extern int	E_gid;
extern long	FreeFSpace;
extern int	FQmax;
extern char *	HomeDir;
extern bool	IsattyDone;
extern char *	InvokeDir;
extern char *	LastField;
extern char *	LogNode;
extern Ulong	MaxSeq;
extern int	MaxWorkFiles;
extern char *	Name;
extern int	NetGid;
extern int	NetUid;
extern char **	NewEnvs;
extern int	NodeNameLen;
extern bool	NoLog;
extern int	Pid;
extern int	RdFileSize;
extern Time_t	RdFileTime;
extern int	R_uid;
extern char *	SenderName;
extern int	SEQLEN;
extern char *	SourceAddress;
extern int	SpoolDirLen;
extern int	SpoolAltDirs;
extern char *	SysDetails;
extern TimeBuf	TimeNow;
extern char *	Invoker;
extern char *	UserName;
extern char *	ValidChars;
extern int	VC_size;
