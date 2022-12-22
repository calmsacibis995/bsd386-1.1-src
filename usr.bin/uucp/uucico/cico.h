/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Common data for uucico routines.
**
**	RCSID $Id: cico.h,v 1.4 1994/01/31 01:26:43 donn Exp $
**
**	$Log: cico.h,v $
 * Revision 1.4  1994/01/31  01:26:43  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.3  1993/02/28  15:36:33  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.2  1992/11/20  19:40:04  trent
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
 * Revision 1.1.1.1  1992/09/28  20:08:53  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/04/17  21:55:10  piers
 * add message counting to stats (use in name_prog())
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

/*
**	Defaults.
*/

#define	CONV_COMPLETE	"CONVERSATION COMPLETE"
#define	DFLTTTYNAME	"notty"
#define	DFLT_BAUD	9600		/* If unknown */
#define	MAX_MESG_CHARS	256		/* Max chars in 1st reply */
#define	MAXMSGTIME	45		/* Max. secs. to wait for 1st reply */
#define	MAXRQST		250		/* Max. bytes in a command */
#define	TRYCALLS	2		/* Number of tries to dial call */

/*
**	This structure tells about a device.
*/

#define	MAXDEVARGS	20
#define	MAXDEVCHARS	168

typedef struct
{
	int	D_nargs;
	int	D_speed;
	char *	D_arg[MAXDEVARGS];
	char	D_buf[MAXDEVCHARS];
}
		Device;

/** Fields from DEVFILE **/

#define	D_type		D_arg[0]
#define	D_line		D_arg[1]
#define	D_calldev	D_arg[2]
#define	D_class		D_arg[3]
#define	D_brand		D_arg[4]
#define	D_CHAT		5

/*
**	This structure tells how to get to a device.
*/

typedef struct
{
	char *	CU_meth;				/* method, such as 'ACU' or 'DIR' */
	char *	CU_brand;				/* brand, such as 'Hayes' or 'Vadic' */
	CallType (*CU_gen)(char **);			/* what to call to search for brands */
	CallType (*CU_open)(char *, char **, Device *);	/* what to call to open brand */
	CallType (*CU_clos)(int);			/* what to call to close brand */
}
		ConDev;

extern ConDev	condevs[];

/*
**	Baud / kernel speed table structure.
*/

typedef struct
{
	Ulong	baud;		/* Bits/second */
	Ulong	speed;		/* Kernel number */
}
		Speed;

/*
**	Meaning of `Role'.
*/

typedef enum { SLAVE, MASTER } Role;

/*
**	Statistics.
*/

typedef struct
{
	Ulong	byts_sent;
	Ulong	byts_rcvd;
	Ulong	bps_count;	/* NB: `bits-per-second' */
	Ulong	bps_total;
	Ulong	msgs_sent;
	Ulong	msgs_rcvd;
}
		Stat;

typedef enum { SENT, RECEIVED }	Rate;

/*
**	Active file structure.
*/

typedef struct
{
	char	pid[11];		/* Process ID (10 digits, '\n') */
	char	node[64-11];		/* Node on which process is active */
}
	ActFile;

/*
**	Miscellaneous common variables.
*/

extern jmp_buf	AlarmJmp;		/* SIGALRM */
extern Ulong	Baud;			/* Bits/second for VC */
extern bool	CheckForWork;		/* If no files to send, give up */
extern Role	CurrentRole;		/* Current behaviour */
extern CallType	(*CU_end)();
extern int	DebugRemote;		/* Remote end debug level */
extern char	DefMaxGrade;		/* Default job priority */
extern char	devSel[];		/* Device used by Connect() */
extern char	FAILED[];		/* ``FAILED'' */
extern char	FailMsg[];		/* For failure reasons from ACU library */
extern bool	HaveSentHup;		/* Hangup message sent */
#ifdef SPRINT_HACK
extern Time_t	HangupTime;		/* Time we must hangup a 900 call */
extern bool	Is900;			/* Circuit is on 900 line */
#endif /* SPRINT_HACK */
extern int	Ifd;			/* Input side of virtual circuit */
extern bool	IgnoreTimeToCall;	/* Call again */
extern Role	InitialRole;		/* MASTER/SLAVE */
extern char *	IPHostName;		/* IP circuit remote address */
extern bool	IsTCP;			/* Virtual circuit via TCP/IP */
extern bool	IsX25;			/* Circuit is via X.25 */
extern bool	Is_a_tty;		/* Virtual circuit has `tty' semantics */
extern bool	KeepAudit;		/* Keep time-stamped audit */
extern bool	LocalOnly;		/* Only call LOCAL, DIR, or TCP sites */
extern char *	LoginName;		/* Invoker's name */
extern char	LineType[];		/* Set by Connect() */
extern char	MasterStr[];		/* ``MASTER'' */
extern char	MaxGrade;		/* Job priority */
extern int	NextFd;			/* Predicted fd to close interrupted opens */
extern int	Ofd;			/* Output side of virtual circuit */
extern char	Protocol;		/* Selected protocol */
extern char *	RemoteNode;		/* Particular host to be called */
extern bool	RemoteDebug;		/* Debug enabled from remote */
extern bool	ReverseRole;		/* Caller begins by receiving  */
extern char	SlaveStr[];		/* ``SLAVE'' */
extern Speed	Speeds[];		/* Table of kernel speeds */
extern Time_t	StartTime;		/* Comms. start time */
extern Stat	Stats;			/* Transmission statistics */
extern char *	TtyDevice;		/* Name of virtual circuit device, if known */
extern char *	TtyName;		/* Name of virtual circuit, if known */
extern int	TurnTime;		/* Turn time between role swaps */
extern char *	VCName;			/* Virtual circuit name */

/*
**	Common subroutines.
*/

extern StatusType	CallOK(char *);

extern Uint	chksum(char *, int);
extern void	cico_args(int, char **);
extern void	CloseACU();
extern int	Connect(void);
extern CallType	Control(Role);
extern void	CreateActive(bool);
extern CallType	dochat(Device *, char **, int);
extern void	do_cico(void);
extern CallType	expect(char *, int);
extern void	ExpandTelno(char *, char *);
extern char *	fdig(char *);
extern char *	findsys(void);
extern bool	find_work(void);
extern void	fioclex(int);
extern void	getnextfd(void);
extern bool	InMesg(int, char *, int);
extern void	name_prog(char *);
extern bool	NextDevice(char **, Device *);
extern int	OutMesg(char, char *, int);
extern void	Reenable(void);
extern void	ReportRate(Rate, TimeBuf *, long, long, int);
extern void	SendBRK(int, int);
extern bool	SetupTty(int, int);
extern CallType	Startup(Role);
extern void	Timeout(int);
extern int	Uucp(char *, char *);
extern int	open_dev(char *);
extern int	close_dev(int);
extern void	drop_dtr(int);

#include "uu_termios.h"
