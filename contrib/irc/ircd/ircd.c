/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd.c
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

#ifndef lint
static	char sccsid[] = "@(#)ircd.c	2.46 10 Oct 1993 (C) 1988 University of Oulu, \
Computing Center and Jarkko Oikarinen";
#endif

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include "h.h"

aClient me;			/* That's me */
aClient *client = &me;		/* Pointer to beginning of Client list */

void	server_reboot();
void	restart PROTO((char *));
static	void	open_debugfile(), setup_signals();

char	**myargv;
int	portnum = -1;		    /* Server port number, listening this */
char	*configfile = CONFIGFILE;	/* Server configuration file */
int	debuglevel = -1;		/* Server debug level */
int	bootopt = 0;			/* Server boot option flags */
char	*debugmode = "";		/*  -"-    -"-   -"-  */
static	int	dorehash = 0;
static	char	*dpath = DPATH;

time_t	nextconnect = 1;	/* time for next try_connections call */
time_t	nextping = 1;		/* same as above for check_pings() */
time_t	nextdnscheck = 0;	/* next time to poll dns to force timeouts */
time_t	nextexpire = 1;	/* next expire run on the dns cache */

#ifdef	PROFIL
extern	etext();

VOIDSIG	s_monitor()
{
	static	int	mon = 0;
#ifdef	POSIX_SIGNALS
	struct	sigaction act;
#endif

	(void)moncontrol(mon);
	mon = 1 - mon;
#ifdef	POSIX_SIGNALS
	act.sa_handler = s_rehash;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGUSR1);
	(void)sigaction(SIGUSR1, &act, NULL);
#else
	(void)signal(SIGUSR1, s_monitor);
#endif
}
#endif

VOIDSIG s_die()
{
#ifdef	USE_SYSLOG
	(void)syslog(LOG_CRIT, "Server Killed By SIGTERM");
#endif
	flush_connections(me.fd);
	exit(-1);
}

static VOIDSIG s_rehash()
{
#ifdef	POSIX_SIGNALS
	struct	sigaction act;
#endif
	dorehash = 1;
#ifdef	POSIX_SIGNALS
	act.sa_handler = s_rehash;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
#else
	(void)signal(SIGHUP, s_rehash);	/* sysV -argv */
#endif
}

void	restart(mesg)
char	*mesg;
{
#ifdef	USE_SYSLOG
	(void)syslog(LOG_WARNING, "Restarting Server because: %s",mesg);
#endif
	server_reboot();
}

VOIDSIG s_restart()
{
	static int restarting = 0;

#ifdef	USE_SYSLOG
	(void)syslog(LOG_WARNING, "Server Restarting on SIGINT");
#endif
	if (restarting == 0)
	    {
		/* Send (or attempt to) a dying scream to oper if present */

		restarting = 1;
		server_reboot();
	    }
}

void	server_reboot()
{
	Reg1	int	i;

	sendto_ops("Aieeeee!!!  Restarting server...");
	Debug((DEBUG_NOTICE,"Restarting server..."));
	flush_connections(me.fd);
	/*
	** fd 0 must be 'preserved' if either the -d or -i options have
	** been passed to us before restarting.
	*/
#ifdef USE_SYSLOG
	(void)closelog();
#endif
	for (i = 3; i < MAXCONNECTIONS; i++)
		(void)close(i);
	if (!(bootopt & (BOOT_TTY|BOOT_DEBUG)))
		(void)close(2);
	(void)close(1);
	if ((bootopt & BOOT_CONSOLE) || isatty(0))
		(void)close(0);
	if (!(bootopt & (BOOT_INETD|BOOT_OPER)))
		(void)execv(MYNAME, myargv);
#ifdef USE_SYSLOG
	/* Have to reopen since it has been closed above */

	openlog(myargv[0], LOG_PID|LOG_NDELAY, LOG_FACILITY);
	syslog(LOG_CRIT, "execv(%s,%s) failed: %m\n", MYNAME, myargv[0]);
	closelog();
#endif
	Debug((DEBUG_FATAL,"Couldn't restart server: %s", strerror(errno)));
	exit(-1);
}


