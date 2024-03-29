/* $Header: /bsdi/MASTER/BSDI_OS/contrib/tcsh/tc.func.c,v 1.2 1994/01/13 23:24:38 polk Exp $ */
/*
 * tc.func.c: New tcsh builtins.
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "sh.h"

RCSID("$Id: tc.func.c,v 1.2 1994/01/13 23:24:38 polk Exp $")

#include "ed.h"
#include "ed.defns.h"		/* for the function names */
#include "tw.h"
#include "tc.h"

#ifdef HESIOD
# include <hesiod.h>
#endif /* HESIOD */

#ifdef TESLA
extern int do_logout;
#endif /* TESLA */
extern time_t t_period;
extern int just_signaled;
static bool precmd_active = 0;
static bool periodic_active = 0;
static bool cwdcmd_active = 0;	/* PWP: for cwd_cmd */
static bool beepcmd_active = 0;
static void (*alm_fun)() = NULL;

static	void	 Reverse	__P((Char *));
static	void	 auto_logout	__P((void));
static	char	*xgetpass	__P((char *));
static	void	 auto_lock	__P((void));
#ifdef BSDJOBS
static	void	 insert		__P((struct wordent *, bool));
static	void	 insert_we	__P((struct wordent *, struct wordent *));
static	int	 inlist		__P((Char *, Char *));
#endif /* BSDJOBS */
static  Char    *gethomedir	__P((Char *));

/*
 * Tops-C shell
 */

/*
 * expand_lex: Take the given lex and put an expanded version of it in the
 * string buf. First guy in lex list is ignored; last guy is ^J which we
 * ignore Only take lex'es from position from to position to inclusive Note:
 * csh sometimes sets bit 8 in characters which causes all kinds of problems
 * if we don't mask it here. Note: excl's in lexes have been un-back-slashed
 * and must be re-back-slashed
 * (PWP: NOTE: this returns a pointer to the END of the string expanded
 *             (in other words, where the NUL is).)
 */
/* PWP: this is a combination of the old sprlex() and the expand_lex from
   the magic-space stuff */

Char   *
expand_lex(buf, bufsiz, sp0, from, to)
    Char   *buf;
    int     bufsiz;
    struct wordent *sp0;
    int     from, to;
{
    register struct wordent *sp;
    register Char *s, *d, *e;
    register Char prev_c;
    register int i;

    buf[0] = '\0';
    prev_c = '\0';
    d = buf;
    e = &buf[bufsiz];		/* for bounds checking */

    if (!sp0)
	return (buf);		/* null lex */
    if ((sp = sp0->next) == sp0)
	return (buf);		/* nada */
    if (sp == (sp0 = sp0->prev))
	return (buf);		/* nada */

    for (i = 0; i < NCARGS; i++) {
	if ((i >= from) && (i <= to)) {	/* if in range */
	    for (s = sp->word; *s && d < e; s++) {
		/*
		 * bugfix by Michael Bloom: anything but the current history
		 * character {(PWP) and backslash} seem to be dealt with
		 * elsewhere.
		 */
		if ((*s & QUOTE)
		    && (((*s & TRIM) == HIST) ||
			(((*s & TRIM) == '\'') && (prev_c != '\\')) ||
			(((*s & TRIM) == '\"') && (prev_c != '\\')) ||
			(((*s & TRIM) == '\\') && (prev_c != '\\')))) {
		    *d++ = '\\';
		}
		*d++ = (*s & TRIM);
		prev_c = *s;
	    }
	    if (d < e)
		*d++ = ' ';
	}
	sp = sp->next;
	if (sp == sp0)
	    break;
    }
    if (d > buf)
	d--;			/* get rid of trailing space */

    return (d);
}

Char   *
sprlex(buf, sp0)
    Char   *buf;
    struct wordent *sp0;
{
    Char   *cp;

    cp = expand_lex(buf, INBUFSIZE, sp0, 0, NCARGS);
    *cp = '\0';
    return (buf);
}

