/* $Header: /bsdi/MASTER/BSDI_OS/contrib/tcsh/sh.sem.c,v 1.2 1994/01/13 23:04:29 polk Exp $ */
/*
 * sh.sem.c: I/O redirections and job forking. A touchy issue!
 *	     Most stuff with builtins is incorrect
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

RCSID("$Id: sh.sem.c,v 1.2 1994/01/13 23:04:29 polk Exp $")

#include "tc.h"

#ifdef CLOSE_ON_EXEC
# ifndef SUNOS4
#  ifndef CLEX_DUPS
#   define CLEX_DUPS
#  endif /* CLEX_DUPS */
# endif /* !SUNOS4 */
#endif /* CLOSE_ON_EXEC */

#if defined(__sparc__) || defined(sparc)
# if !defined(MACH) && SYSVREL == 0 && !defined(Lynx)
#  include <vfork.h>
# endif /* !MACH && SYSVREL == 0 */
#endif /* __sparc__ || sparc */

#ifdef VFORK
static	sigret_t	vffree	__P((int));
#endif 
static	Char		*splicepipe	__P((struct command *, Char *));
static	void		 doio		__P((struct command *, int *, int *));
static	void		 chkclob	__P((char *));

/*
 * C shell
 */

/*
 * For SVR4, there are problems with pipelines having the first process as
 * the group leader.  The problem occurs when the first process exits before
 * the others have a chance to setpgid().  This is because in SVR4 you can't
 * have a zombie as a group leader.  The solution I have used is to reverse
 * the order in which pipelines are started, making the last process the
 * group leader.  (Note I am not using 'pipeline' in the generic sense -- I
 * mean processes connected by '|'.)  I don't know yet if this causes other
 * problems.
 *
 * All the changes for this are in execute(), and are enclosed in 
 * '#ifdef BACKPIPE'
 *
 * David Dawes (dawes@physics.su.oz.au) Oct 1991
 */

static struct {
    int wanttty;
    struct biltins *bifunc;
    bool forked;
} _gv;

#define VOL_SAVE()	( bifunc = _gv.bifunc, \
			  forked = _gv.forked, \
			  owanttty = _gv.wanttty )
#define VOL_RESTORE() (	_gv.bifunc = bifunc, \
			_gv.forked = forked, \
			_gv.wanttty = owanttty )