/*
** try_connections
**
**	Scan through configuration and try new connections.
**	Returns the calendar time when the next call to this
**	function should be made latest. (No harm done if this
**	is called earlier or later...)
*/
static	time_t	try_connections(currenttime)
time_t	currenttime;
{
	Reg1	aConfItem *aconf;
	Reg2	aClient *cptr;
	aConfItem **pconf;
	int	connecting, confrq;
	time_t	next = 0;
	aClass	*cltmp;
	aConfItem *con_conf;
	int	con_class = 0;

	connecting = FALSE;
	Debug((DEBUG_NOTICE,"Connection check at   : %s",
		myctime(currenttime)));
	for (aconf = conf; aconf; aconf = aconf->next )
	    {
		/* Also when already connecting! (update holdtimes) --SRB */
		if (!(aconf->status & CONF_CONNECT_SERVER) || aconf->port <= 0)
			continue;
		cltmp = Class(aconf);
		/*
		** Skip this entry if the use of it is still on hold until
		** future. Otherwise handle this entry (and set it on hold
		** until next time). Will reset only hold times, if already
		** made one successfull connection... [this algorithm is
		** a bit fuzzy... -- msa >;) ]
		*/

		if ((aconf->hold > currenttime))
		    {
			if ((next > aconf->hold) || (next == 0))
				next = aconf->hold;
			continue;
		    }

		confrq = get_con_freq(cltmp);
		aconf->hold = currenttime + confrq;
		/*
		** Found a CONNECT config with port specified, scan clients
		** and see if this server is already connected?
		*/
		cptr = find_name(aconf->name, (aClient *)NULL);

		if (!cptr && (Links(cltmp) < MaxLinks(cltmp)) &&
		    (!connecting || (Class(cltmp) > con_class)))
		    {
			con_class = Class(cltmp);
			con_conf = aconf;
			/* We connect only one at time... */
			connecting = TRUE;
		    }
		if ((next > aconf->hold) || (next == 0))
			next = aconf->hold;
	    }
	if (connecting)
	    {
		if (con_conf->next)  /* are we already last? */
		    {
			for (pconf = &conf; (aconf = *pconf);
			     pconf = &(aconf->next))
				/* put the current one at the end and
				 * make sure we try all connections
				 */
				if (aconf == con_conf)
					*pconf = aconf->next;
			(*pconf = con_conf)->next = 0;
		    }
		if (connect_server(con_conf, (aClient *)NULL,
				   (struct hostent *)NULL) == 0)
			sendto_ops("Connection to %s[%s] activated.",
				   con_conf->name, con_conf->host);
	    }
	Debug((DEBUG_NOTICE,"Next connection check : %s", myctime(next)));
	return (next);
}

static	time_t	check_pings(currenttime)
time_t	currenttime;
{		
	Reg1	aClient	*cptr;
	Reg2	int	killflag;
	int	ping = 0, i, rflag = 0;
	time_t	oldest = 0, timeout;

	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]) || IsMe(cptr) || IsLog(cptr))
			continue;

		/*
		** Note: No need to notify opers here. It's
		** already done when "FLAGS_DEADSOCKET" is set.
		*/
		if (cptr->flags & FLAGS_DEADSOCKET)
		    {
			(void)exit_client(cptr, cptr, &me, "Dead socket");
			i = 0;
			continue;
		    }

		killflag = IsPerson(cptr) ? find_kill(cptr) : 0;
#ifdef R_LINES_OFTEN
		rflag = IsPerson(cptr) ? find_restrict(cptr) : 0;
