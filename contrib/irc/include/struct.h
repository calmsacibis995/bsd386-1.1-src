/*	BSDI	$Id: struct.h,v 1.1.1.1 1994/01/13 21:15:35 polk Exp $	*/
/************************************************************************
 *   IRC - Internet Relay Chat, include/struct.h
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef	__struct_include__
#define __struct_include__

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef __bsdi__
#undef BSD
#include <netdb.h>
#else
#include <netdb.h>
#endif
#if defined(SGI) || defined(AIX)
# include <stddef.h>
#endif

#ifdef USE_SYSLOG
# include <syslog.h>
# ifndef HPUX
#  include <sys/syslog.h>
# endif
#endif

typedef	struct	ConfItem aConfItem;
typedef	struct 	Client	aClient;
typedef	struct	Channel	aChannel;
typedef	struct	User	anUser;
typedef	struct	Server	aServer;
typedef	struct	SLink	Link;
typedef	struct	SMode	Mode;

#ifndef VMSP
#include "class.h"
#include "dbuf.h"	/* THIS REALLY SHOULDN'T BE HERE!!! --msa */
#endif

#define	HOSTLEN		63	/* Length of hostname.  Updated to         */
				/* comply with RFC1123                     */

#define	NICKLEN		9	/* Necessary to put 9 here instead of 10
				** if s_msg.c/m_nick has been corrected.
				** This preserves compatibility with old
				** servers --msa
				*/
#define	USERLEN		10
#define	REALLEN	 	50
#define	TOPICLEN	80
#define	CHANNELLEN	200
#define	PASSWDLEN 	20
#define	KEYLEN		23
#define	BUFSIZE		512		/* WARNING: *DONT* CHANGE THIS!!!! */
#define	MAXRECIPIENTS 	20
#define	MAXBANS		20
#define	MAXBANLENGTH	1024

#define	USERHOST_REPLYLEN	(NICKLEN+HOSTLEN+USERLEN+5)

#ifdef USE_SERVICES
#include "service.h"
#endif

/*
** 'offsetof' is defined in ANSI-C. The following definition
** is not absolutely portable (I have been told), but so far
** it has worked on all machines I have needed it. The type
** should be size_t but...  --msa
*/
#ifndef offsetof
#define	offsetof(t,m) (int)((&((t *)0L)->m))
#endif

#define	elementsof(x) (sizeof(x)/sizeof(x[0]))

/*
** flags for bootup options (command line flags)
*/
#define	BOOT_CONSOLE	1
#define	BOOT_QUICK	2
#define	BOOT_DEBUG	4
#define	BOOT_INETD	8
#define	BOOT_TTY	16
#define	BOOT_OPER	32
#define	BOOT_AUTODIE	64

#define	STAT_LOG	-6	/* logfile for -x */
#define	STAT_MASTER	-5	/* Local ircd master before identification */
#define	STAT_CONNECTING	-4
#define	STAT_HANDSHAKE	-3
#define	STAT_ME		-2
#define	STAT_UNKNOWN	-1
#define	STAT_SERVER	0
#define	STAT_CLIENT	1
#define	STAT_SERVICE	2	/* Services not implemented yet */

/*
 * status macros.
 */
#define	IsRegisteredUser(x)	((x)->status == STAT_CLIENT)
#define	IsRegistered(x)		((x)->status >= STAT_SERVER)
#define	IsConnecting(x)		((x)->status == STAT_CONNECTING)
#define	IsHandshake(x)		((x)->status == STAT_HANDSHAKE)
#define	IsMe(x)			((x)->status == STAT_ME)
#define	IsUnknown(x)		((x)->status == STAT_UNKNOWN || \
				 (x)->status == STAT_MASTER)
#define	IsServer(x)		((x)->status == STAT_SERVER)
#define	IsClient(x)		((x)->status == STAT_CLIENT)
#define	IsLog(x)		((x)->status == STAT_LOG)
#define	IsService(x)		((x)->status == STAT_SERVICE)

#define	SetMaster(x)		((x)->status = STAT_MASTER)
#define	SetConnecting(x)	((x)->status = STAT_CONNECTING)
#define	SetHandshake(x)		((x)->status = STAT_HANDSHAKE)
#define	SetMe(x)		((x)->status = STAT_ME)
#define	SetUnknown(x)		((x)->status = STAT_UNKNOWN)
#define	SetServer(x)		((x)->status = STAT_SERVER)
#define	SetClient(x)		((x)->status = STAT_CLIENT)
#define	SetLog(x)		((x)->status = STAT_LOG)
#define	SetService(x)		((x)->status = STAT_SERVICE)