/*VARARGS 1*/
void
execute(t, wanttty, pipein, pipeout)
    register struct command *t;
    int     wanttty;
    int *pipein, *pipeout;
{
#ifdef VFORK
    extern bool use_fork;	/* use fork() instead of vfork()? */
#endif

    bool    forked;
    struct biltins *bifunc;
    int     pid = 0, owanttty;
    int     pv[2];
#ifdef BSDSIGS
    static sigmask_t csigmask;
#endif /* BSDSIGS */
#ifdef VFORK
    static int onosigchld = 0;
#endif /* VFORK */
    static int nosigchld = 0;

    if (t == 0) 
	return;

    VOL_SAVE();
    _gv.forked = 0;
    _gv.wanttty = wanttty;
    _gv.bifunc = NULL;

    /*
     * From: Michael Schroeder <mlschroe@immd4.informatik.uni-erlangen.de>
     * Don't check for wantty > 0...
     */
    if (t->t_dflg & F_AMPERSAND)
	_gv.wanttty = 0;
    switch (t->t_dtyp) {

    case NODE_COMMAND:
	if ((t->t_dcom[0][0] & (QUOTE | TRIM)) == QUOTE)
	    (void) Strcpy(t->t_dcom[0], t->t_dcom[0] + 1);
	if ((t->t_dflg & F_REPEAT) == 0)
	    Dfix(t);		/* $ " ' \ */
	if (t->t_dcom[0] == 0) {
	    VOL_RESTORE();
	    return;
	}
	/*FALLTHROUGH*/

    case NODE_PAREN:
#ifdef BACKPIPE
	if (t->t_dflg & F_PIPEIN)
	    mypipe(pipein);
#else /* !BACKPIPE */
	if (t->t_dflg & F_PIPEOUT)
	    mypipe(pipeout);
#endif /* BACKPIPE */
	/*
	 * Must do << early so parent will know where input pointer should be.
	 * If noexec then this is all we do.
	 */
	if (t->t_dflg & F_READ) {
	    (void) close(0);
	    heredoc(t->t_dlef);
	    if (noexec)
		(void) close(0);
	}

	set(STRstatus, Strsave(STR0), VAR_READWRITE);

	/*
	 * This mess is the necessary kludge to handle the prefix builtins:
	 * nice, nohup, time.  These commands can also be used by themselves,
	 * and this is not handled here. This will also work when loops are
	 * parsed.
	 */
	while (t->t_dtyp == NODE_COMMAND)
	    if (eq(t->t_dcom[0], STRnice))
		if (t->t_dcom[1])
		    if (strchr("+-", t->t_dcom[1][0]))
			if (t->t_dcom[2]) {
			    setname("nice");
			    t->t_nice =
				getn(t->t_dcom[1]);
			    lshift(t->t_dcom, 2);
			    t->t_dflg |= F_NICE;
			}
			else
			    break;
		    else {
			t->t_nice = 4;
			lshift(t->t_dcom, 1);
			t->t_dflg |= F_NICE;
		    }
		else
		    break;
	    else if (eq(t->t_dcom[0], STRnohup))
		if (t->t_dcom[1]) {
		    t->t_dflg |= F_NOHUP;
		    lshift(t->t_dcom, 1);
		}
		else
		    break;
	    else if (eq(t->t_dcom[0], STRtime))
		if (t->t_dcom[1]) {
		    t->t_dflg |= F_TIME;
		    lshift(t->t_dcom, 1);
		}
		else
		    break;
#ifdef F_VER
	    else if (eq(t->t_dcom[0], STRver))
		if (t->t_dcom[1] && t->t_dcom[2]) {
		    setname("ver");
		    t->t_systype = getv(t->t_dcom[1]);
		    lshift(t->t_dcom, 2);
		    t->t_dflg |= F_VER;
		}
		else
		    break;
#endif  /* F_VER */
	    else
		break;

	/* is it a command */
	if (t->t_dtyp == NODE_COMMAND) {
	    /*
	     * Check if we have a builtin function and remember which one.
	     */
	    _gv.bifunc = isbfunc(t);
 	    if (noexec && _gv.bifunc) {
		/*
		 * Continue for builtins that are part of the scripting language
		 */
		if (_gv.bifunc->bfunct != (void (*)())dobreak	&&
		    _gv.bifunc->bfunct != (void (*)())docontin	&&
		    _gv.bifunc->bfunct != (void (*)())doelse	&&
		    _gv.bifunc->bfunct != (void (*)())doend	&&
		    _gv.bifunc->bfunct != (void (*)())doforeach	&&
		    _gv.bifunc->bfunct != (void (*)())dogoto	&&
		    _gv.bifunc->bfunct != (void (*)())doif	&&
		    _gv.bifunc->bfunct != (void (*)())dorepeat	&&
		    _gv.bifunc->bfunct != (void (*)())doswbrk	&&
		    _gv.bifunc->bfunct != (void (*)())doswitch	&&
		    _gv.bifunc->bfunct != (void (*)())dowhile	&&
		    _gv.bifunc->bfunct != (void (*)())dozip)
		    break;
	    }
	}
	else {			/* not a command */
	    _gv.bifunc = NULL;
	    if (noexec)
		break;
	}

	/*
	 * We fork only if we are timed, or are not the end of a parenthesized
	 * list and not a simple builtin function. Simple meaning one that is
	 * not pipedout, niced, nohupped, or &'d. It would be nice(?) to not
	 * fork in some of these cases.
	 */
	/*
	 * Prevent forking cd, pushd, popd, chdir cause this will cause the
	 * shell not to change dir!
	 */
#ifdef BACKPIPE
	/*
	 * Can't have NOFORK for the tail of a pipe - because it is not the
	 * last command spawned (even if it is at the end of a parenthesised
	 * list).
	 */
	if (t->t_dflg & F_PIPEIN)
	    t->t_dflg &= ~(F_NOFORK);
#endif /* BACKPIPE */
	if (_gv.bifunc && (_gv.bifunc->bfunct == (void(*)())dochngd ||
			   _gv.bifunc->bfunct == (void(*)())dopushd ||
			   _gv.bifunc->bfunct == (void(*)())dopopd))
	    t->t_dflg &= ~(F_NICE);
	if (((t->t_dflg & F_TIME) || ((t->t_dflg & F_NOFORK) == 0 &&
	     (!_gv.bifunc || t->t_dflg &
	      (F_PIPEOUT | F_AMPERSAND | F_NICE | F_NOHUP)))) ||
	/*
	 * We have to fork for eval too.
	 */
	    (_gv.bifunc && (t->t_dflg & F_PIPEIN) != 0 &&
	     _gv.bifunc->bfunct == (void(*)())doeval))
#ifdef VFORK
	    if (t->t_dtyp == NODE_PAREN ||
		t->t_dflg & (F_REPEAT | F_AMPERSAND) || _gv.bifunc)
#endif /* VFORK */
	    {
		_gv.forked++;
		/*
		 * We need to block SIGCHLD here, so that if the process does
		 * not die before we can set the process group
		 */
		if (_gv.wanttty >= 0 && !nosigchld) {
#ifdef BSDSIGS
		    csigmask = sigblock(sigmask(SIGCHLD));
#else /* !BSDSIGS */
		    (void) sighold(SIGCHLD);
#endif /* BSDSIGS */

		    nosigchld = 1;
		}

		pid = pfork(t, _gv.wanttty);
		if (pid == 0 && nosigchld) {
#ifdef BSDSIGS
		    (void) sigsetmask(csigmask);
#else /* !BSDSIGS */
		    (void) sigrelse(SIGCHLD);
#endif /* BSDSIGS */
		    nosigchld = 0;
		}
		else if (pid != 0 && (t->t_dflg & F_AMPERSAND))
		    backpid = pid;
	    }

#ifdef VFORK
	    else {
		int     ochild, osetintr, ohaderr, odidfds;
		int     oSHIN, oSHOUT, oSHDIAG, oOLDSTD, otpgrp;
		int     oisoutatty, oisdiagatty;

# ifndef CLOSE_ON_EXEC
		int     odidcch;
# endif  /* !CLOSE_ON_EXEC */
# ifdef BSDSIGS
		sigmask_t omask, ocsigmask;
# endif /* BSDSIGS */

		/*
		 * Prepare for the vfork by saving everything that the child
		 * corrupts before it exec's. Note that in some signal
		 * implementations which keep the signal info in user space
		 * (e.g. Sun's) it will also be necessary to save and restore
		 * the current sigvec's for the signals the child touches
		 * before it exec's.
		 */

		/*
		 * Sooooo true... If this is a Sun, save the sigvec's. (Skip
		 * Gilbrech - 11/22/87)
		 */
# ifdef SAVESIGVEC
		sigvec_t savesv[NSIGSAVED];
		sigmask_t savesm;

# endif /* SAVESIGVEC */
		if (_gv.wanttty >= 0 && !nosigchld && !noexec) {
# ifdef BSDSIGS
		    csigmask = sigblock(sigmask(SIGCHLD));
# else /* !BSDSIGS */
		    (void) sighold(SIGCHLD);
# endif  /* BSDSIGS */
		    nosigchld = 1;
		}
# ifdef BSDSIGS
		omask = sigblock(sigmask(SIGCHLD)|sigmask(SIGINT));
# else /* !BSDSIGS */
		(void) sighold(SIGCHLD);
		(void) sighold(SIGINT);
# endif  /* BSDSIGS */
		ochild = child;
		osetintr = setintr;
		ohaderr = haderr;
		odidfds = didfds;
# ifndef CLOSE_ON_EXEC
		odidcch = didcch;
# endif /* !CLOSE_ON_EXEC */
		oSHIN = SHIN;
		oSHOUT = SHOUT;
		oSHDIAG = SHDIAG;
		oOLDSTD = OLDSTD;
		otpgrp = tpgrp;
		oisoutatty = isoutatty;
		oisdiagatty = isdiagatty;
# ifdef BSDSIGS
		ocsigmask = csigmask;
# endif /* BSDSIGS */
		onosigchld = nosigchld;
		Vsav = Vdp = 0;
		Vexpath = 0;
		Vt = 0;
# ifdef SAVESIGVEC
		savesm = savesigvec(savesv);
# endif /* SAVESIGVEC */
		if (use_fork)
		    pid = fork();
		else
		    pid = vfork();

		if (pid < 0) {
# ifdef BSDSIGS
#  ifdef SAVESIGVEC
		    restoresigvec(savesv, savesm);
#  endif /* SAVESIGVEC */
		    (void) sigsetmask(omask);
# else /* !BSDSIGS */
		    (void) sigrelse(SIGCHLD);
		    (void) sigrelse(SIGINT);
# endif  /* BSDSIGS */
		    stderror(ERR_NOPROC);
		}
		_gv.forked++;
		if (pid) {	/* parent */
# ifdef SAVESIGVEC
		    restoresigvec(savesv, savesm);
# endif /* SAVESIGVEC */
		    child = ochild;
		    setintr = osetintr;
		    haderr = ohaderr;
		    didfds = odidfds;
		    SHIN = oSHIN;
# ifndef CLOSE_ON_EXEC
		    didcch = odidcch;
# endif /* !CLOSE_ON_EXEC */
		    SHOUT = oSHOUT;
		    SHDIAG = oSHDIAG;
		    OLDSTD = oOLDSTD;
		    tpgrp = otpgrp;
		    isoutatty = oisoutatty;
		    isdiagatty = oisdiagatty;
# ifdef BSDSIGS
		    csigmask = ocsigmask;
# endif /* BSDSIGS */
		    nosigchld = onosigchld;

		    xfree((ptr_t) Vsav);
		    Vsav = 0;
		    xfree((ptr_t) Vdp);
		    Vdp = 0;
		    xfree((ptr_t) Vexpath);
		    Vexpath = 0;
		    blkfree((Char **) Vt);
		    Vt = 0;
		    /* this is from pfork() */
		    palloc(pid, t);
# ifdef BSDSIGS
		    (void) sigsetmask(omask);
# else /* !BSDSIGS */
		    (void) sigrelse(SIGCHLD);
		    (void) sigrelse(SIGINT);
# endif  /* BSDSIGS */
		}
		else {		/* child */
		    /* this is from pfork() */
		    int     pgrp;
		    bool    ignint = 0;
		    if (nosigchld) {
# ifdef BSDSIGS
			(void) sigsetmask(csigmask);
# else /* !BSDSIGS */
			(void) sigrelse(SIGCHLD);
# endif /* BSDSIGS */
			nosigchld = 0;
		    }

		    if (setintr)
			ignint = (tpgrp == -1 && (t->t_dflg & F_NOINTERRUPT))
				|| (gointr && eq(gointr, STRminus));
		    pgrp = pcurrjob ? pcurrjob->p_jobid : getpid();
		    child++;
		    if (setintr) {
			setintr = 0;
/*
 * casts made right for SunOS 4.0 by Douglas C. Schmidt
 * <schmidt%sunshine.ics.uci.edu@ROME.ICS.UCI.EDU>
 * (thanks! -- PWP)
 *
 * ignint ifs cleaned by Johan Widen <mcvax!osiris.sics.se!jw@uunet.UU.NET>
 * (thanks again)
 */
			if (ignint) {
			    (void) signal(SIGINT, SIG_IGN);
			    (void) signal(SIGQUIT, SIG_IGN);
			}
			else {
			    (void) signal(SIGINT,  vffree);
			    (void) signal(SIGQUIT, SIG_DFL);
			}
# ifdef BSDJOBS
			if (_gv.wanttty >= 0) {
			    (void) signal(SIGTSTP, SIG_DFL);
			    (void) signal(SIGTTIN, SIG_DFL);
			    (void) signal(SIGTTOU, SIG_DFL);
			}
# endif /* BSDJOBS */

			(void) signal(SIGTERM, parterm);
		    }
		    else if (tpgrp == -1 &&
			     (t->t_dflg & F_NOINTERRUPT)) {
			(void) signal(SIGINT, SIG_IGN);
			(void) signal(SIGQUIT, SIG_IGN);
		    }

		    pgetty(_gv.wanttty, pgrp);

		    if (t->t_dflg & F_NOHUP)
			(void) signal(SIGHUP, SIG_IGN);
		    if (t->t_dflg & F_NICE)
# ifdef BSDNICE
			(void) setpriority(PRIO_PROCESS, 0, t->t_nice);
# else /* !BSDNICE */
			(void) nice(t->t_nice);
# endif /* BSDNICE */
# ifdef F_VER
		    if (t->t_dflg & F_VER) {
			tsetenv(STRSYSTYPE, t->t_systype ? STRbsd43 : STRsys53);
			dohash(NULL, NULL);
		    }
# endif /* F_VER */
		}

	    }
#endif /* VFORK */
	if (pid != 0) {
	    /*
	     * It would be better if we could wait for the whole job when we
	     * knew the last process had been started.  Pwait, in fact, does
	     * wait for the whole job anyway, but this test doesn't really
	     * express our intentions.
	     */
#ifdef BACKPIPE
	    if (didfds == 0 && t->t_dflg & F_PIPEOUT) {
		(void) close(pipeout[0]);
		(void) close(pipeout[1]);
	    }
	    if ((t->t_dflg & F_PIPEIN) != 0)
		break;
#else /* !BACKPIPE */
	    if (didfds == 0 && t->t_dflg & F_PIPEIN) {
		(void) close(pipein[0]);
		(void) close(pipein[1]);
	    }
	    if ((t->t_dflg & F_PIPEOUT) != 0)
		break;
#endif /* BACKPIPE */

	    if (nosigchld) {
#ifdef BSDSIGS
		(void) sigsetmask(csigmask);
#else /* !BSDSIGS */
		(void) sigrelse(SIGCHLD);
#endif /* BSDSIGS */
		nosigchld = 0;
	    }
	    if ((t->t_dflg & F_AMPERSAND) == 0)
		pwait();
	    break;
	}

	doio(t, pipein, pipeout);
#ifdef BACKPIPE
	if (t->t_dflg & F_PIPEIN) {
	    (void) close(pipein[0]);
	    (void) close(pipein[1]);
	}
#else /* !BACKPIPE */
	if (t->t_dflg & F_PIPEOUT) {
	    (void) close(pipeout[0]);
	    (void) close(pipeout[1]);
	}
#endif /* BACKPIPE */
	/*
	 * Perform a builtin function. If we are not forked, arrange for
	 * possible stopping
	 */
	if (_gv.bifunc) {
	    func(t, _gv.bifunc);
	    if (_gv.forked)
		exitstat();
	    break;
	}
	if (t->t_dtyp != NODE_PAREN) {
	    doexec(t);
	    /* NOTREACHED */
	}
	/*
	 * For () commands must put new 0,1,2 in FSH* and recurse
	 */
	OLDSTD = dcopy(0, FOLDSTD);
	SHOUT = dcopy(1, FSHOUT);
	isoutatty = isatty(SHOUT);
	SHDIAG = dcopy(2, FSHDIAG);
	isdiagatty = isatty(SHDIAG);
	(void) close(SHIN);
	SHIN = -1;
#ifndef CLOSE_ON_EXEC
	didcch = 0;
#endif /* !CLOSE_ON_EXEC */
	didfds = 0;
	_gv.wanttty = -1;
	t->t_dspr->t_dflg |= t->t_dflg & F_NOINTERRUPT;
	execute(t->t_dspr, _gv.wanttty, NULL, NULL);
	exitstat();

    case NODE_PIPE:
#ifdef BACKPIPE
	t->t_dcdr->t_dflg |= F_PIPEIN | (t->t_dflg &
			(F_PIPEOUT | F_AMPERSAND | F_NOFORK | F_NOINTERRUPT));
	execute(t->t_dcdr, _gv.wanttty, pv, pipeout);
	t->t_dcar->t_dflg |= F_PIPEOUT |
	    (t->t_dflg & (F_PIPEIN | F_AMPERSAND | F_STDERR | F_NOINTERRUPT));
	execute(t->t_dcar, _gv.wanttty, pipein, pv);
#else /* !BACKPIPE */
	t->t_dcar->t_dflg |= F_PIPEOUT |
	    (t->t_dflg & (F_PIPEIN | F_AMPERSAND | F_STDERR | F_NOINTERRUPT));
	execute(t->t_dcar, _gv.wanttty, pipein, pv);
	t->t_dcdr->t_dflg |= F_PIPEIN | (t->t_dflg &
			(F_PIPEOUT | F_AMPERSAND | F_NOFORK | F_NOINTERRUPT));
	execute(t->t_dcdr, _gv.wanttty, pv, pipeout);
#endif /* BACKPIPE */
	break;

    case NODE_LIST:
	if (t->t_dcar) {
	    t->t_dcar->t_dflg |= t->t_dflg & F_NOINTERRUPT;
	    execute(t->t_dcar, _gv.wanttty, NULL, NULL);
	    /*
	     * In strange case of A&B make a new job after A
	     */
	    if (t->t_dcar->t_dflg & F_AMPERSAND && t->t_dcdr &&
		(t->t_dcdr->t_dflg & F_AMPERSAND) == 0)
		pendjob();
	}
	if (t->t_dcdr) {
	    t->t_dcdr->t_dflg |= t->t_dflg &
		(F_NOFORK | F_NOINTERRUPT);
	    execute(t->t_dcdr, _gv.wanttty, NULL, NULL);
	}
	break;

    case NODE_OR:
    case NODE_AND:
	if (t->t_dcar) {
	    t->t_dcar->t_dflg |= t->t_dflg & F_NOINTERRUPT;
	    execute(t->t_dcar, _gv.wanttty, NULL, NULL);
	    if ((getn(value(STRstatus)) == 0) !=
		(t->t_dtyp == NODE_AND)) {
		VOL_RESTORE();
		return;
	    }
	}
	if (t->t_dcdr) {
	    t->t_dcdr->t_dflg |= t->t_dflg &
		(F_NOFORK | F_NOINTERRUPT);
	    execute(t->t_dcdr, _gv.wanttty, NULL, NULL);
	}
	break;

    default:
	break;
    }
    /*
     * Fall through for all breaks from switch
     * 
     * If there will be no more executions of this command, flush all file
     * descriptors. Places that turn on the F_REPEAT bit are responsible for
     * doing donefds after the last re-execution
     */
    if (didfds && !(t->t_dflg & F_REPEAT))
	donefds();
    VOL_RESTORE();
}