void
Itoa(n, s)			/* convert n to characters in s */
    int     n;
    Char   *s;
{
    int     i, sign;

    if ((sign = n) < 0)		/* record sign */
	n = -n;
    i = 0;
    do {
	s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
	s[i++] = '-';
    s[i] = '\0';
    Reverse(s);
}

static void
Reverse(s)
    Char   *s;
{
    Char   c;
    int     i, j;

    for (i = 0, j = (int) Strlen(s) - 1; i < j; i++, j--) {
	c = s[i];
	s[i] = s[j];
	s[j] = c;
    }
}


/*ARGSUSED*/
void
dolist(v, c)
    register Char **v;
    struct command *c;
{
    int     i, k;
    struct stat st;

    USE(c);
    if (*++v == NULL) {
	(void) t_search(STRNULL, NULL, LIST, 0, TW_ZERO, 0, STRNULL, 0);
	return;
    }
    gflag = 0;
    tglob(v);
    if (gflag) {
	v = globall(v);
	if (v == 0)
	    stderror(ERR_NAME | ERR_NOMATCH);
    }
    else
	v = gargv = saveblk(v);
    trim(v);
    for (k = 0; v[k] != NULL && v[k][0] != '-'; k++)
	continue;
    if (v[k]) {
	/*
	 * We cannot process a flag therefore we let ls do it right.
	 */
	static Char STRls[] = {'l', 's', '\0'};
	static Char STRmCF[] = {'-', 'C', 'F', '\0', '\0' };
	struct command *t;
	struct wordent cmd, *nextword, *lastword;
	Char   *cp;
	struct varent *vp;

#ifdef BSDSIGS
	sigmask_t omask = 0;

	if (setintr)
	    omask = sigblock(sigmask(SIGINT)) & ~sigmask(SIGINT);
#else /* !BSDSIGS */
	(void) sighold(SIGINT);
#endif /* BSDSIGS */
	if (seterr) {
	    xfree((ptr_t) seterr);
	    seterr = NULL;
	}

	/* Look at showdots, to add -A to the flags if necessary */
	if ((vp = adrof(STRshowdots)) != NULL) {
	    if (vp->vec[0][0] == '-' && vp->vec[0][1] == 'A' &&
		vp->vec[0][2] == '\0')
		STRmCF[3] = 'A';
	    else
		STRmCF[3] = '\0';
	}
	else
	    STRmCF[3] = '\0';

	cmd.word = STRNULL;
	lastword = &cmd;
	nextword = (struct wordent *) xcalloc(1, sizeof cmd);
	nextword->word = Strsave(STRls);
	lastword->next = nextword;
	nextword->prev = lastword;
	lastword = nextword;
	nextword = (struct wordent *) xcalloc(1, sizeof cmd);
	nextword->word = Strsave(STRmCF);
	lastword->next = nextword;
	nextword->prev = lastword;
	lastword = nextword;
	for (cp = *v; cp; cp = *++v) {
	    nextword = (struct wordent *) xcalloc(1, sizeof cmd);
	    nextword->word = Strsave(cp);
	    lastword->next = nextword;
	    nextword->prev = lastword;
	    lastword = nextword;
	}
	lastword->next = &cmd;
	cmd.prev = lastword;

	/* build a syntax tree for the command. */
	t = syntax(cmd.next, &cmd, 0);
	if (seterr)
	    stderror(ERR_OLD);
	/* expand aliases like process() does */
	/* alias(&cmd); */
	/* execute the parse tree. */
	execute(t, tpgrp > 0 ? tpgrp : -1, NULL, NULL);
	/* done. free the lex list and parse tree. */
	freelex(&cmd), freesyn(t);
	if (setintr)
#ifdef BSDSIGS
	    (void) sigsetmask(omask);
#else /* !BSDSIGS */
	    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */
    }
    else {
	Char   *dp, *tmp, buf[MAXPATHLEN];

	for (k = 0, i = 0; v[k] != NULL; k++) {
	    tmp = dnormalize(v[k], symlinks == SYM_IGNORE);
	    dp = &tmp[Strlen(tmp) - 1];
	    if (*dp == '/' && dp != tmp)
#ifdef apollo
		if (dp != &tmp[1])
#endif /* apollo */
		*dp = '\0';
	    if (stat(short2str(tmp), &st) == -1) {
		if (k != i) {
		    if (i != 0)
			xputchar('\n');
		    print_by_column(STRNULL, &v[i], k - i, FALSE);
		}
		xprintf("%S: %s.\n", tmp, strerror(errno));
		i = k + 1;
	    }
	    else if (S_ISDIR(st.st_mode)) {
		Char   *cp;

		if (k != i) {
		    if (i != 0)
			xputchar('\n');
		    print_by_column(STRNULL, &v[i], k - i, FALSE);
		}
		if (k != 0 && v[1] != NULL)
		    xputchar('\n');
		xprintf("%S:\n", tmp);
		for (cp = tmp, dp = buf; *cp; *dp++ = (*cp++ | QUOTE))
		    continue;
		if (dp[-1] != (Char) ('/' | QUOTE))
		    *dp++ = '/';
		else 
		    dp[-1] &= TRIM;
		*dp = '\0';
		(void) t_search(buf, NULL, LIST, 0, TW_ZERO, 0, STRNULL, 0);
		i = k + 1;
	    }
	    xfree((ptr_t) tmp);
	}
	if (k != i) {
	    if (i != 0)
		xputchar('\n');
	    print_by_column(STRNULL, &v[i], k - i, FALSE);
	}
    }

    if (gargv) {
	blkfree(gargv);
	gargv = 0;
    }
}

static char *defaulttell = "ALL";
extern bool GotTermCaps;

/*ARGSUSED*/
void
dotelltc(v, c)
    register Char **v;
    struct command *c;
{
    USE(c);
    if (!GotTermCaps)
	GetTermCaps();

    TellTC(v[1] ? short2str(v[1]) : defaulttell);
}

/*ARGSUSED*/
void
doechotc(v, c)
    register Char **v;
    struct command *c;
{
    if (!GotTermCaps)
	GetTermCaps();
    EchoTC(++v);
}

/*ARGSUSED*/
void
dosettc(v, c)
    Char  **v;
    struct command *c;
{
    char    tv[2][BUFSIZE];

    if (!GotTermCaps)
	GetTermCaps();

    (void) strcpy(tv[0], short2str(v[1]));
    (void) strcpy(tv[1], short2str(v[2]));
    SetTC(tv[0], tv[1]);
}

/* The dowhich() is by:
 *  Andreas Luik <luik@isaak.isa.de>
 *  I S A  GmbH - Informationssysteme fuer computerintegrierte Automatisierung
 *  Azenberstr. 35
 *  D-7000 Stuttgart 1
 *  West-Germany
 * Thanks!!
 */

/*ARGSUSED*/
void
dowhich(v, c)
    register Char **v;
    struct command *c;
{
    struct wordent lexp[3];
    struct varent *vp;

    USE(c);
    lexp[0].next = &lexp[1];
    lexp[1].next = &lexp[2];
    lexp[2].next = &lexp[0];

    lexp[0].prev = &lexp[2];
    lexp[1].prev = &lexp[0];
    lexp[2].prev = &lexp[1];

    lexp[0].word = STRNULL;
    lexp[2].word = STRret;

#ifdef notdef
    /* 
     * We don't want to glob dowhich args because we lose quoteing
     * E.g. which \ls if ls is aliased will not work correctly if
     * we glob here.
     */
    gflag = 0, tglob(v);
    if (gflag) {
	v = globall(v);
	if (v == 0)
	    stderror(ERR_NAME | ERR_NOMATCH);
    }
    else {
	v = gargv = saveblk(v);
	trim(v);
    }
#endif

    while (*++v) {
	if ((vp = adrof1(*v, &aliases)) != NULL) {
	    xprintf("%S: \t aliased to ", *v);
	    blkpr(vp->vec);
	    xputchar('\n');
	}
	else {
	    lexp[1].word = *v;
	    tellmewhat(lexp);
	}
    }
#ifdef notdef
    /* Again look at the comment above; since we don't glob, we don't free */
    if (gargv)
	blkfree(gargv), gargv = 0;
#endif
}

/* PWP: a hack to start up your stopped editor on a single keystroke */
/* jbs - fixed hack so it worked :-) 3/28/89 */

struct process *
find_stop_ed()
{
    register struct process *pp;
    register char *ep, *vp, *cp, *p;
    int     epl, vpl;

    if ((ep = getenv("EDITOR")) != NULL) {	/* if we have a value */
	if ((p = strrchr(ep, '/')) != NULL) 	/* if it has a path */
	    ep = p + 1;		/* then we want only the last part */
    }
    else 
	ep = "ed";

    if ((vp = getenv("VISUAL")) != NULL) {	/* if we have a value */
	if ((p = strrchr(vp, '/')) != NULL) 	/* and it has a path */
	    vp = p + 1;		/* then we want only the last part */
    }
    else 
	vp = "vi";

    for (vpl = 0; vp[vpl] && !Isspace(vp[vpl]); vpl++)
	continue;
    for (epl = 0; ep[epl] && !Isspace(ep[epl]); epl++)
	continue;

    if (pcurrent == NULL)	/* see if we have any jobs */
	return NULL;		/* nope */

    for (pp = proclist.p_next; pp; pp = pp->p_next)
	if (pp->p_procid == pp->p_jobid) {
	    p = short2str(pp->p_command);
	    /* get the first word */
	    for (cp = p; *cp && !isspace(*cp); cp++)
		continue;
	    *cp = '\0';
		
	    if ((cp = strrchr(p, '/')) != NULL)	/* and it has a path */
		cp = cp + 1;		/* then we want only the last part */
	    else
		cp = p;			/* else we get all of it */

	    /* if we find either in the current name, fg it */
	    if (strncmp(ep, cp, (size_t) epl) == 0 ||
		strncmp(vp, cp, (size_t) vpl) == 0)
		return pp;
	}

    return NULL;		/* didn't find a job */
}

void
fg_proc_entry(pp)
    register struct process *pp;
{
#ifdef BSDSIGS
    sigmask_t omask;
#endif
    jmp_buf_t osetexit;
    bool    ohaderr;
    Char    oGettingInput;

    getexit(osetexit);

#ifdef BSDSIGS
    omask = sigblock(sigmask(SIGINT));
#else
    (void) sighold(SIGINT);
#endif
    oGettingInput = GettingInput;
    GettingInput = 0;

    ohaderr = haderr;		/* we need to ignore setting of haderr due to
				 * process getting stopped by a signal */
    if (setexit() == 0) {	/* come back here after pjwait */
	pendjob();
	pstart(pp, 1);		/* found it. */
	(void) alarm(0);	/* No autologout */
	pjwait(pp);
    }
    setalarm(1);		/* Autologout back on */
    resexit(osetexit);
    haderr = ohaderr;
    GettingInput = oGettingInput;

#ifdef BSDSIGS
    (void) sigsetmask(omask);
#else /* !BSDSIGS */
    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */

}

static char *
xgetpass(prm)
    char *prm;
{
    static char pass[9];
    int fd, i;
    sigret_t (*sigint)();

    sigint = (sigret_t (*)()) sigset(SIGINT, SIG_IGN);
    (void) Rawmode();	/* Make sure, cause we want echo off */
    if ((fd = open("/dev/tty", O_RDWR)) == -1)
	fd = SHIN;

    xprintf("%s", prm); flush();
    for (i = 0;;)  {
	if (read(fd, &pass[i], 1) < 1 || pass[i] == '\n') 
	    break;
	if (i < 8)
	    i++;
    }
	
    pass[i] = '\0';

    if (fd != SHIN)
	(void) close(fd);
    (void) sigset(SIGINT, sigint);

    return(pass);
}
	
/*
 * Ask the user for his login password to continue working
 * On systems that have a shadow password, this will only 
 * work for root, but what can we do?
 *
 * If we fail to get the password, then we log the user out
 * immediately
 */
static void
auto_lock()
{
#ifndef NO_CRYPT

    int i;
    char *srpp = NULL;
    struct passwd *pw;

#undef XCRYPT

#if defined(PW_AUTH) && !defined(XCRYPT)

    struct authorization *apw;
    extern char *crypt16();

# define XCRYPT(a, b) crypt16(a, b)

    if ((pw = getpwuid(euid)) != NULL &&	/* effective user passwd  */
        (apw = getauthuid(euid)) != NULL) 	/* enhanced ultrix passwd */
	srpp = apw->a_password;

#endif /* PW_AUTH && !XCRYPT */

#if defined(PW_SHADOW) && !defined(XCRYPT)

    struct spwd *spw;
    extern char *crypt();

# define XCRYPT(a, b) crypt(a, b)

    if ((pw = getpwuid(euid)) != NULL &&	/* effective user passwd  */
	(spw = getspnam(pw->pw_name)) != NULL)	/* shadowed passwd	  */
	srpp = spw->sp_pwdp;

#endif /* PW_SHADOW && !XCRYPT */

#ifndef XCRYPT
    extern char *crypt();

#define XCRYPT(a, b) crypt(a, b)

    if ((pw = getpwuid(euid)) != NULL)	/* effective user passwd  */
	srpp = pw->pw_passwd;

#endif /* !XCRYPT */

    if (srpp == NULL) {
	auto_logout();
	/*NOTREACHED*/
	return;
    }

    setalarm(0);		/* Not for locking any more */
#ifdef BSDSIGS
    (void) sigsetmask(sigblock(0) & ~(sigmask(SIGALRM)));
#else /* !BSDSIGS */
    (void) sigrelse(SIGALRM);
#endif /* BSDSIGS */
    xputchar('\n'); 
    for (i = 0; i < 5; i++) {
	char *crpp, *pp;
	pp = xgetpass("Password:"); 

	crpp = XCRYPT(pp, srpp);
	if (strcmp(crpp, srpp) == 0) {
	    if (GettingInput && !just_signaled) {
		(void) Rawmode();
		ClearLines();	
		ClearDisp();	
		Refresh();
	    }
	    just_signaled = 0;
	    return;
	}
	xprintf("\nIncorrect passwd for %s\n", pw->pw_name);
    }
#endif /* NO_CRYPT */
    auto_logout();
}


static void
auto_logout()
{
    xprintf("auto-logout\n");
    /* Don't leave the tty in raw mode */
    if (editing)
	(void) Cookedmode();
    (void) close(SHIN);
    set(STRlogout, Strsave(STRautomatic), VAR_READWRITE);
    child = 1;
#ifdef TESLA
    do_logout = 1;
#endif /* TESLA */
    GettingInput = FALSE; /* make flush() work to write hist files. Huber*/
    goodbye(NULL, NULL);
}

sigret_t
/*ARGSUSED*/
alrmcatch(snum)
int snum;
{
#ifdef UNRELSIGS
    if (snum)
	(void) sigset(SIGALRM, alrmcatch);
#endif /* UNRELSIGS */

    (*alm_fun)();

    setalarm(1);
#ifndef SIGVOID
    return (snum);
#endif /* !SIGVOID */
}

/*
 * Karl Kleinpaste, 21oct1983.
 * Added precmd(), which checks for the alias
 * precmd in aliases.  If it's there, the alias
 * is executed as a command.  This is done
 * after mailchk() and just before print-
 * ing the prompt.  Useful for things like printing
 * one's current directory just before each command.
 */
void
precmd()
{
#ifdef BSDSIGS
    sigmask_t omask;

    omask = sigblock(sigmask(SIGINT));
#else /* !BSDSIGS */
    (void) sighold(SIGINT);
#endif /* BSDSIGS */
    if (precmd_active) {	/* an error must have been caught */
	aliasrun(2, STRunalias, STRprecmd);
	xprintf("Faulty alias 'precmd' removed.\n");
	goto leave;
    }
    precmd_active = 1;
    if (!whyles && adrof1(STRprecmd, &aliases))
	aliasrun(1, STRprecmd, NULL);
leave:
    precmd_active = 0;
#ifdef BSDSIGS
    (void) sigsetmask(omask);
#else /* !BSDSIGS */
    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */
}

/*
 * Paul Placeway  11/24/87  Added cwd_cmd by hacking precmd() into
 * submission...  Run every time $cwd is set (after it is set).  Useful
 * for putting your machine and cwd (or anything else) in an xterm title
 * space.
 */
void
cwd_cmd()
{
#ifdef BSDSIGS
    sigmask_t omask;

    omask = sigblock(sigmask(SIGINT));
#else /* !BSDSIGS */
    (void) sighold(SIGINT);
#endif /* BSDSIGS */
    if (cwdcmd_active) {	/* an error must have been caught */
	aliasrun(2, STRunalias, STRcwdcmd);
	xprintf("Faulty alias 'cwdcmd' removed.\n");
	goto leave;
    }
    cwdcmd_active = 1;
    if (!whyles && adrof1(STRcwdcmd, &aliases))
	aliasrun(1, STRcwdcmd, NULL);
leave:
    cwdcmd_active = 0;
#ifdef BSDSIGS
    (void) sigsetmask(omask);
#else /* !BSDSIGS */
    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */
}

/*
 * Joachim Hoenig  07/16/91  Added beep_cmd, run every time tcsh wishes 
 * to beep the terminal bell. Useful for playing nice sounds instead.
 */
void
beep_cmd()
{
#ifdef BSDSIGS
    sigmask_t omask;

    omask = sigblock(sigmask(SIGINT));
#else /* !BSDSIGS */
    (void) sighold(SIGINT);
#endif /* BSDSIGS */
    if (beepcmd_active) {	/* an error must have been caught */
	aliasrun(2, STRunalias, STRbeepcmd);
	xprintf("Faulty alias 'beepcmd' removed.\n");
    }
    else {
	beepcmd_active = 1;
	if (!whyles && adrof1(STRbeepcmd, &aliases))
	    aliasrun(1, STRbeepcmd, NULL);
    }
    beepcmd_active = 0;
#ifdef BSDSIGS
    (void) sigsetmask(omask);
#else /* !BSDSIGS */
    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */
}


/*
 * Karl Kleinpaste, 18 Jan 1984.
 * Added period_cmd(), which executes the alias "periodic" every
 * $tperiod minutes.  Useful for occasional checking of msgs and such.
 */
void
period_cmd()
{
    register Char *vp;
    time_t  t, interval;
#ifdef BSDSIGS
    sigmask_t omask;

    omask = sigblock(sigmask(SIGINT));
#else /* !BSDSIGS */
    (void) sighold(SIGINT);
#endif /* BSDSIGS */
    if (periodic_active) {	/* an error must have been caught */
	aliasrun(2, STRunalias, STRperiodic);
	xprintf("Faulty alias 'periodic' removed.\n");
	goto leave;
    }
    periodic_active = 1;
    if (!whyles && adrof1(STRperiodic, &aliases)) {
	vp = value(STRtperiod);
	if (vp == STRNULL)
	    return;
	interval = getn(vp);
	(void) time(&t);
	if (t - t_period >= interval * 60) {
	    t_period = t;
	    aliasrun(1, STRperiodic, NULL);
	}
    }
leave:
    periodic_active = 0;
#ifdef BSDSIGS
    (void) sigsetmask(omask);
#else /* !BSDSIGS */
    (void) sigrelse(SIGINT);
#endif /* BSDSIGS */
}

/*
 * Karl Kleinpaste, 21oct1983.
 * Set up a one-word alias command, for use for special things.
 * This code is based on the mainline of process().
 */
void
aliasrun(cnt, s1, s2)
    int     cnt;
    Char   *s1, *s2;
{
    struct wordent w, *new1, *new2;	/* for holding alias name */
    struct command *t = NULL;
    jmp_buf_t osetexit;
    int status;

    getexit(osetexit);
    if (seterr) {
	xfree((ptr_t) seterr);
	seterr = NULL;	/* don't repeatedly print err msg. */
    }
    w.word = STRNULL;
    new1 = (struct wordent *) xcalloc(1, sizeof w);
    new1->word = Strsave(s1);
    if (cnt == 1) {
	/* build a lex list with one word. */
	w.next = w.prev = new1;
	new1->next = new1->prev = &w;
    }
    else {
	/* build a lex list with two words. */
	new2 = (struct wordent *) xcalloc(1, sizeof w);
	new2->word = Strsave(s2);
	w.next = new2->prev = new1;
	new1->next = w.prev = new2;
	new1->prev = new2->next = &w;
    }

    /* Save the old status */
    status = getn(value(STRstatus));

    /* expand aliases like process() does. */
    alias(&w);
    /* build a syntax tree for the command. */
    t = syntax(w.next, &w, 0);
    if (seterr)
	stderror(ERR_OLD);

    psavejob();


    /* catch any errors here */
    if (setexit() == 0)
	/* execute the parse tree. */
	/*
	 * From: Michael Schroeder <mlschroe@immd4.informatik.uni-erlangen.de>
	 * was execute(t, tpgrp);
	 */
	execute(t, tpgrp > 0 ? tpgrp : -1, NULL, NULL);	
    /* done. free the lex list and parse tree. */
    freelex(&w), freesyn(t);
    if (haderr) {
	haderr = 0;
	/*
	 * Either precmd, or cwdcmd, or periodic had an error. Call it again so
	 * that it is removed
	 */
	if (precmd_active)
	    precmd();
#ifdef notdef
	/*
	 * XXX: On the other hand, just interrupting them causes an error too.
	 * So if we hit ^C in the middle of cwdcmd or periodic the alias gets
	 * removed. We don't want that. Note that we want to remove precmd
	 * though, cause that could lead into an infinite loop. This should be
	 * fixed correctly, but then haderr should give us the whole exit
	 * status not just true or false.
	 */
	else if (cwdcmd_active)
	    cwd_cmd();
	else if (beepcmd_active)
	    beep_cmd();
	else if (periodic_active)
	    period_cmd();
#endif /* notdef */
    }
    /* reset the error catcher to the old place */
    resexit(osetexit);
    prestjob();
    pendjob();
    /* Restore status */
    set(STRstatus, putn(status), VAR_READWRITE);
}

void
setalarm(lck)
    int lck;
{
    struct varent *vp;
    Char   *cp;
    unsigned alrm_time = 0, logout_time, lock_time;
    time_t cl, nl, sched_dif;

    if ((vp = adrof(STRautologout)) != NULL) {
	if ((cp = vp->vec[0]) != 0) {
	    if ((logout_time = (unsigned) atoi(short2str(cp)) * 60) > 0) {
		alrm_time = logout_time;
		alm_fun = auto_logout;
	    }
	}
	if ((cp = vp->vec[1]) != 0) {
	    if ((lock_time = (unsigned) atoi(short2str(cp)) * 60) > 0) {
		if (lck) {
		    if (alrm_time == 0 || lock_time < alrm_time) {
			alrm_time = lock_time;
			alm_fun = auto_lock;
		    }
		}
		else /* lock_time always < alrm_time */
		    if (alrm_time)
			alrm_time -= lock_time;
	    }
	}
    }
    if ((nl = sched_next()) != -1) {
	(void) time(&cl);
	sched_dif = nl > cl ? nl - cl : 0;
	if ((alrm_time == 0) || ((unsigned) sched_dif < alrm_time)) {
	    alrm_time = ((unsigned) sched_dif) + 1;
	    alm_fun = sched_run;
	}
    }
    (void) alarm(alrm_time);	/* Autologout ON */
}

#undef RMDEBUG			/* For now... */

void
rmstar(cp)
    struct wordent *cp;
{
    struct wordent *we, *args;
    register struct wordent *tmp, *del;

#ifdef RMDEBUG
    static Char STRrmdebug[] = {'r', 'm', 'd', 'e', 'b', 'u', 'g', '\0'};
    Char   *tag;
#endif /* RMDEBUG */
    Char   *charac;
    char    c;
    int     ask, doit, star = 0, silent = 0;

    if (!adrof(STRrmstar))
	return;
#ifdef RMDEBUG
    tag = value(STRrmdebug);
#endif /* RMDEBUG */
    we = cp->next;
    while (*we->word == ';' && we != cp)
	we = we->next;
    while (we != cp) {
#ifdef RMDEBUG
	if (*tag)
	    xprintf("parsing command line\n");
#endif /* RMDEBUG */
	if (!Strcmp(we->word, STRrm)) {
	    args = we->next;
	    ask = (*args->word != '-');
	    while (*args->word == '-' && !silent) {	/* check options */
		for (charac = (args->word + 1); *charac && !silent; charac++)
		    silent = (*charac == 'i' || *charac == 'f');
		args = args->next;
	    }
	    ask = (ask || (!ask && !silent));
	    if (ask) {
		for (; !star && *args->word != ';'
		     && args != cp; args = args->next)
		    if (!Strcmp(args->word, STRstar))
			star = 1;
		if (ask && star) {
		    xprintf("Do you really want to delete all files? [n/y] ");
		    flush();
		    (void) read(SHIN, &c, 1);
		    doit = (c == 'Y' || c == 'y');
		    while (c != '\n')
			(void) read(SHIN, &c, 1);
		    if (!doit) {
			/* remove the command instead */
#ifdef RMDEBUG
			if (*tag)
			    xprintf("skipping deletion of files!\n");
#endif /* RMDEBUG */
			for (tmp = we;
			     *tmp->word != '\n' &&
			     *tmp->word != ';' && tmp != cp;) {
			    tmp->prev->next = tmp->next;
			    tmp->next->prev = tmp->prev;
			    xfree((ptr_t) tmp->word);
			    del = tmp;
			    tmp = tmp->next;
			    xfree((ptr_t) del);
			}
			if (*tmp->word == ';') {
			    tmp->prev->next = tmp->next;
			    tmp->next->prev = tmp->prev;
			    xfree((ptr_t) tmp->word);
			    del = tmp;
			    xfree((ptr_t) del);
			}
		    }
		}
	    }
	}
	for (we = we->next;
	     *we->word != ';' && we != cp;
	     we = we->next)
	    continue;
	if (*we->word == ';')
	    we = we->next;
    }
#ifdef RMDEBUG
    if (*tag) {
	xprintf("command line now is:\n");
	for (we = cp->next; we != cp; we = we->next)
	    xprintf("%S ", we->word);
    }
#endif /* RMDEBUG */
    return;
}

#ifdef BSDJOBS
/* Check if command is in continue list
   and do a "aliasing" if it exists as a job in background */

#undef CNDEBUG			/* For now */
void
continue_jobs(cp)
    struct wordent *cp;
{
    struct wordent *we;
    register struct process *pp, *np;
    Char   *cmd, *continue_list, *continue_args_list;

#ifdef CNDEBUG
    Char   *tag;
    static Char STRcndebug[] =
    {'c', 'n', 'd', 'e', 'b', 'u', 'g', '\0'};
#endif /* CNDEBUG */
    bool    in_cont_list, in_cont_arg_list;


#ifdef CNDEBUG
    tag = value(STRcndebug);
#endif /* CNDEBUG */
    continue_list = value(STRcontinue);
    continue_args_list = value(STRcontinue_args);
    if (*continue_list == '\0' && *continue_args_list == '\0')
	return;

    we = cp->next;
    while (*we->word == ';' && we != cp)
	we = we->next;
    while (we != cp) {
#ifdef CNDEBUG
	if (*tag)
	    xprintf("parsing command line\n");
#endif /* CNDEBUG */
	cmd = we->word;
	in_cont_list = inlist(continue_list, cmd);
	in_cont_arg_list = inlist(continue_args_list, cmd);
	if (in_cont_list || in_cont_arg_list) {
#ifdef CNDEBUG
	    if (*tag)
		xprintf("in one of the lists\n");
#endif /* CNDEBUG */
	    np = NULL;
	    for (pp = proclist.p_next; pp; pp = pp->p_next) {
		if (prefix(cmd, pp->p_command)) {
		    if (pp->p_index) {
			np = pp;
			break;
		    }
		}
	    }
	    if (np) {
		insert(we, in_cont_arg_list);
	    }
	}
	for (we = we->next;
	     *we->word != ';' && we != cp;
	     we = we->next)
	    continue;
	if (*we->word == ';')
	    we = we->next;
    }
#ifdef CNDEBUG
    if (*tag) {
	xprintf("command line now is:\n");
	for (we = cp->next; we != cp; we = we->next)
	    xprintf("%S ", we->word);
    }
#endif /* CNDEBUG */
    return;
}

/* The actual "aliasing" of for backgrounds() is done here
   with the aid of insert_we().   */
static void
insert(pl, file_args)
    struct wordent *pl;
    bool    file_args;
{
    struct wordent *now, *last;
    Char   *cmd, *bcmd, *cp1, *cp2;
    int     cmd_len;
    Char   *pause = STRunderpause;
    int     p_len = (int) Strlen(pause);

    cmd_len = (int) Strlen(pl->word);
    cmd = (Char *) xcalloc(1, (size_t) ((cmd_len + 1) * sizeof(Char)));
    (void) Strcpy(cmd, pl->word);
/* Do insertions at beginning, first replace command word */

    if (file_args) {
	now = pl;
	xfree((ptr_t) now->word);
	now->word = (Char *) xcalloc(1, (size_t) (5 * sizeof(Char)));
	(void) Strcpy(now->word, STRecho);

	now = (struct wordent *) xcalloc(1, (size_t) sizeof(struct wordent));
	now->word = (Char *) xcalloc(1, (size_t) (6 * sizeof(Char)));
	(void) Strcpy(now->word, STRbackqpwd);
	insert_we(now, pl);

	for (last = now; *last->word != '\n' && *last->word != ';';
	     last = last->next)
	    continue;

	now = (struct wordent *) xcalloc(1, (size_t) sizeof(struct wordent));
	now->word = (Char *) xcalloc(1, (size_t) (2 * sizeof(Char)));
	(void) Strcpy(now->word, STRgt);
	insert_we(now, last->prev);

	now = (struct wordent *) xcalloc(1, (size_t) sizeof(struct wordent));
	now->word = (Char *) xcalloc(1, (size_t) (2 * sizeof(Char)));
	(void) Strcpy(now->word, STRbang);
	insert_we(now, last->prev);

	now = (struct wordent *) xcalloc(1, (size_t) sizeof(struct wordent));
	now->word = (Char *) xcalloc(1, (size_t) cmd_len + p_len + 4);
	cp1 = now->word;
	cp2 = cmd;
	*cp1++ = '~';
	*cp1++ = '/';
	*cp1++ = '.';
	while ((*cp1++ = *cp2++) != '\0')
	    continue;
	cp1--;
	cp2 = pause;
	while ((*cp1++ = *cp2++) != '\0')
	    continue;
	insert_we(now, last->prev);

	now = (struct wordent *) xcalloc(1, (size_t) sizeof(struct wordent));
	now->word = (Char *) xcalloc(1, (size_t) (2 * sizeof(Char)));
	(void) Strcpy(now->word, STRsemi);
	insert_we(now, last->prev);
	bcmd = (Char *) xcalloc(1, (size_t) ((cmd_len + 2) * sizeof(Char)));
	cp1 = bcmd;
	cp2 = cmd;
	*cp1++ = '%';
	while ((*cp1++ = *cp2++) != '\0')
	    continue;
	now = (struct wordent *) xcalloc(1, (size_t) (sizeof(struct wordent)));
	now->word = bcmd;
	insert_we(now, last->prev);
    }
    else {
	struct wordent *del;

	now = pl;
	xfree((ptr_t) now->word);
	now->word = (Char *) xcalloc(1, 
				     (size_t) ((cmd_len + 2) * sizeof(Char)));
	cp1 = now->word;
	cp2 = cmd;
	*cp1++ = '%';
	while ((*cp1++ = *cp2++) != '\0')
	    continue;
	for (now = now->next;
	     *now->word != '\n' && *now->word != ';' && now != pl;) {
	    now->prev->next = now->next;
	    now->next->prev = now->prev;
	    xfree((ptr_t) now->word);
	    del = now;
	    now = now->next;
	    xfree((ptr_t) del);
	}
    }
}

static void
insert_we(new, where)
    struct wordent *new, *where;
{

    new->prev = where;
    new->next = where->next;
    where->next = new;
    new->next->prev = new;
}

static int
inlist(list, name)
    Char   *list, *name;
{
    register Char *l, *n;

    l = list;
    n = name;

    while (*l && *n) {
	if (*l == *n) {
	    l++;
	    n++;
	    if (*n == '\0' && (*l == ' ' || *l == '\0'))
		return (1);
	    else
		continue;
	}
	else {
	    while (*l && *l != ' ')
		l++;		/* skip to blank */
	    while (*l && *l == ' ')
		l++;		/* and find first nonblank character */
	    n = name;
	}
    }
    return (0);
}

#endif /* BSDJOBS */


/*
 * Implement a small cache for tilde names. This is used primarily
 * to expand tilde names to directories, but also
 * we can find users from their home directories for the tilde
 * prompt, on machines where yp lookup is slow this can be a big win...
 * As with any cache this can run out of sync, rehash can sync it again.
 */
static struct tildecache {
    Char   *user;
    Char   *home;
    int     hlen;
}      *tcache = NULL;

#define TILINCR 10
int tlength = 0;
static int tsize = TILINCR;

static int
tildecompare(p1, p2)
    struct tildecache *p1, *p2;
{
    return Strcmp(p1->user, p2->user);
}

static Char *
gethomedir(us)
    Char   *us;
{
    register struct passwd *pp;
#ifdef HESIOD
    char **res, **res1, *cp;
    Char *rp;
#endif /* HESIOD */
    
    pp = getpwnam(short2str(us));
#ifdef YPBUGS
    fix_yp_bugs();
#endif /* YPBUGS */
    if (pp != NULL)
	return Strsave(str2short(pp->pw_dir));
#ifdef HESIOD
    res = hes_resolve(short2str(us), "filsys");
    rp = 0;
    if (res != 0) {
	extern char *strtok();
	if ((*res) != 0) {
	    /*
	     * Look at the first token to determine how to interpret
	     * the rest of it.
	     * Yes, strtok is evil (it's not thread-safe), but it's also
	     * easy to use.
	     */
	    cp = strtok(*res, " ");
	    if (strcmp(cp, "AFS") == 0) {
		/* next token is AFS pathname.. */
		cp = strtok(NULL, " ");
		if (cp != NULL)
		    rp = Strsave(str2short(cp));
	    } else if (strcmp(cp, "NFS") == 0) {
		cp = NULL;
		if ((strtok(NULL, " ")) && /* skip remote pathname */
		    (strtok(NULL, " ")) && /* skip host */
		    (strtok(NULL, " ")) && /* skip mode */
		    (cp = strtok(NULL, " "))) {
		    rp = Strsave(str2short(cp));
		}
	    }
	}
	for (res1 = res; *res1; res1++)
	    free(*res1);
	return rp;
    }
#endif /* HESIOD */
    return NULL;
}

Char   *
gettilde(us)
    Char   *us;
{
    struct tildecache *bp1, *bp2, *bp;
    Char *hd;

    if (tcache == NULL)
	tcache = (struct tildecache *) xmalloc((size_t) (TILINCR *
						  sizeof(struct tildecache)));
    /*
     * Binary search
     */
    for (bp1 = tcache, bp2 = tcache + tlength; bp1 < bp2;) {
	register int i;

	bp = bp1 + ((bp2 - bp1) >> 1);
	if ((i = *us - *bp->user) == 0 && (i = Strcmp(us, bp->user)) == 0)
	    return (bp->home);
	if (i < 0)
	    bp2 = bp;
	else
	    bp1 = bp + 1;
    }
    /*
     * Not in the cache, try to get it from the passwd file
     */
    hd = gethomedir(us);
    if (hd == NULL)
	return NULL;

    /*
     * Update the cache
     */
    tcache[tlength].user = Strsave(us);
    tcache[tlength].home = hd;
    tcache[tlength++].hlen = (int) Strlen(hd);

    qsort((ptr_t) tcache, (size_t) tlength, sizeof(struct tildecache),
	  (int (*) __P((const void *, const void *))) tildecompare);

    if (tlength == tsize) {
	tsize += TILINCR;
	tcache = (struct tildecache *) xrealloc((ptr_t) tcache,
						(size_t) (tsize *
						  sizeof(struct tildecache)));
    }
    return (hd);
}

/*
 * Return the username if the directory path passed contains a
 * user's home directory in the tilde cache, otherwise return NULL
 * hm points to the place where the path became different.
 * Special case: Our own home directory.
 * If we are passed a null pointer, then we flush the cache.
 */
Char   *
getusername(hm)
    Char  **hm;
{
    Char   *h, *p;
    int     i, j;

    if (hm == NULL) {
	for (i = 0; i < tlength; i++) {
	    xfree((ptr_t) tcache[i].home);
	    xfree((ptr_t) tcache[i].user);
	}
	xfree((ptr_t) tcache);
	tlength = 0;
	tsize = TILINCR;
	tcache = NULL;
	return NULL;
    }
    if (((h = value(STRhome)) != STRNULL) &&
	(Strncmp(p = *hm, h, (size_t) (j = (int) Strlen(h))) == 0) &&
	(p[j] == '/' || p[j] == '\0')) {
	*hm = &p[j];
	return STRNULL;
    }
    for (i = 0; i < tlength; i++)
	if ((Strncmp(p = *hm, tcache[i].home, (size_t)
	    (j = tcache[i].hlen)) == 0) && (p[j] == '/' || p[j] == '\0')) {
	    *hm = &p[j];
	    return tcache[i].user;
	}
    return NULL;
}

/*
 * PWP: read a bunch of aliases out of a file QUICKLY.  The format
 *  is almost the same as the result of saying "alias > FILE", except
 *  that saying "aliases > FILE" does not expand non-letters to printable
 *  sequences.
 */
/*ARGSUSED*/
void
doaliases(v, c)
    Char  **v;
    struct command *c;
{
    jmp_buf_t oldexit;
    Char  **vec, *lp;
    int     fd;
    Char    buf[BUFSIZE], line[BUFSIZE];
    char    tbuf[BUFSIZE + 1], *tmp;
    extern bool output_raw;	/* PWP: in sh.print.c */

    USE(c);
    v++;
    if (*v == 0) {
	output_raw = 1;
	plist(&aliases, VAR_ALL);
	output_raw = 0;
	return;
    }

    gflag = 0, tglob(v);
    if (gflag) {
	v = globall(v);
	if (v == 0)
	    stderror(ERR_NAME | ERR_NOMATCH);
    }
    else {
	v = gargv = saveblk(v);
	trim(v);
    }

    if ((fd = open(tmp = short2str(*v), O_RDONLY)) < 0)
	stderror(ERR_NAME | ERR_SYSTEM, tmp, strerror(errno));

    getexit(oldexit);
    if (setexit() == 0) {
	for (;;) {
	    Char   *p = NULL;
	    int     n = 0;
	    lp = line;
	    for (;;) {
		if (n <= 0) {
		    int     i;

		    if ((n = read(fd, tbuf, BUFSIZE)) <= 0)
			goto eof;
		    for (i = 0; i < n; i++)
  			buf[i] = (Char) tbuf[i];
		    p = buf;
		}
		n--;
		if ((*lp++ = *p++) == '\n') {
		    lp[-1] = '\0';
		    break;
		}
	    }
	    for (lp = line; *lp; lp++) {
		if (isspc(*lp)) {
		    *lp++ = '\0';
		    while (isspc(*lp))
			lp++;
		    vec = (Char **) xmalloc((size_t)
					    (2 * sizeof(Char **)));
		    vec[0] = Strsave(lp);
		    vec[1] = NULL;
		    setq(strip(line), vec, &aliases, VAR_READWRITE);
		    break;
		}
	    }
	}
    }

eof:
    (void) close(fd);
    tw_cmd_free();
    if (gargv)
	blkfree(gargv), gargv = 0;
    resexit(oldexit);
}


/*
 * set the shell-level var to 1 or apply change to it.
 */
void
shlvl(val)
    int val;
{
    char *cp;

    if ((cp = getenv("SHLVL")) != NULL) {

	if (loginsh)
	    val = 1;
	else
	    val += atoi(cp);

	if (val <= 0) {
	    unsetv(STRshlvl);
	    Unsetenv(STRKSHLVL);
	}
	else {
	    Char    buff[BUFSIZE];

	    Itoa(val, buff);
	    set(STRshlvl, Strsave(buff), VAR_READWRITE);
	    tsetenv(STRKSHLVL, buff);
	}
    }
    else {
	set(STRshlvl, SAVE("1"), VAR_READWRITE);
	tsetenv(STRKSHLVL, str2short("1"));
    }
}


/* fixio():
 *	Try to recover from a read error
 */
int
fixio(fd, e)
    int fd, e;
{
    switch (e) {
    case -1:	/* Make sure that the code is reachable */

#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
# define TRY_AGAIN
#endif /* EWOULDBLOCK */

#if defined(POSIX) && defined(EAGAIN)
# if !defined(EWOULDBLOCK) || EWOULDBLOCK != EAGAIN
    case EAGAIN:
#  define TRY_AGAIN
# endif /* !EWOULDBLOCK || EWOULDBLOCK != EAGAIN */
#endif /* POSIX && EAGAIN */

	e = 0;
#ifdef TRY_AGAIN
# if defined(F_SETFL) && defined(O_NDELAY)
	if ((e = fcntl(fd, F_GETFL, 0)) == -1)
	    return -1;

	if (fcntl(fd, F_SETFL, e & ~O_NDELAY) == -1)
	    return -1;
	else 
	    e = 1;
# endif /* F_SETFL && O_NDELAY */

# ifdef FIONBIO
	e = 0;
	if (ioctl(fd, FIONBIO, (ioctl_t) &e) == -1)
	    return -1;
	else
	    e = 1;
# endif	/* FIONBIO */

#endif /* TRY_AGAIN */
	return e ? 0 : -1;

    case EINTR:
	return 0;

    default:
	return -1;
    }
}

/* collate():
 *	String collation
 */
int
collate(a, b)
    const Char *a;
    const Char *b;
{
    int rv;
#ifdef SHORT_STRINGS
    /* This strips the quote bit as a side effect */
    char *sa = strsave(short2str(a));
    char *sb = strsave(short2str(b));
#else
    char *sa = strip(strsave(a));
    char *sb = strip(strsave(b));
#endif /* SHORT_STRINGS */

#if defined(NLS) && !defined(NOSTRCOLL)
    errno = 0;	/* strcoll sets errno, another brain-damage */

    rv = strcoll(sa, sb);

    if (errno != 0) {
	xfree((ptr_t) sa);
	xfree((ptr_t) sb);
	stderror(ERR_SYSTEM, "strcoll", strerror(errno));
    }
#else
    rv = strcmp(sa, sb);
#endif /* NLS && !NOSTRCOLL */

    xfree((ptr_t) sa);
    xfree((ptr_t) sb);

    return rv;
}

#ifdef HASHBANG
/*
 * From: peter@zeus.dialix.oz.au (Peter Wemm)
 * If exec() fails look first for a #! [word] [word] ....
 * If it is, splice the header into the argument list and retry.
 */
#define HACKBUFSZ 1024		/* Max chars in #! vector */
#define HACKVECSZ 128		/* Max words in #! vector */
int
hashbang(fd, vp)
    int fd;
    Char ***vp;
{
    unsigned char lbuf[HACKBUFSZ];
    char *sargv[HACKVECSZ];
    unsigned char *p, *ws;
    int sargc = 0;

    if (read(fd, (char *) lbuf, HACKBUFSZ) <= 0)
	return -1;

    ws = 0;	/* word started = 0 */

    for (p = lbuf; p < &lbuf[HACKBUFSZ]; )
	switch (*p) {
	case ' ':
	case '\t':
	    if (ws) {	/* a blank after a word.. save it */
		*p = '\0';
		if (sargc < HACKVECSZ - 1)
		    sargv[sargc++] = ws;
		ws = NULL;
	    }
	    p++;
	    continue;

	case '\0':	/* Whoa!! what the hell happened */
	    return -1;

	case '\n':	/* The end of the line. */
	    if (ws) {	/* terminate the last word */
		*p = '\0';
		if (sargc < HACKVECSZ - 1)
		    sargv[sargc++] = ws;
		sargv[sargc] = NULL;
		ws = NULL;
		*vp = blk2short(sargv);
		return 0;
	    }
	    else
		return -1;

	default:
	    if (!ws)	/* Start a new word? */
		ws = p; 
	    p++;
	    break;
	}
    return -1;
}
#endif /* HASHBANG */