#define	FLAGS_PINGSENT   0x0001	/* Unreplied ping sent */
#define	FLAGS_DEADSOCKET 0x0002	/* Local socket is dead--Exiting soon */
#define	FLAGS_KILLED     0x0004	/* Prevents "QUIT" from being sent for this */
#define	FLAGS_OPER       0x0008	/* Operator */
#define	FLAGS_LOCOP      0x0010 /* Local operator -- SRB */
#define	FLAGS_INVISIBLE  0x0020 /* makes user invisible */
#define	FLAGS_WALLOP     0x0040 /* send wallops to them */
#define	FLAGS_SERVNOTICE 0x0080 /* server notices such as kill */
#define	FLAGS_BLOCKED    0x0100	/* socket is in a blocked condition */
#define	FLAGS_UNIX	 0x0200	/* socket is in the unix domain, not inet */
#define	FLAGS_CLOSING    0x0400	/* set when closing to suppress errors */
#define	FLAGS_LISTEN     0x0800 /* used to mark clients which we listen() on */
#define	FLAGS_CHKACCESS  0x1000 /* ok to check clients access if set */
#define	FLAGS_DOINGDNS	 0x2000 /* client is waiting for a DNS response */
#define	FLAGS_AUTH	 0x4000 /* client is waiting on rfc931 response */
#define	FLAGS_WRAUTH	 0x8000	/* set if we havent writen to ident server */
#define	FLAGS_LOCAL	0x10000 /* set for local clients */
#define	FLAGS_GOTID	0x20000	/* successful ident lookup achieved */
#define	FLAGS_DOID	0x40000	/* I-lines say must use ident return */
#define	FLAGS_NONL	0x80000 /* No \n in buffer */

#define	SEND_UMODES	(FLAGS_INVISIBLE|FLAGS_OPER|FLAGS_WALLOP)
#define	ALL_UMODES	(SEND_UMODES|FLAGS_SERVNOTICE)

/*
 * flags macros.
 */
#define	IsOper(x)		((x)->flags & FLAGS_OPER)
#define	IsLocOp(x)		((x)->flags & FLAGS_LOCOP)
#define	IsInvisible(x)		((x)->flags & FLAGS_INVISIBLE)
#define	IsAnOper(x)		((x)->flags & (FLAGS_OPER|FLAGS_LOCOP))
#define	IsPerson(x)		IsClient(x)
#define	IsPrivileged(x)		(IsAnOper(x) || IsServer(x))
#define	SendWallops(x)		((x)->flags & FLAGS_WALLOP)
#define	SendServNotice(x)	((x)->flags & FLAGS_SERVNOTICE)
#define	IsUnixSocket(x)		((x)->flags & FLAGS_UNIX)
#define	IsListening(x)		((x)->flags & FLAGS_LISTEN)
#define	DoAccess(x)		((x)->flags & FLAGS_CHKACCESS)
#define	IsLocal(x)		((x)->flags & FLAGS_LOCAL)
#define	IsDead(x)		((x)->flags & FLAGS_DEADSOCKET)

#define	SetOper(x)		((x)->flags |= FLAGS_OPER)
#define	SetLocOp(x)    		((x)->flags |= FLAGS_LOCOP)
#define	SetInvisible(x)		((x)->flags |= FLAGS_INVISIBLE)
#define	SetWallops(x)  		((x)->flags |= FLAGS_WALLOP)
#define	SetUnixSock(x)		((x)->flags |= FLAGS_UNIX)
#define	SetDNS(x)		((x)->flags |= FLAGS_DOINGDNS)
#define	DoingDNS(x)		((x)->flags & FLAGS_DOINGDNS)
#define	SetAccess(x)		((x)->flags |= FLAGS_CHKACCESS)
#define	DoingAuth(x)		((x)->flags & FLAGS_AUTH)
#define	NoNewLine(x)		((x)->flags & FLAGS_NONL)