#endif
		ping = get_client_ping(cptr);
		if (!IsRegistered(cptr))
			ping = CONNECTTIMEOUT;
		/*
		 * Ok, so goto's are ugly and can be avoided here but this code
		 * is already indented enough so I think its justified. -avalon
		 */
		if (!killflag && !rflag &&
		    (ping >= currenttime - cptr->lasttime))
			goto ping_timeout;
		/*
		 * If the server hasnt talked to us in 2*ping seconds
		 * and it has a ping time, then close its connection.
		 * If the client is a user and a KILL line was found
		 * to be active, close this connection too.
		 */
		if (killflag || rflag ||
		    ((currenttime - cptr->lasttime) >= (2 * ping) &&
		     (cptr->flags & FLAGS_PINGSENT)) ||
		    (!IsRegistered(cptr) &&
		     (currenttime - cptr->since) >= ping))
		    {
			if (!IsRegistered(cptr) &&
			    (DoingDNS(cptr) || DoingAuth(cptr)))
			    {
				if (cptr->authfd >= 0)
				    {
					(void)close(cptr->authfd);
					cptr->authfd = -1;
					cptr->count = 0;
					*cptr->buffer = '\0';
				    }
				Debug((DEBUG_NOTICE,"DNS/AUTH timeout %s",
					get_client_name(cptr,TRUE)));
				del_queries((char *)cptr);
				ClearAuth(cptr);
				ClearDNS(cptr);
				SetAccess(cptr);
				cptr->since = currenttime;
				continue;
			    }
			if (IsServer(cptr) || IsConnecting(cptr) ||
			    IsHandshake(cptr))
				sendto_ops("No response from %s, closing link",
					   get_client_name(cptr, FALSE));
			/*
			 * this is used for KILL lines with time restrictions
			 * on them - send a messgae to the user being killed
			 * first.
			 */
			if (killflag && IsPerson(cptr))
				sendto_ops("Kill line active for %s",
					   get_client_name(cptr, FALSE));

#if defined(R_LINES) && defined(R_LINES_OFTEN)
			if (IsPerson(cptr) && rflag)
				sendto_ops("Restricting %s, closing link.",
					   get_client_name(cptr,FALSE));
#endif
			(void)exit_client(cptr, cptr, &me, "Ping timeout");
			/*
			 * need to start loop over because the close can
			 * affect the ordering of the local[] array.- avalon
			 */
			i = 0;
			continue;
		    }
		else if ((cptr->flags & FLAGS_PINGSENT) == 0)
		    {
			/*
			 * if we havent PINGed the connection and we havent
			 * heard from it in a while, PING it to make sure
			 * it is still alive.
			 */
			cptr->flags |= FLAGS_PINGSENT;
			/* not nice but does the job */
			cptr->lasttime = currenttime - ping;
			sendto_one(cptr, "PING :%s", me.name);
		    }
ping_timeout:
		timeout = cptr->lasttime + ping;
		while (timeout <= currenttime)
			timeout += ping;
		if (timeout < oldest || !oldest)
			oldest = timeout;
	    }
	if (!oldest || oldest < currenttime)
		oldest = currenttime + PINGFREQUENCY;
	Debug((DEBUG_NOTICE,"Next check_ping() call at: %s, %d %d %d",
		myctime(oldest), ping, oldest, currenttime));

	return (oldest);
}

/*
** bad_command
**	This is called when the commandline is not acceptable.
**	Give error message and exit without starting anything.
*/
static	int	bad_command()
{
  (void)printf(
	 "Usage: ircd %s[-h servername] [-p portnumber] [-x loglevel] [-t]\n",
#ifdef CMDLINE_CONFIG
	 "[-f config] "
#else
	 ""
#endif
	 );
  (void)printf("Server not started\n\n");
  return (-1);
}

