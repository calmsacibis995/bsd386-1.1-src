/************************************************************************
 *   IRC - Internet Relay Chat, ircd/s_debug.c
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
static  char sccsid[] = "@(#)s_debug.c	2.27 17 Oct 1993 (C) 1988 University of Oulu, \
Computing Center and Jarkko Oikarinen";
#endif

#include "struct.h"
/*
 * Option string.  Must be before #ifdef DEBUGMODE.
 */
char	serveropts[] = {
#ifdef	SENDQ_ALWAYS
'A',
#endif
#ifdef	CHROOTDIR
'c',
#endif
#ifdef	CMDLINE_CONFIG
'C',
#endif
#ifdef	DEBUGMODE
'D',
#endif
#ifdef	LOCOP_REHASH
'e',
#endif
#ifdef	OPER_REHASH
'E',
#endif
#ifdef	HUB
'H',
#endif
#ifdef	SHOW_INVISIBLE_LUSERS
'i',
#endif
#ifndef	NO_DEFAULT_INVISIBLE
'I',
#endif
#ifdef	OPER_KILL
# ifdef  LOCAL_KILL_ONLY
'k',
# else
'K',
# endif
#endif
#ifdef	LEAST_IDLE
'L',
#endif
#ifdef	M4_PREPROC
'm',
#endif
#ifdef	IDLE_FROM_MSG
'M',
#endif
#ifdef	CRYPT_OPER_PASSWORD
'p',
#endif
#ifdef	CRYPT_LINK_PASSWORD
'P',
#endif
#ifdef	NPATH
'N',
#endif
#ifdef	LOCOP_RESTART
'r',
#endif
#ifdef	OPER_RESTART
'R',
#endif
#ifdef	ENABLE_SUMMON
'S',
#endif
#ifdef	OPER_REMOTE
't',
#endif
#ifdef	IRCII_KLUDGE
'u',
#endif
#ifdef	ENABLE_USERS
'U',
#endif
#ifdef	VALLOC
'V',
#endif
#ifdef	UNIXPORT
'X',
#endif
#ifdef	USE_SYSLOG
'Y',
#endif
#ifdef	V28PlusOnly
'8',
#endif
'\0'};

#include "numeric.h"
#include "common.h"
#include "sys.h"
#include "whowas.h"
#include "hash.h"
#include <sys/file.h>
#ifdef HPUX
#include <fcntl.h>
#endif
#if !defined(ULTRIX) && !defined(SGI) && !defined(sequent) && \
    !defined(__convex__)
# include <sys/param.h>
#endif
#ifdef HPUX
# include <sys/syscall.h>
# define getrusage(a,b) syscall(SYS_GETRUSAGE, a, b)
#endif
#ifdef GETRUSAGE_2
# ifdef SOL20
#  include <sys/time.h>
# endif
# include <sys/resource.h>
#else
#  ifdef TIMES_2
#   include <sys/times.h>
#  endif
#endif
#ifdef PCS
# include <time.h>
#endif
#ifdef HPUX
#include <unistd.h>
#ifdef DYNIXPTX
#include <sys/types.h>
#include <time.h>
#endif
#endif
#include "h.h"

#ifndef ssize_t
#define ssize_t int
#endif

extern	etext;

#ifdef DEBUGMODE
static	char	debugbuf[1024];