#define	ClearOper(x)		((x)->flags &= ~FLAGS_OPER)
#define	ClearInvisible(x)	((x)->flags &= ~FLAGS_INVISIBLE)
#define	ClearWallops(x)		((x)->flags &= ~FLAGS_WALLOP)
#define	ClearDNS(x)		((x)->flags &= ~FLAGS_DOINGDNS)
#define	ClearAuth(x)		((x)->flags &= ~FLAGS_AUTH)
#define	ClearAccess(x)		((x)->flags &= ~FLAGS_CHKACCESS)

/*
 * defined debugging levels
 */
#define	DEBUG_FATAL  0
#define	DEBUG_ERROR  1	/* report_error() and other errors that are found */
#define	DEBUG_NOTICE 3
#define	DEBUG_DNS    4	/* used by all DNS related routines - a *lot* */
#define	DEBUG_INFO   5	/* general usful info */
#define	DEBUG_NUM    6	/* numerics */
#define	DEBUG_SEND   7	/* everything that is sent out */
#define	DEBUG_DEBUG  8	/* anything to do with debugging, ie unimportant :) */
#define	DEBUG_MALLOC 9	/* malloc/free calls */
#define	DEBUG_LIST  10	/* debug list use */

/*
 * defines for curses in client
 */
#define	DUMMY_TERM	0
#define	CURSES_TERM	1
#define	TERMCAP_TERM	2

struct	ConfItem	{
	unsigned int	status;	/* If CONF_ILLEGAL, delete when no clients */
	int	clients;	/* Number of *LOCAL* clients using this */
	struct	in_addr ipnum;	/* ip number of host field */
	char	*host;
	char	*passwd;
	char	*name;
	int	port;
	time_t	hold;	/* Hold action until this time (calendar time) */
#ifndef VMSP
	aClass	*class;  /* Class of connection */
#endif
	struct	ConfItem *next;
};

#define	CONF_ILLEGAL            0x80000000
#define	CONF_QUARANTINED_SERVER 0x0001
#define	CONF_CLIENT             0x0002
#define	CONF_CONNECT_SERVER     0x0004
#define	CONF_NOCONNECT_SERVER   0x0008
#define	CONF_LOCOP              0x0010
#define	CONF_OPERATOR           0x0020
#define	CONF_ME                 0x0040
#define	CONF_KILL               0x0080
#define	CONF_ADMIN              0x0100
#ifdef R_LINES
#define	CONF_RESTRICT           0x0200
#endif
#define	CONF_CLASS              0x0400
#define	CONF_SERVICE            0x0800
#define	CONF_LEAF		0x1000
#define	CONF_LISTEN_PORT	0x2000
#define	CONF_HUB		0x4000

#define	CONF_OPS		(CONF_OPERATOR | CONF_LOCOP)
#define	CONF_SERVER_MASK	(CONF_CONNECT_SERVER | CONF_NOCONNECT_SERVER)
#define	CONF_CLIENT_MASK	(CONF_CLIENT | CONF_SERVICE | CONF_OPS | \
				 CONF_SERVER_MASK)

#define	IsIllegal(x)	((x)->status & CONF_ILLEGAL)

/*
 * Client structures
 */
struct	User	{
	struct	User	*nextu;
	Link	*channel;	/* chain of channel pointer blocks */
	Link	*invited;	/* chain of invite pointer blocks */
	char	*away;		/* pointer to away message */
	time_t	last;
	int	refcnt;		/* Number of times this block is referenced */
	int	joined;		/* number of channels joined */
	char	username[USERLEN+1];
	char	host[HOSTLEN+1];
        char	server[HOSTLEN+1];
				/*
				** In a perfect world the 'server' name
				** should not be needed, a pointer to the
				** client describing the server is enough.
				** Unfortunately, in reality, server may
				** not yet be in links while USER is
				** introduced... --msa
				*/
#ifdef	LIST_DEBUG
	aClient	*bcptr;
#endif
};

struct	Server	{
	struct	Server	*nexts;
	anUser	*user;		/* who activated this connection */
	char	up[HOSTLEN+1];	/* uplink for this server */
	char	by[NICKLEN+1];
	aConfItem *nline;	/* N-line pointer for this server */
#ifdef	LIST_DEBUG
	aClient	*bcptr;
#endif
};