int	main(argc, argv)
int	argc;
char	*argv[];
{
	int	portarg = 0;
	uid_t	uid, euid;
	time_t	delay = 0, now;

	uid = getuid();
	euid = geteuid();
#ifdef	PROFIL
	(void)monstartup(0, etext);
	(void)moncontrol(1);
	(void)signal(SIGUSR1, s_monitor);
#endif

#ifdef	CHROOTDIR
	if (chdir(dpath))
	    {
		perror("chdir");
		exit(-1);
	    }
	res_init();
	if (chroot(DPATH))
	  {
	    (void)fprintf(stderr,"ERROR:  Cannot chdir/chroot\n");
	    exit(5);
	  }
#endif /*CHROOTDIR*/

	myargv = argv;
	(void)umask(077);                /* better safe than sorry --SRB */
	bzero((char *)&me, sizeof(me));

	setup_signals();

	/*
	** All command line parameters have the syntax "-fstring"
	** or "-f string" (e.g. the space is optional). String may
	** be empty. Flag characters cannot be concatenated (like
	** "-fxyz"), it would conflict with the form "-fstring".
	*/
	while (--argc > 0 && (*++argv)[0] == '-')
	    {
		char	*p = argv[0]+1;
		int	flag = *p++;

		if (flag == '\0' || *p == '\0')
			if (argc > 1 && argv[1][0] != '-')
			    {
				p = *++argv;
				argc -= 1;
			    }
			else
				p = "";

		switch (flag)
		    {
                    case 'a':
			bootopt |= BOOT_AUTODIE;
			break;
		    case 'c':
			bootopt |= BOOT_CONSOLE;
			break;
		    case 'q':
			bootopt |= BOOT_QUICK;
			break;
		    case 'd' :
                        (void)setuid((uid_t)uid);
			dpath = p;
			break;
		    case 'o': /* Per user local daemon... */
                        (void)setuid((uid_t)uid);
			bootopt |= BOOT_OPER;
		        break;
#ifdef CMDLINE_CONFIG
		    case 'f':
                        (void)setuid((uid_t)uid);
			configfile = p;
			break;
#endif
		    case 'h':
			strncpyzt(me.name, p, sizeof(me.name));
			break;
		    case 'i':
			bootopt |= BOOT_INETD|BOOT_AUTODIE;
		        break;
		    case 'p':
			if ((portarg = atoi(p)) > 0 )
				portnum = portarg;
			break;
		    case 't':
                        (void)setuid((uid_t)uid);
			bootopt |= BOOT_TTY;
			break;
		    case 'v':
			(void)printf("ircd %s\n", version);
			exit(0);
		    case 'x':
#ifdef	DEBUGMODE
                        (void)setuid((uid_t)uid);
			debuglevel = atoi(p);
			debugmode = *p ? p : "0";
			bootopt |= BOOT_DEBUG;
			break;
#else
			(void)fprintf(stderr,
				"%s: DEBUGMODE must be defined for -x y\n",
				myargv[0]);
			exit(0);
#endif
		    default:
			bad_command();
			break;
		    }
	    }

#ifndef	CHROOT
	if (chdir(dpath))
	    {
		perror("chdir");
		exit(-1);
	    }
#endif

#ifndef IRC_UID
	if ((uid != euid) && !euid)
	    {
		(void)fprintf(stderr,
			"ERROR: do not run ircd setuid root. Make it setuid a\
 normal user.\n");
		exit(-1);
	    }
#endif

#if !defined(CHROOTDIR) || (defined(IRC_UID) && defined(IRC_GID))
# ifndef	AIX
	(void)setuid((uid_t)euid);
# endif

	if ((int)getuid() == 0)
	    {
# if defined(IRC_UID) && defined(IRC_GID)

		/* run as a specified user */
		(void)fprintf(stderr,"WARNING: running ircd with uid = %d\n",
			IRC_UID);
		(void)fprintf(stderr,"         changing to gid %d.\n",IRC_GID);
		(void)setuid(IRC_UID);
		(void)setgid(IRC_GID);
#else
		/* check for setuid root as usual */
		(void)fprintf(stderr,
			"ERROR: do not run ircd setuid root. Make it setuid a\
 normal user.\n");
		exit(-1);
# endif	
	    } 
#endif /*CHROOTDIR/UID/GID*/

	/* didn't set debuglevel */
	/* but asked for debugging output to tty */
	if ((debuglevel < 0) &&  (bootopt & BOOT_TTY))
	    {
		(void)fprintf(stderr,
			"you specified -t without -x. use -x <n>\n");
		exit(-1);
	    }

	if (argc > 0)
		return bad_command(); /* This should exit out */

	clear_client_hash_table();
	clear_channel_hash_table();
	initlists();
	initclass();
	initwhowas();
	initstats();
	open_debugfile();
	if (portnum < 0)
		portnum = PORTNUM;
	me.port = portnum;
	(void)init_sys();
	me.flags = FLAGS_LISTEN;
	if (bootopt & BOOT_INETD)
	    {
		me.fd = 0;
		local[0] = &me;
		me.flags = FLAGS_LISTEN;
	    }
	else
		me.fd = -1;

#ifdef USE_SYSLOG
	openlog(myargv[0], LOG_PID|LOG_NDELAY, LOG_FACILITY);
#endif
	if (initconf(bootopt) == -1)
	    {
		Debug((DEBUG_FATAL, "Failed in reading configuration file %s",
			configfile));
		(void)printf("Couldn't open configuration file %s\n",
			configfile);
		exit(-1);
	    }
	if (!(bootopt & BOOT_INETD))
	    {
		aConfItem	*aconf;

		if ((aconf = find_me()) && portarg <= 0 && aconf->port > 0)
			portnum = aconf->port;
		Debug((DEBUG_ERROR, "Port = %d", portnum));
		if (inetport(&me, "*", portnum))
			exit(1);
	    }
	else if (inetport(&me, "*", 0))
		exit(1);

	(void)setup_ping();
	(void)get_my_name(&me, me.sockhost, sizeof(me.sockhost)-1);
	if (me.name[0] == '\0')
		strncpyzt(me.name, me.sockhost, sizeof(me.name));
	me.hopcount = 0;
	me.authfd = -1;
	me.confs = NULL;
	me.next = NULL;
	me.user = NULL;
	me.from = &me;
	SetMe(&me);
	make_server(&me);
	(void)strcpy(me.serv->up, me.name);

	me.lasttime = me.since = me.firsttime = time(NULL);
	(void)add_to_client_hash_table(me.name, &me);

	check_class();
	if (bootopt & BOOT_OPER)
	    {
		aClient *tmp = add_connection(&me, 0);

		if (!tmp)
			exit(1);
		SetMaster(tmp);
	    }
	else
		write_pidfile();

	Debug((DEBUG_NOTICE,"Server ready..."));
#ifdef USE_SYSLOG
	syslog(LOG_NOTICE, "Server Ready");
#endif

	for (;;)
	    {
		now = time(NULL);
		/*
		** We only want to connect if a connection is due,
		** not every time through.  Note, if there are no
		** active C lines, this call to Tryconnections is
		** made once only; it will return 0. - avalon
		*/
		if (nextconnect && now >= nextconnect)
			nextconnect = try_connections(now);
		/*
		** DNS checks. One to timeout queries, one for cache expiries.
		*/
		if (now >= nextdnscheck)
			nextdnscheck = timeout_query_list(now);
		if (now >= nextexpire)
			nextexpire = expire_cache(now);
		/*
		** take the smaller of the two 'timed' event times as
		** the time of next event (stops us being late :) - avalon
		** WARNING - nextconnect can return 0!
		*/
		if (nextconnect)
			delay = MIN(nextping, nextconnect);
		else
			delay = nextping;
		delay = MIN(nextdnscheck, delay);
		delay = MIN(nextexpire, delay);
		delay -= now;
		/*
		** Adjust delay to something reasonable [ad hoc values]
		** (one might think something more clever here... --msa)
		** We don't really need to check that often and as long
		** as we don't delay too long, everything should be ok.
		** waiting too long can cause things to timeout...
		** i.e. PINGS -> a disconnection :(
		** - avalon
		*/
		if (delay < 1)
			delay = 1;
		else
			delay = MIN(delay, TIMESEC);
		(void)read_message(delay);
		
		Debug((DEBUG_DEBUG ,"Got message(s)"));
		
		now = time(NULL);
		/*
		** ...perhaps should not do these loops every time,
		** but only if there is some chance of something
		** happening (but, note that conf->hold times may
		** be changed elsewhere--so precomputed next event
		** time might be too far away... (similarly with
		** ping times) --msa
		*/
		if (now >= nextping)
			nextping = check_pings(now);

		if (dorehash)
		    {
			(void)rehash(&me, &me, 1);
			dorehash = 0;
		    }
		/*
		** Flush output buffers on all connections now if they
		** have data in them (or at least try to flush)
		** -avalon
		*/
		flush_connections(me.fd);
	    }
    }