#ifdef VFORK
static sigret_t
/*ARGSUSED*/
vffree(snum)
int snum;
{
    register Char **v;

    USE(snum);
    if ((v = gargv) != 0) {
	gargv = 0;
	xfree((ptr_t) v);
    }

    if ((v = pargv) != 0) {
	pargv = 0;
	xfree((ptr_t) v);
    }

    _exit(1);
#ifndef SIGVOID
    /*NOTREACHED*/
    return(0);
#endif /* SIGVOID */
}
#endif /* VFORK */

/*
 * Expand and glob the words after an i/o redirection.
 * If more than one word is generated, then update the command vector.
 *
 * This is done differently in all the shells:
 * 1. in the bourne shell and ksh globbing is not performed
 * 2. Bash/csh say ambiguous
 * 3. zsh does i/o to/from all the files
 * 4. itcsh concatenates the words.
 *
 * I don't know what is best to do. I think that Ambiguous is better
 * than restructuring the command vector, because the user can get
 * unexpected results. In any case, the command vector restructuring 
 * code is present and the user can choose it by setting noambiguous
 */
static Char *
splicepipe(t, cp)
    register struct command *t;
    Char *cp;	/* word after < or > */
{
    Char *blk[2];

    if (adrof(STRnoambiguous)) {
	Char **pv;

	blk[0] = Dfix1(cp); /* expand $ */
	blk[1] = NULL;

	gflag = 0, tglob(blk);
	if (gflag) {
	    pv = globall(blk);
	    if (pv == NULL) {
		setname(short2str(blk[0]));
		xfree((ptr_t) blk[0]);
		stderror(ERR_NAME | ERR_NOMATCH);
	    }
	    gargv = NULL;
	    if (pv[1] != NULL) { /* we need to fix the command vector */
		Char **av = blkspl(t->t_dcom, &pv[1]);
		xfree((ptr_t) t->t_dcom);
		t->t_dcom = av;
	    }
	    xfree((ptr_t) blk[0]);
	    blk[0] = pv[0];
	    xfree((ptr_t) pv);
	}
    }
    else {
	Char buf[BUFSIZE];

	(void) Strcpy(buf, blk[1] = Dfix1(cp));
	xfree((ptr_t) blk[1]);
	blk[0] = globone(buf, G_ERROR);
    }
    return(blk[0]);
}
    