struct Client	{
	struct	Client *next,*prev, *hnext;
	anUser	*user;		/* ...defined, if this is a User */
	aServer	*serv;		/* ...defined, if this is a server */
#ifdef USE_SERVICES
	aService *service;
#endif
	time_t	lasttime;	/* ...should be only LOCAL clients? --msa */
	time_t	firsttime;	/* time client was created */
	time_t	since;		/* last time we parsed something */
	long	flags;		/* client flags */
	aClient	*from;		/* == self, if Local Client, *NEVER* NULL! */
	int	fd;		/* >= 0, for local clients */
	int	hopcount;	/* number of servers to this 0 = local */
	short	status;		/* Client type */
	char	name[HOSTLEN+1]; /* Unique name of the client, nick or host */
	char	username[USERLEN+1]; /* username here now for auth stuff */
	char	info[REALLEN+1]; /* Free form additional client information */
	/*
	** The following fields are allocated only for local clients
	** (directly connected to *this* server with a socket.
	** The first of them *MUST* be the "count"--it is the field
	** to which the allocation is tied to! *Never* refer to
	** these fields, if (from != self).
	*/
	int	count;		/* Amount of data in buffer */
	char	buffer[BUFSIZE]; /* Incoming message buffer */
	short	lastsq;		/* # of 2k blocks when sendqueued called last*/
	dbuf	sendQ;		/* Outgoing message queue--if socket full */
	dbuf	recvQ;		/* Hold for data incoming yet to be parsed */
	long	sendM;		/* Statistics: protocol messages send */
	long	sendB;		/* Statistics: total bytes send */
	long	receiveM;	/* Statistics: protocol messages received */
	long	receiveB;	/* Statistics: total bytes received */
	aClient	*acpt;		/* listening client which we accepted from */
	Link	*confs;		/* Configuration record associated */
	int	authfd;		/* fd for rfc931 authentication */
	struct	in_addr	ip;	/* keep real ip# too */
	unsigned short	port;	/* and the remote port# too :-) */
	struct	hostent	*hostp;
	char	sockhost[HOSTLEN+1]; /* This is the host name from the socket
				  ** and after which the connection was
				  ** accepted.
				  */
	char	passwd[PASSWDLEN+1];
};

#define	CLIENT_LOCAL_SIZE sizeof(aClient)
#define	CLIENT_REMOTE_SIZE offsetof(aClient,count)

/*
 * statistics structures
 */
struct	stats {
	unsigned int	is_cl;	/* number of client connections */
	unsigned int	is_sv;	/* number of server connections */
	unsigned int	is_ni;	/* connection but no idea who it was */
	unsigned long	is_cbs;	/* bytes sent to clients */
	unsigned long	is_cbr;	/* bytes received to clients */
	unsigned long	is_sbs;	/* bytes sent to servers */
	unsigned long	is_sbr;	/* bytes received to servers */
	time_t 		is_cti;	/* time spent connected by clients */
	time_t		is_sti;	/* time spent connected by servers */
	unsigned int	is_ac;	/* connections accepted */
	unsigned int	is_ref;	/* accepts refused */
	unsigned int	is_unco; /* unknown commands */
	unsigned int	is_wrdi; /* command going in wrong direction */
	unsigned int	is_unpf; /* unknown prefix */
	unsigned int	is_empt; /* empty message */
	unsigned int	is_num;	/* numeric message */
	unsigned int	is_kill; /* number of kills generated on collisions */
	unsigned int	is_fake; /* MODE 'fakes' */
	unsigned int	is_asuc; /* successful auth requests */
	unsigned int	is_abad; /* bad auth requests */
	unsigned int	is_udp;	/* packets recv'd on udp port */
	unsigned int	is_loc;	/* local connections made */
};

/* mode structure for channels */

struct	SMode	{
	unsigned int	mode;
	int	limit;
	char	key[KEYLEN+1];
};

/* Message table structure */

struct	Message	{
	char	*cmd;
	int	(* func)();
	unsigned int	count;
	int	parameters;
	char	flags;
		/* bit 0 set means that this command is allowed to be used
		 * only on the average of once per 2 seconds -SRB */
	unsigned long	bytes;
};

/* general link structure used for chains */

struct	SLink	{
	struct	SLink	*next;
	union {
		aClient	*cptr;
		aChannel *chptr;
		aConfItem *aconf;
		char	*cp;
	} value;
	int	flags;
};

/* channel structure */

struct Channel	{
	struct	Channel *nextch, *prevch, *hnextch;
	Mode	mode;
	char	topic[TOPICLEN+1];
	int	users;
	Link	*members;
	Link	*invites;
	Link	*banlist;
	char	chname[1];
};