/*
 * open_debugfile
 *
 * If the -t option is not given on the command line when the server is
 * started, all debugging output is sent to the file set by LPATH in config.h
 * Here we just open that file and make sure it is opened to fd 2 so that
 * any fprintf's to stderr also goto the logfile.  If the debuglevel is not
 * set from the command line by -x, use /dev/null as the dummy logfile as long
 * as DEBUGMODE has been defined, else dont waste the fd.
 */
static	void	open_debugfile()
{
#ifdef	DEBUGMODE
	int	fd;
	aClient	*cptr;

	if (debuglevel >= 0)
	    {
		cptr = make_client(NULL);
		cptr->fd = 2;
		SetLog(cptr);
		cptr->port = debuglevel;
		cptr->flags = 0;
		cptr->acpt = cptr;
		local[2] = cptr;
		(void)strcpy(cptr->sockhost, me.sockhost);

		(void)printf("isatty = %d ttyname = %#x\n",
			isatty(2), (u_int)ttyname(2));
		if (!(bootopt & BOOT_TTY)) /* leave debugging output on fd 2 */
		    {
			(void)truncate(LOGFILE, 0);
			if ((fd = open(LOGFILE, O_WRONLY | O_CREAT, 0600)) < 0) 
				if ((fd = open("/dev/null", O_WRONLY)) < 0)
					exit(-1);
			if (fd != 2)
			    {
				(void)dup2(fd, 2);
				(void)close(fd); 
			    }
			strncpyzt(cptr->name, LOGFILE, sizeof(cptr->name));
		    }
		else if (isatty(2) && ttyname(2))
			strncpyzt(cptr->name, ttyname(2), sizeof(cptr->name));
		else
			(void)strcpy(cptr->name, "FD2-Pipe");
		Debug((DEBUG_FATAL, "Debug: File <%s> Level: %d at %s",
			cptr->name, cptr->port, myctime(time(NULL))));
	    }
	else
		local[2] = NULL;
#endif
	return;
}