/*
 * Perform io redirection.
 * We may or maynot be forked here.
 */
static void
doio(t, pipein, pipeout)
    register struct command *t;
    int    *pipein, *pipeout;
{
    register int fd;
    register Char *cp;
    register int flags = t->t_dflg;

    if (didfds || (flags & F_REPEAT))
	return;
    if ((flags & F_READ) == 0) {/* F_READ already done */
	if (t->t_dlef) {
	    char    tmp[MAXPATHLEN+1];

	    /*
	     * so < /dev/std{in,out,err} work
	     */
	    (void) dcopy(SHIN, 0);
	    (void) dcopy(SHOUT, 1);
	    (void) dcopy(SHDIAG, 2);
	    cp = splicepipe(t, t->t_dlef);
	    (void) strncpy(tmp, short2str(cp), MAXPATHLEN);
	    tmp[MAXPATHLEN] = '\0';
	    xfree((ptr_t) cp);
	    if ((fd = open(tmp, O_RDONLY)) < 0)
		stderror(ERR_SYSTEM, tmp, strerror(errno));
	    (void) dmove(fd, 0);
	}
	else if (flags & F_PIPEIN) {
	    (void) close(0);
	    (void) dup(pipein[0]);
	    (void) close(pipein[0]);
	    (void) close(pipein[1]);
	}
	else if ((flags & F_NOINTERRUPT) && tpgrp == -1) {
	    (void) close(0);
	    (void) open(_PATH_DEVNULL, O_RDONLY);
	}
	else {
	    (void) close(0);
	    (void) dup(OLDSTD);
#if defined(CLOSE_ON_EXEC) && defined(CLEX_DUPS)
	    /*
	     * PWP: Unlike Bezerkeley 4.3, FIONCLEX for Pyramid is preserved
	     * across dup()s, so we have to UNSET it here or else we get a
	     * command with NO stdin, stdout, or stderr at all (a bad thing
	     * indeed)
	     */
	    (void) close_on_exec(0, 0);
#endif /* CLOSE_ON_EXEC && CLEX_DUPS */
	}
    }
    if (t->t_drit) {
	char    tmp[MAXPATHLEN+1];

	cp = splicepipe(t, t->t_drit);
	(void) strncpy(tmp, short2str(cp), MAXPATHLEN);
	tmp[MAXPATHLEN] = '\0';
	xfree((ptr_t) cp);
	/*
	 * so > /dev/std{out,err} work
	 */
	(void) dcopy(SHOUT, 1);
	(void) dcopy(SHDIAG, 2);
	if ((flags & F_APPEND) != 0) {
#ifdef O_APPEND
	    fd = open(tmp, O_WRONLY | O_APPEND);
#else /* !O_APPEND */
	    fd = open(tmp, O_WRONLY);
	    (void) lseek(1, (off_t) 0, L_XTND);
#endif /* O_APPEND */
	}
	else
	    fd = 0;
	if ((flags & F_APPEND) == 0 || fd == -1) {
	    if (!(flags & F_OVERWRITE) && adrof(STRnoclobber)) {
		if (flags & F_APPEND)
		    stderror(ERR_SYSTEM, tmp, strerror(errno));
		chkclob(tmp);
	    }
	    if ((fd = creat(tmp, 0666)) < 0)
		stderror(ERR_SYSTEM, tmp, strerror(errno));
	}
	(void) dmove(fd, 1);
	is1atty = isatty(1);
    }
    else if (flags & F_PIPEOUT) {
	(void) close(1);
	(void) dup(pipeout[1]);
	is1atty = 0;
    }
    else {
	(void) close(1);
	(void) dup(SHOUT);
	is1atty = isoutatty;
# if defined(CLOSE_ON_EXEC) && defined(CLEX_DUPS)
	(void) close_on_exec(1, 0);
# endif /* CLOSE_ON_EXEC && CLEX_DUPS */
    }

    (void) close(2);
    if (flags & F_STDERR) {
	(void) dup(1);
	is2atty = is1atty;
    }
    else {
	(void) dup(SHDIAG);
	is2atty = isdiagatty;
# if defined(CLOSE_ON_EXEC) && defined(CLEX_DUPS)
	(void) close_on_exec(2, 0);
# endif /* CLOSE_ON_EXEC && CLEX_DUPS */
    }
    didfds = 1;
}

void
mypipe(pv)
    register int *pv;
{

    if (pipe(pv) < 0)
	goto oops;
    pv[0] = dmove(pv[0], -1);
    pv[1] = dmove(pv[1], -1);
    if (pv[0] >= 0 && pv[1] >= 0)
	return;
oops:
    stderror(ERR_PIPE);
}

static void
chkclob(cp)
    register char *cp;
{
    struct stat stb;

    if (stat(cp, &stb) < 0)
	return;
    if (S_ISCHR(stb.st_mode))
	return;
    stderror(ERR_EXISTS, cp);
}
