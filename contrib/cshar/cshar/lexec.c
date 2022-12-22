/*
**  Process stuff, like fork exec and wait.  Also signals.
*/
/* LINTLIBRARY */
#include "shar.h"
#include <signal.h>
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/lexec.c,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $";
#endif	/* RCSID */


/* How to fork(), what to wait with. */
#ifdef	SYS_WAIT
#include <sys/wait.h>
#define FORK()		 vfork()
#define W_VAL(w)	 ((w).w_retcode)
typedef union wait	 WAITER;
#else
#define FORK()		 fork()
#define W_VAL(w)	 ((w) >> 8)
typedef int		 WAITER;
#endif	/* SYS_WAIT */



/*
**  Set up a signal handler.
*/
SetSigs(What, Func)
    int		  What;
    sigret_t	(*Func)();
{
    if (What == S_IGNORE)
	Func = SIG_IGN;
    else if (What == S_RESET)
	Func = SIG_DFL;
#ifdef	SIGINT
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	(void)signal(SIGINT, Func);
#endif	/* SIGINT */
#ifdef	SIGQUIT
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	(void)signal(SIGQUIT, Func);
#endif	/* SIGQUIT */
}


/*
**  Return the process ID.
*/
int
Pid()
{
#ifdef	MSDOS
    return(1);
#else
    static int	 X;

    if (X == 0)
	X = getpid();
    return(X);
#endif	/* MSDOS */
}


#ifndef	USE_SYSTEM
/*
**  Fork off a command.
*/
int
Execute(av)
    char		*av[];
{
    REGISTER int	 i;
    REGISTER int	 j;
    WAITER		 W;

    if ((i = FORK()) == 0) {
	SetSigs(S_RESET, (sigret_t (*)())NULL);
	(void)execvp(av[0], av);
	perror(av[0]);
	_exit(1);
    }

    SetSigs(S_IGNORE, (sigret_t (*)())NULL);
    while ((j = wait((int *)&W)) < 0 && j != i)
	;
    SetSigs(S_RESET, (sigret_t (*)())NULL);
    return(W_VAL(W));
}

#else

/*
**  Cons all the arguments together into a single command line and hand
**  it off to the shell to execute.
*/
int
Execute(av)
    REGISTER char	*av[];
{
    REGISTER char	**v;
    REGISTER char	 *p;
    REGISTER char	 *q;
    REGISTER int	 i;

    /* Get length of command line. */
    for (i = 0, v = av; *v; v++)
	i += strlen(*v) + 1;

    /* Create command line and execute it. */
    p = NEW(char, i);
    for (q = p, v = av; *v; v++) {
	*q++ = ' ';
	q += strlen(strcpy(q, *v));
    }

    return(system(p));
}
#endif	/* USE_SYSTEM */
