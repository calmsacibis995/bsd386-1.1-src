/* NAME:
 *      sigact.h - sigaction et al
 *
 * SYNOPSIS:
 *      #include "sigact.h"
 *
 * DESCRIPTION:
 *      This header is the interface to a fake sigaction(2) 
 *      implementation. It provides a POSIX compliant interface 
 *      to whatever signal handling mechanisms are available.
 *      It also provides a Signal() function that is implemented 
 *      in terms of sigaction().
 *      If not using signal(2) as part of the underlying 
 *      implementation (USE_SIGNAL or USE_SIGMASK), and 
 *      NO_SIGNAL is not defined, it also provides a signal() 
 *      function that calls Signal(). 
 *      
 * SEE ALSO:
 *      sigact.c
 */
/*
 * RCSid:
 *      $Id: sigact.h,v 1.2 1993/12/21 01:52:02 polk Exp $
 */
#ifndef _SIGACT_H
#define _SIGACT_H

/*
 * most modern systems use void for signal handlers but
 * not all.
 */
#ifndef SIG_HDLR
# define SIG_HDLR void
#endif

#undef ARGS
#if defined(__STDC__) || defined(__cplusplus)
# define ARGS(p) p
#else
# define ARGS(p) ()
# define volatile			/* don't optimize please */
# define const				/* read only */
#endif

SIG_HDLR (*Signal	ARGS((int sig, SIG_HDLR (*disp)(int)))) ARGS((int)); 

/*
 * if you want to install this header as signal.h,
 * modify this to pick up the original signal.h
 */
#ifndef SIGKILL
# include <signal.h>
#endif
  
#ifndef SIG_ERR
# define SIG_ERR  (SIG_HDLR (*)())-1
#endif
#ifndef BADSIG
# define BADSIG  SIG_ERR
#endif
    
#ifndef SA_NOCLDSTOP
/* we assume we need the fake sigaction */
/* sa_flags */
#define	SA_NOCLDSTOP	1		/* don't send SIGCHLD on child stop */
#define SA_RESTART	2		/* re-start I/O */

/* sigprocmask flags */
#define	SIG_BLOCK	1
#define	SIG_UNBLOCK	2
#define	SIG_SETMASK	4

/*
 * this is a bit untidy
 */
#if !defined(__sys_stdtypes_h)
typedef unsigned int sigset_t;
#endif
  
/*
 * POSIX sa_handler should return void, but since we are
 * implementing in terms of something else, it may
 * be appropriate to use the normal SIG_HDLR return type
 */
struct sigaction
{
  SIG_HDLR	(*sa_handler)();
  sigset_t	sa_mask;
  int		sa_flags;
};


int	sigaction	ARGS(( int sig, struct sigaction *act, struct sigaction *oact ));
int	sigaddset	ARGS(( sigset_t *mask, int sig ));
int	sigdelset	ARGS(( sigset_t *mask, int sig ));
int	sigemptyset	ARGS(( sigset_t *mask ));
int	sigfillset	ARGS(( sigset_t *mask ));
int	sigismember	ARGS(( sigset_t *mask, int sig ));
int	sigpending	ARGS(( sigset_t *set ));
int	sigprocmask	ARGS(( int how, sigset_t *set, sigset_t *oset ));
int	sigsuspend	ARGS(( sigset_t *mask ));
	
#ifndef sigmask
# define sigmask(s)	(1<<((s)-1))	/* convert SIGnum to mask */
#endif
#if !defined(NSIG) && defined(_NSIG)
# define NSIG _NSIG
#endif
#endif /* ! SA_NOCLDSTOP */
#endif /* _SIGACT_H */