static	void	setup_signals()
{
#ifdef	POSIX_SIGNALS
	struct	sigaction act;

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGPIPE);
	(void)sigaddset(&act.sa_mask, SIGALRM);
# ifdef	SIGWINCH
	(void)sigaddset(&act.sa_mask, SIGWINCH);
	(void)sigaction(SIGWINCH, &act, NULL);
# endif
	(void)sigaction(SIGPIPE, &act, NULL);
	act.sa_handler = dummy;
	(void)sigaction(SIGALRM, &act, NULL);
	act.sa_handler = s_rehash;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
	act.sa_handler = s_restart;
	(void)sigaddset(&act.sa_mask, SIGINT);
	(void)sigaction(SIGINT, &act, NULL);
	act.sa_handler = s_die;
	(void)sigaddset(&act.sa_mask, SIGTERM);
	(void)sigaction(SIGTERM, &act, NULL);

#else
# ifndef	HAVE_RELIABLE_SIGNALS
	(void)signal(SIGPIPE, dummy);
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, dummy);
#  endif
# else
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, SIG_IGN);
#  endif
	(void)signal(SIGPIPE, SIG_IGN);
# endif
	(void)signal(SIGALRM, dummy);   
	(void)signal(SIGHUP, s_rehash);
	(void)signal(SIGTERM, s_die); 
	(void)signal(SIGINT, s_restart);
#endif

#ifdef RESTARTING_SYSTEMCALLS
	/*
	** At least on Apollo sr10.1 it seems continuing system calls
	** after signal is the default. The following 'siginterrupt'
	** should change that default to interrupting calls.
	*/
	(void)siginterrupt(SIGALRM, 1);
#endif
}