/*
** Channel Related macros follow
*/

/* Channel related flags */

#define	CHFL_CHANOP     0x0001 /* Channel operator */
#define	CHFL_VOICE      0x0002 /* the power to speak */
#define	CHFL_BAN	0x0004 /* ban channel flag */

/* Channel Visibility macros */

#define	MODE_CHANOP	CHFL_CHANOP
#define	MODE_VOICE	CHFL_VOICE
#define	MODE_PRIVATE	0x0004
#define	MODE_SECRET	0x0008
#define	MODE_MODERATED  0x0010
#define	MODE_TOPICLIMIT 0x0020
#define	MODE_INVITEONLY 0x0040
#define	MODE_NOPRIVMSGS 0x0080
#define	MODE_KEY	0x0100
#define	MODE_BAN	0x0200
#define	MODE_LIMIT	0x0400
#define MODE_FLAGS	0x07ff
/*
 * mode flags which take another parameter (With PARAmeterS)
 */
#define	MODE_WPARAS	(MODE_CHANOP|MODE_VOICE|MODE_BAN|MODE_KEY|MODE_LIMIT)
/*
 * Undefined here, these are used in conjunction with the above modes in
 * the source.
#define	MODE_DEL       0x40000000
#define	MODE_ADD       0x80000000
 */

#define	HoldChannel(x)		(!(x))
/* name invisible */
#define	SecretChannel(x)	((x) && ((x)->mode.mode & MODE_SECRET))
/* channel not shown but names are */
#define	HiddenChannel(x)	((x) && ((x)->mode.mode & MODE_PRIVATE))
/* channel visible */
#define	ShowChannel(v,c)	(PubChannel(c) || IsMember((v),(c)))
#define	PubChannel(x)		((!x) || ((x)->mode.mode &\
				 (MODE_PRIVATE | MODE_SECRET)) == 0)

#define	IsMember(user,chan) (find_user_link((chan)->members,user) ? 1 : 0)
#define	IsChannelName(name) ((name) && (*(name) == '#' || *(name) == '&'))

/* Misc macros */

#define	BadPtr(x) (!(x) || (*(x) == '\0'))

#define	isvalid(c) (((c) >= 'A' && (c) <= '~') || isdigit(c) || (c) == '-')

#define	MyConnect(x)			((x)->fd >= 0)
#define	MyClient(x)			(MyConnect(x) && IsClient(x))
#define	MyOper(x)			(MyConnect(x) && IsOper(x))

/* String manipulation macros */

/* strncopynt --> strncpyzt to avoid confusion, sematics changed
   N must be now the number of bytes in the array --msa */
#define	strncpyzt(x, y, N) do{(void)strncpy(x,y,N);x[N-1]='\0';}while(0)
#define	StrEq(x,y)	(!strcmp((x),(y)))

/* used in SetMode() in channel.c and m_umode() in s_msg.c */

#define	MODE_NULL      0
#define	MODE_ADD       0x40000000
#define	MODE_DEL       0x20000000

/* return values for hunt_server() */

#define	HUNTED_NOSUCH	(-1)	/* if the hunted server is not found */
#define	HUNTED_ISME	0	/* if this server should execute the command */
#define	HUNTED_PASS	1	/* if message passed onwards successfully */

/* used when sending to #mask or $mask */

#define	MATCH_SERVER  1
#define	MATCH_HOST    2

/* used for async dns values */

#define	ASYNC_NONE	(-1)
#define	ASYNC_CLIENT	0
#define	ASYNC_CONNECT	1
#define	ASYNC_CONF	2
#define	ASYNC_SERVER	3

/* misc variable externs */

extern	char	*version, *infotext[];
extern	char	*generation, *creation;

/* misc defines */

#define	FLUSH_BUFFER	-2
#define	UTMP		"/etc/utmp"
#define	COMMA		","

/* IRC client structures */

#ifdef	CLIENT_COMPILE
typedef	struct	Ignore {
	char	user[NICKLEN+1];
	char	from[USERLEN+HOSTLEN+2];
	int	flags;
	struct	Ignore	*next;
} anIgnore;

#define	IGNORE_PRIVATE	1
#define	IGNORE_PUBLIC	2
#define	IGNORE_TOTAL	3

#define	HEADERLEN	200

#endif

#endif /* __struct_include__ */