#ifndef	USE_VARARGS
/*VARARGS2*/
void	debug(level, form, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
int	level;
char	*form, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10;
{
#else
void	debug(level, form, va_alist)
int	level;
char	*form;
va_dcl
{
	va_list	vl;

	va_start(vl);
#endif
	int	err = errno;

	if ((debuglevel >= 0) && (level <= debuglevel))
	    {
#ifndef	USE_VARARGS
		(void)sprintf(debugbuf, form,
				p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
#else
		(void)vsprintf(debugbuf, form, vl);
#endif
		if (local[2])
		    {
			local[2]->sendM++;
			local[2]->sendB += strlen(debugbuf);
		    }
		(void)fprintf(stderr, "%s", debugbuf);
		(void)fputc('\n', stderr);
	    }
	errno = err;
}

/*
 * This is part of the STATS replies. There is no offical numeric for this
 * since this isnt an official command, in much the same way as HASH isnt.
 * It is also possible that some systems wont support this call or have
 * different field names for "struct rusage".
 * -avalon
 */
void	send_usage(cptr, nick)
aClient *cptr;
char	*nick;
{

#ifdef GETRUSAGE_2
	struct	rusage	rus;
	time_t	secs, rup;
#ifdef	hz
# define hzz hz
#else
# ifdef HZ
#  define hzz HZ
# else
	int	hzz = 1;
#  ifdef HPUX
	hzz = (int)sysconf(_SC_CLK_TCK);
#  endif
# endif
#endif

	if (getrusage(RUSAGE_SELF, &rus) == -1)
	    {
		extern char *sys_errlist[];
		sendto_one(cptr,":%s NOTICE %s :Getruseage error: %s.",
			   me.name, nick, sys_errlist[errno]);
		return;
	    }
	secs = rus.ru_utime.tv_sec + rus.ru_stime.tv_sec;
	rup = time(NULL) - me.since;
	if (secs == 0)
		secs = 1;

	sendto_one(cptr,
		   ":%s %d %s :CPU Secs %d:%d User %d:%d System %d:%d",
		   me.name, RPL_STATSDEBUG, nick, secs/60, secs%60,
		   rus.ru_utime.tv_sec/60, rus.ru_utime.tv_sec%60,
		   rus.ru_stime.tv_sec/60, rus.ru_stime.tv_sec%60);
	sendto_one(cptr, ":%s %d %s :RSS %d ShMem %d Data %d Stack %d",
		   me.name, RPL_STATSDEBUG, nick, rus.ru_maxrss,
		   rus.ru_ixrss / (rup * hzz), rus.ru_idrss / (rup * hzz),
		   rus.ru_isrss / (rup * hzz));
	sendto_one(cptr, ":%s %d %s :Swaps %d Reclaims %d Faults %d",
		   me.name, RPL_STATSDEBUG, nick, rus.ru_nswap,
		   rus.ru_minflt, rus.ru_majflt);
	sendto_one(cptr, ":%s %d %s :Block in %d out %d",
		   me.name, RPL_STATSDEBUG, nick, rus.ru_inblock,
		   rus.ru_oublock);
	sendto_one(cptr, ":%s %d %s :Msg Rcv %d Send %d",
		   me.name, RPL_STATSDEBUG, nick, rus.ru_msgrcv, rus.ru_msgsnd);
	sendto_one(cptr, ":%s %d %s :Signals %d Context Vol. %d Invol %d",
		   me.name, RPL_STATSDEBUG, nick, rus.ru_nsignals,
		   rus.ru_nvcsw, rus.ru_nivcsw);
#else
# ifdef TIMES_2
	struct	tms	tmsbuf;
	time_t	secs, mins;
	int	hzz = 1, ticpermin;
	int	umin, smin, usec, ssec;

#  ifdef HPUX
	hzz = sysconf(_SC_CLK_TCK);
#  endif
	ticpermin = hzz * 60;

	umin = tmsbuf.tms_utime / ticpermin;
	usec = (tmsbuf.tms_utime%ticpermin)/(float)hzz;
	smin = tmsbuf.tms_stime / ticpermin;
	ssec = (tmsbuf.tms_stime%ticpermin)/(float)hzz;
	secs = usec + ssec;
	mins = (secs/60) + umin + smin;
	secs %= hzz;

	if (times(&tmsbuf) == -1)
	    {
		extern char *sys_errlist[];
		sendto_one(cptr,":%s %d %s :times(2) error: %s.",
			   me.name, RPL_STATSDEBUG, nick, strerror(errno));
		return;
	    }
	secs = tmsbuf.tms_utime + tmsbuf.tms_stime;

	sendto_one(cptr,
		   ":%s %d %s :CPU Secs %d:%d User %d:%d System %d:%d",
		   me.name, RPL_STATSDEBUG, nick, mins, secs, umin, usec,
		   smin, ssec);
# endif
#endif
	sendto_one(cptr, ":%s %d %s :Reads %d Writes %d",
		   me.name, RPL_STATSDEBUG, nick, readcalls, writecalls);
	sendto_one(cptr, ":%s %d %s :DBUF alloc %d blocks %d",
		   me.name, RPL_STATSDEBUG, nick, dbufalloc, dbufblocks);
	sendto_one(cptr,
		   ":%s %d %s :Writes:  <0 %d 0 %d <16 %d <32 %d <64 %d",
		   me.name, RPL_STATSDEBUG, nick,
		   writeb[0], writeb[1], writeb[2], writeb[3], writeb[4]);
	sendto_one(cptr,
		   ":%s %d %s :<128 %d <256 %d <512 %d <1024 %d >1024 %d",
		   me.name, RPL_STATSDEBUG, nick,
		   writeb[5], writeb[6], writeb[7], writeb[8], writeb[9]);
	return;
}
#endif

void	count_memory(cptr, nick)
aClient	*cptr;
char	*nick;
{
	extern	aChannel	*channel;
	extern	aClass	*classes;
	extern	aConfItem	*conf;

	Reg1 aClient *acptr;
	Reg2 Link *link;
	Reg3 aChannel *chptr;
	Reg4 aConfItem *aconf;
	Reg5 aClass *cltmp;

	int	lc = 0,		/* local clients */
		ch = 0,		/* channels */
		lcc = 0,	/* local client conf links */
		rc = 0,		/* remote clients */
		us = 0,		/* user structs */
		chu = 0,	/* channel users */
		chi = 0,	/* channel invites */
		chb = 0,	/* channel bans */
		wwu = 0,	/* whowas users */
		cl = 0,		/* classes */
		co = 0;		/* conf lines */

	int	usi = 0,	/* users invited */
		usc = 0,	/* users in channels */
		aw = 0,		/* aways set */
		wwa = 0;	/* whowas aways */

	u_long	chm = 0,	/* memory used by channels */
		chbm = 0,	/* memory used by channel bans */
		lcm = 0,	/* memory used by local clients */
		rcm = 0,	/* memory used by remote clients */
		awm = 0,	/* memory used by aways */
		wwam = 0,	/* whowas away memory used */
		wwm = 0,	/* whowas array memory used */
		com = 0,	/* memory used by conf lines */
		db = 0,		/* memory used by dbufs */
		rm = 0,		/* res memory used */
		totcl = 0,
		totch = 0,
		totww = 0,
		tot = 0;

	count_whowas_memory(&wwu, &wwa, &wwam);
	wwm = sizeof(aName) * NICKNAMEHISTORYLENGTH;

	for (acptr = client; acptr; acptr = acptr->next)
	    {
		if (MyConnect(acptr))
		    {
			lc++;
			for (link = acptr->confs; link; link = link->next)
				lcc++;
		    }
		else
			rc++;
		if (acptr->user)
		   {
			us++;
			for (link = acptr->user->invited; link;
			     link = link->next)
				usi++;
			for (link = acptr->user->channel; link;
			     link = link->next)
				usc++;
			if (acptr->user->away)
			    {
				aw++;
				awm += (strlen(acptr->user->away)+1);
			    }
		   }
	    }
	lcm = lc * CLIENT_LOCAL_SIZE;
	rcm = rc * CLIENT_REMOTE_SIZE;

	for (chptr = channel; chptr; chptr = chptr->nextch)
	    {
		ch++;
		chm += (strlen(chptr->chname) + sizeof(aChannel));
		for (link = chptr->members; link; link = link->next)
			chu++;
		for (link = chptr->invites; link; link = link->next)
			chi++;
		for (link = chptr->banlist; link; link = link->next)
		    {
			chb++;
			chbm += (strlen(link->value.cp)+1+sizeof(Link));
		    }
	    }

	for (aconf = conf; aconf; aconf = aconf->next)
	    {
		co++;
		com += aconf->host ? strlen(aconf->host)+1 : 0;
		com += aconf->passwd ? strlen(aconf->passwd)+1 : 0;
		com += aconf->name ? strlen(aconf->name)+1 : 0;
		com += sizeof(aConfItem);
	    }

	for (cltmp = classes; cltmp; cltmp = cltmp->next)
		cl++;

	sendto_one(cptr, ":%s %d %s :Client Local %d(%d) Remote %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, lc, lcm, rc, rcm);
	sendto_one(cptr, ":%s %d %s :Users %d(%d) Invites %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, us, us*sizeof(anUser), usi,
		   usi * sizeof(Link));
	sendto_one(cptr, ":%s %d %s :User channels %d(%d) Aways %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, usc, usc*sizeof(Link),
		   aw, awm);
	sendto_one(cptr, ":%s %d %s :Attached confs %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, lcc, lcc*sizeof(Link));

	totcl = lcm + rcm + us*sizeof(anUser) + usc*sizeof(Link) + awm;
	totcl += lcc*sizeof(Link) + usi*sizeof(Link);

	sendto_one(cptr, ":%s %d %s :Conflines %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, co, com);

	sendto_one(cptr, ":%s %d %s :Classes %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, cl, cl*sizeof(aClass));

	sendto_one(cptr, ":%s %d %s :Channels %d(%d) Bans %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, ch, chm, chb, chbm);
	sendto_one(cptr, ":%s %d %s :Channel membrs %d(%d) invite %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, chu, chu*sizeof(Link),
		   chi, chi*sizeof(Link));

	totch = chm + chbm + chu*sizeof(Link) + chi*sizeof(Link);

	sendto_one(cptr, ":%s %d %s :Whowas users %d(%d) away %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, wwu, wwu*sizeof(anUser),
		   wwa, wwam);
	sendto_one(cptr, ":%s %d %s :Whowas array %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, NICKNAMEHISTORYLENGTH, wwm);

	totww = wwu*sizeof(anUser) + wwam + wwm;

	sendto_one(cptr, ":%s %d %s :Hash: client %d(%d) chan %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, HASHSIZE,
		   sizeof(aHashEntry) * HASHSIZE,
		   CHANNELHASHSIZE, sizeof(aHashEntry) * CHANNELHASHSIZE);
	db = dbufblocks * sizeof(dbufbuf);
	sendto_one(cptr, ":%s %d %s :Dbuf blocks %d(%d)",
		   me.name, RPL_STATSDEBUG, nick, dbufblocks, db);

	rm = cres_mem(cptr);

	tot = totww + totch + totcl + com + cl*sizeof(aClass) + db + rm;
	tot += sizeof(aHashEntry) * HASHSIZE;
	tot += sizeof(aHashEntry) * CHANNELHASHSIZE;

	sendto_one(cptr, ":%s %d %s :Total: ww %d ch %d cl %d co %d db %d",
		   me.name, RPL_STATSDEBUG, nick, totww, totch, totcl, com, db);
	sendto_one(cptr, ":%s %d %s :TOTAL: %d sbrk(0)-etext: %d",
		   me.name, RPL_STATSDEBUG, nick, tot,
#if !defined(AIX) && !defined(NEXT)
		   (int)sbrk((ssize_t)0)-(int)etext);
#else
		   (int)sbrk((ssize_t)0));
#endif
	return;
}
