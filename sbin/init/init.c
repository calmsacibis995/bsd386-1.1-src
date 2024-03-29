/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * This code is derived from software contributed to Berkeley by
 * Donn Seeley at UUNET Technologies, Inc.
 *
 *	BSDI $Id: init.c,v 1.15 1993/12/21 03:36:51 polk Exp $
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1992 Berkeley Software Design, Inc.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)init.c	6.5 (Berkeley) 7/25/91";
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <db.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef SECURE
#include <pwd.h>
#endif

#include "pathnames.h"

/*
 * Until the mythical util.h arrives...
 */
extern int login_tty __P((int));
extern int logout __P((const char *));
extern void logwtmp __P((const char *, const char *, const char *));

/*
 * Sleep times; used to prevent thrashing.
 */
#define	GETTY_SPACING		 5	/* N secs minimum getty spacing */
#define	GETTY_SLEEP		30	/* sleep N secs after spacing problem */
#define	WINDOW_WAIT		 3	/* wait N secs after starting window */
#define	STALL_TIMEOUT		30	/* wait N secs after warning */
#define	DEATH_WATCH		10	/* wait N secs for procs to die */

void handle __P((sig_t, ...));
void delset __P((sigset_t *, ...));

void stall __P((char *, ...));
void warning __P((char *, ...));
void emergency __P((char *, ...));
void disaster __P((int));

/*
 * We really need a recursive typedef...
 * The following at least guarantees that the return type of (*state_t)()
 * is sufficiently wide to hold a function pointer.
 */
typedef long (*state_func_t) __P((void));
typedef state_func_t (*state_t) __P((void));

state_func_t single_user __P((void));
state_func_t runcom __P((void));
state_func_t read_ttys __P((void));
state_func_t multi_user __P((void));
state_func_t clean_ttys __P((void));
state_func_t catatonia __P((void));
state_func_t death __P((void));

enum { AUTOBOOT, FASTBOOT } runcom_mode = AUTOBOOT;

void transition __P((state_t));
state_t requested_transition = runcom;

void setctty __P((char *));

typedef struct session {
	int	se_index;		/* index of entry in ttys file */
	pid_t	se_process;		/* controlling process */
	time_t	se_started;		/* used to avoid thrashing */
	int	se_flags;		/* status of session */
#define	SE_SHUTDOWN	0x1		/* session won't be restarted */
	char	*se_device;		/* filename of port */
	char	*se_getty;		/* what to run on that port */
	char	**se_getty_argv;	/* pre-parsed argument array */
	char	*se_window;		/* window system (started only once) */
	char	**se_window_argv;	/* pre-parsed argument array */
	struct	session *se_prev;
	struct	session *se_next;
} session_t;

void free_session __P((session_t *));
session_t *new_session __P((session_t *, int, struct ttyent *));
session_t *sessions;

char **construct_argv __P((char *));
void start_window_system __P((session_t *));
void collect_child __P((int));
pid_t start_getty __P((session_t *));
void transition_handler __P((int));
void alrm_handler __P((int));
int clang;


int start_session_db __P((void));
void add_session __P((session_t *));
void del_session __P((session_t *));
session_t *find_session __P((pid_t));
DB *session_db;

/*
 * The mother of all processes.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	struct sigaction sa;
	sigset_t mask;


	/* Dispose of random users. */
	if (getuid() != 0) {
		(void)fprintf(stderr, "init: %s\n", strerror(EPERM));
		exit (1);
	}

	/* System V users like to reexec init. */
	if (getpid() != 1) {
		(void)fprintf(stderr, "init: already running\n");
		exit (1);
	}

	/*
	 * Note that this does NOT open a file...
	 * Does 'init' deserve its own facility number?
	 */
	openlog("init", LOG_CONS|LOG_ODELAY, LOG_AUTH);

	/*
	 * Create an initial session.
	 */
	if (setsid() < 0)
		warning("initial setsid() failed: %m");

	/*
	 * This code assumes that we always get arguments through flags,
	 * never through bits set in some random machine register.
	 */
	while ((c = getopt(argc, argv, "sf")) != -1)
		switch (c) {
		case 's':
			requested_transition = single_user;
			break;
		case 'f':
			runcom_mode = FASTBOOT;
			break;
		default:
			warning("unrecognized flag '-%c'", c);
			break;
		}

	if (optind != argc)
		warning("ignoring excess arguments");

	/*
	 * We catch or block signals rather than ignore them,
	 * so that they get reset on exec.
	 */
	handle(disaster, SIGABRT, SIGFPE, SIGILL, SIGSEGV,
	       SIGBUS, SIGSYS, SIGXCPU, SIGXFSZ, 0);
	handle(transition_handler, SIGHUP, SIGTERM, SIGTSTP, 0);
	handle(alrm_handler, SIGALRM, 0);
	sigfillset(&mask);
	delset(&mask, SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGSYS,
		SIGXCPU, SIGXFSZ, SIGHUP, SIGTERM, SIGTSTP, SIGALRM, 0);
	sigprocmask(SIG_SETMASK, &mask, (sigset_t *) 0);
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	(void) sigaction(SIGTTIN, &sa, (struct sigaction *)0);
	(void) sigaction(SIGTTOU, &sa, (struct sigaction *)0);

	/*
	 * Paranoia.
	 */
	close(0);
	close(1);
	close(2);

	/*
	 * Start the state machine.
	 */
	transition(requested_transition);

	/*
	 * Should never reach here.
	 */
	return 1;
}

/*
 * Associate a function with a signal handler.
 */
void
#ifdef __STDC__
handle(sig_t handler, ...)
#else
handle(va_alist)
	va_dcl
#endif
{
	int sig;
	struct sigaction sa;
	int mask_everything;
	va_list ap;
#ifndef __STDC__
	sig_t handler;

	va_start(ap);
	handler = va_arg(ap, sig_t);
#else
	va_start(ap, handler);
#endif

	sa.sa_handler = handler;
	sigfillset(&mask_everything);

	while (sig = va_arg(ap, int)) {
		sa.sa_mask = mask_everything;
		/* XXX SA_RESTART? */
		sa.sa_flags = sig == SIGCHLD ? SA_NOCLDSTOP : 0;
		sigaction(sig, &sa, (struct sigaction *) 0);
	}
}

/*
 * Delete a set of signals from a mask.
 */
void
#ifdef __STDC__
delset(sigset_t *maskp, ...)
#else
delset(va_alist)
	va_dcl
#endif
{
	int sig;
	va_list ap;
#ifndef __STDC__
	sigset_t *maskp;

	va_start(ap);
	maskp = va_arg(ap, sigset_t *);
#else
	va_start(ap, maskp);
#endif

	while (sig = va_arg(ap, int))
		sigdelset(maskp, sig);
}

/*
 * Log a message and sleep for a while (to give someone an opportunity
 * to read it and to save log or hardcopy output if the problem is chronic).
 * NB: should send a message to the session logger to avoid blocking.
 */
void
#ifdef __STDC__
stall(char *message, ...)
#else
stall(va_alist)
	va_dcl
#endif
{
	pid_t pid;
	va_list ap;
#ifndef __STDC__
	char *message;

	va_start(ap);
	message = va_arg(ap, char *);
#else
	va_start(ap, message);
#endif

	vsyslog(LOG_ALERT, message, ap);
	va_end(ap);
	sleep(STALL_TIMEOUT);
}

/*
 * Like stall(), but doesn't sleep.
 * If cpp had variadic macros, the two functions could be #defines for another.
 * NB: should send a message to the session logger to avoid blocking.
 */
void
#ifdef __STDC__
warning(char *message, ...)
#else
warning(va_alist)
	va_dcl
#endif
{
	va_list ap;
#ifndef __STDC__
	char *message;

	va_start(ap);
	message = va_arg(ap, char *);
#else
	va_start(ap, message);
#endif

	vsyslog(LOG_ALERT, message, ap);
	va_end(ap);
}

/*
 * Log an emergency message.
 * NB: should send a message to the session logger to avoid blocking.
 */
void
#ifdef __STDC__
emergency(char *message, ...)
#else
emergency(va_alist)
	va_dcl
#endif
{
	va_list ap;
#ifndef __STDC__
	char *message;

	va_start(ap);
	message = va_arg(ap, char *);
#else
	va_start(ap, message);
#endif

	vsyslog(LOG_EMERG, message, ap);
	va_end(ap);
}

/*
 * Catch an unexpected signal.
 */
void
disaster(sig)
	int sig;
{
	emergency("fatal signal: %s",
		sig < (unsigned) NSIG ? sys_siglist[sig] : "unknown signal");

	sleep(STALL_TIMEOUT);
	_exit(sig);		/* reboot */
}

/*
 * Change states in the finite state machine.
 * The initial state is passed as an argument.
 */
void
transition(s)
	state_t s;
{
	for (;;)
		s = (state_t) (*s)();
}


/*
 * Close out the accounting files for a login session.
 * NB: should send a message to the session logger to avoid blocking.
 */
void
clear_session_logs(sp)
	session_t *sp;
{
	char *line = sp->se_device + sizeof(_PATH_DEV) - 1;

	if (logout(line))
		logwtmp(line, "", "");
}

/*
 * Start a session and allocate a controlling terminal.
 * Only called by children of init after forking.
 */
void
setctty(name)
	char *name;
{
	int fd;

	(void) revoke(name);
	sleep (2);			/* leave DTR low */
	if ((fd = open(name, O_RDWR)) == -1) {
		stall("can't open %s: %m", name);
		_exit(1);
	}
	if (login_tty(fd) == -1) {
		stall("can't get %s for controlling terminal: %m", name);
		_exit(1);
	}
	if (setlogin("root") == -1) {
		stall("setlogin() failed: %m", name);
		_exit(1);
	}
}

/*
 * Bring the system up single user.
 */
state_func_t
single_user()
{
	pid_t pid, wpid;
	int status;
	sigset_t mask;
	char *shell = _PATH_BSHELL;
	char *argv[2];
#ifdef SECURE
	struct ttyent *typ;
	struct passwd *pp;
	static const char banner[] =
		"Enter root password, or ^D to go multi-user\n";
	char *clear, *password;
#endif

	if ((pid = fork()) == 0) {
		/*
		 * Start the single user session.
		 */
		setctty(_PATH_CONSOLE);

#ifdef SECURE
		/*
		 * Check the root password.
		 * We don't care if the console is 'on' by default;
		 * it's the only tty that can be 'off' and 'secure'.
		 */
		typ = getttynam("console");
		pp = getpwnam("root");
		if (typ && (typ->ty_status & TTY_SECURE) == 0 && pp) {
			write(2, banner, sizeof banner - 1);
			for (;;) {
				clear = getpass("Password:");
				if (clear == 0 || *clear == '\0')
					_exit(0);
				password = crypt(clear, pp->pw_passwd);
				bzero(clear, _PASSWORD_LEN);
				if (strcmp(password, pp->pw_passwd) == 0)
					break;
			}
		}
#if 0
		/*
		 * Make the single-user shell be root's standard shell?
		 */
		if (pp && pp->pw_shell)
			shell = pp->pw_shell;
#endif
		endttyent();
		endpwent();
#endif

		/*
		 * Unblock signals.
		 * We catch all the interesting ones,
		 * and those are reset to SIG_DFL on exec.
		 */
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, (sigset_t *) 0);

		/*
		 * Fire off a shell.
		 * If the default one doesn't work, try the Bourne shell.
		 */
		argv[0] = "-sh";
		argv[1] = 0;
		execv(shell, argv);
		emergency("can't exec %s for single user: %m", shell);
		execv(_PATH_BSHELL, argv);
		emergency("can't exec %s for single user: %m", _PATH_BSHELL);
		sleep(STALL_TIMEOUT);
		_exit(1);
	}

	if (pid == -1) {
		/*
		 * We are seriously hosed.  Do our best.
		 */
		emergency("can't fork single-user shell, trying again");
		while (waitpid(-1, (int *) 0, WNOHANG) > 0)
			;
		return (state_func_t) single_user;
	}

	requested_transition = 0;
	do {
		if ((wpid = waitpid(-1, &status, WUNTRACED)) != -1)
			collect_child(wpid);
		if (wpid == -1) {
			if (errno == EINTR)
				continue;
			warning("wait for single-user shell failed: %m; restarting");
			return (state_func_t) single_user;
		}
		if (wpid == pid && WIFSTOPPED(status)) {
			warning("init: shell stopped, restarting\n");
			kill(pid, SIGCONT);
			wpid = -1;
		}
	} while (wpid != pid && !requested_transition);

	if (requested_transition)
		return (state_func_t) requested_transition;

	if (!WIFEXITED(status)) {
		if (WTERMSIG(status) == SIGKILL) { 
			/* 
			 *  reboot(8) killed shell? 
			 */
			warning("single user shell terminated.");
			sleep(STALL_TIMEOUT);
			_exit(0);
		} else {	
			warning("single user shell terminated, restarting");
			return (state_func_t) single_user;
		}
	}

	runcom_mode = FASTBOOT;
	return (state_func_t) runcom;
}

/*
 * Run the system startup script.
 */
state_func_t
runcom()
{
	pid_t pid, wpid;
	int status;
	char *argv[4];
	struct sigaction sa;

	if ((pid = fork()) == 0) {
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_IGN;
		(void) sigaction(SIGTSTP, &sa, (struct sigaction *)0);
		(void) sigaction(SIGHUP, &sa, (struct sigaction *)0);

		setctty(_PATH_CONSOLE);

		argv[0] = "sh";
		argv[1] = _PATH_RUNCOM;
		argv[2] = runcom_mode == AUTOBOOT ? "autoboot" : 0;
		argv[3] = 0;

		sigprocmask(SIG_SETMASK, &sa.sa_mask, (sigset_t *) 0);

		execv(_PATH_BSHELL, argv);
		stall("can't exec %s for %s: %m", _PATH_BSHELL, _PATH_RUNCOM);
		_exit(1);	/* force single user mode */
	}

	if (pid == -1) {
		emergency("can't fork for %s on %s: %m",
			_PATH_BSHELL, _PATH_RUNCOM);
		while (waitpid(-1, (int *) 0, WNOHANG) > 0)
			;
		sleep(STALL_TIMEOUT);
		return (state_func_t) single_user;
	}

	/*
	 * Copied from single_user().  This is a bit paranoid.
	 */
	do {
		if ((wpid = waitpid(-1, &status, WUNTRACED)) != -1)
			collect_child(wpid);
		if (wpid == -1) {
			if (errno == EINTR)
				continue;
			warning("wait for %s on %s failed: %m; going to single user mode",
				_PATH_BSHELL, _PATH_RUNCOM);
			return (state_func_t) single_user;
		}
		if (wpid == pid && WIFSTOPPED(status)) {
			warning("init: %s on %s stopped, restarting\n",
				_PATH_BSHELL, _PATH_RUNCOM);
			kill(pid, SIGCONT);
			wpid = -1;
		}
	} while (wpid != pid);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM &&
	    requested_transition == catatonia) {
		/* /etc/rc executed /sbin/reboot; wait for the end quietly */
		sigset_t s;

		sigfillset(&s);
		for (;;)
			sigsuspend(&s);
	}

	if (!WIFEXITED(status)) {
		warning("%s on %s terminated abnormally, going to single user mode",
			_PATH_BSHELL, _PATH_RUNCOM);
		return (state_func_t) single_user;
	}

	if (WEXITSTATUS(status))
		return (state_func_t) single_user;

	runcom_mode = AUTOBOOT;		/* the default */
	/* NB: should send a message to the session logger to avoid blocking. */
	logwtmp("~", "reboot", "");
	return (state_func_t) read_ttys;
}

/*
 * Open the session database.
 *
 * NB: We could pass in the size here; is it necessary?
 */
int
start_session_db()
{
	if (session_db && (*session_db->close)(session_db))
		emergency("session database close: %s", strerror(errno));
	if ((session_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == 0) {
		emergency("session database open: %s", strerror(errno));
		return (1);
	}
	return (0);
		
}

/*
 * Add a new login session.
 */
void
add_session(sp)
	session_t *sp;
{
	DBT key;
	DBT data;

	key.data = &sp->se_process;
	key.size = sizeof sp->se_process;
	data.data = &sp;
	data.size = sizeof sp;

	if ((*session_db->put)(session_db, &key, &data, 0))
		emergency("insert %d: %s", sp->se_process, strerror(errno));
}

/*
 * Delete an old login session.
 */
void
del_session(sp)
	session_t *sp;
{
	DBT key;

	key.data = &sp->se_process;
	key.size = sizeof sp->se_process;

	if ((*session_db->del)(session_db, &key, 0))
		emergency("delete %d: %s", sp->se_process, strerror(errno));
}

/*
 * Look up a login session by pid.
 */
session_t *
#ifdef __STDC__
find_session(pid_t pid)
#else
find_session(pid)
	pid_t pid;
#endif
{
	DBT key;
	DBT data;

	key.data = &pid;
	key.size = sizeof pid;
	if ((*session_db->get)(session_db, &key, &data, 0) != 0)
		return 0;
	return *(session_t **)data.data;
}

/*
 * Construct an argument vector from a command line.
 */
char **
construct_argv(command)
	char *command;
{
	register int argc = 0;
	register char **argv = (char **) malloc(((strlen(command) + 1) / 2 + 1)
						* sizeof (char *));
	static const char separators[] = " \t";

	if ((argv[argc++] = strtok(command, separators)) == 0)
		return 0;
	while (argv[argc++] = strtok((char *) 0, separators))
		;
	return argv;
}

/*
 * Deallocate a session descriptor.
 */
void
free_session(sp)
	register session_t *sp;
{
	free(sp->se_device);
	free(sp->se_getty);
	free(sp->se_getty_argv);
	if (sp->se_window) {
		free(sp->se_window);
		free(sp->se_window_argv);
	}
	free(sp);
}

/*
 * Allocate a new session descriptor.
 */
session_t *
new_session(sprev, session_index, typ)
	session_t *sprev;
	int session_index;
	register struct ttyent *typ;
{
	register session_t *sp;

	if ((typ->ty_status & TTY_ON) == 0 ||
	    typ->ty_name == 0 ||
	    typ->ty_getty == 0)
		return 0;

	sp = (session_t *) malloc(sizeof (session_t));

	sp->se_index = session_index;
	sp->se_process = 0;
	sp->se_started = 0;
	sp->se_flags = 0;
	sp->se_window = 0;

	sp->se_device = malloc(sizeof(_PATH_DEV) + strlen(typ->ty_name));
	(void) sprintf(sp->se_device, "%s%s", _PATH_DEV, typ->ty_name);

	sp->se_getty = malloc(strlen(typ->ty_getty) + strlen(typ->ty_name) + 2);
	(void) sprintf(sp->se_getty, "%s %s", typ->ty_getty, typ->ty_name);
	sp->se_getty_argv = construct_argv(sp->se_getty);
	if (sp->se_getty_argv == 0) {
		warning("can't parse getty for port %s",
			sp->se_device);
		free_session(sp);
		return 0;
	}
	if (typ->ty_window) {
		sp->se_window = strdup(typ->ty_window);
		sp->se_window_argv = construct_argv(sp->se_window);
		if (sp->se_window_argv == 0) {
			warning("can't parse window for port %s",
				sp->se_device);
			free_session(sp);
			return 0;
		}
	}

	if (sprev == 0) {
		if (sp->se_next = sessions)
			sp->se_next->se_prev = sp;
		sessions = sp;
		sp->se_prev = 0;
	} else {
		if (sp->se_next = sprev->se_next)
			sp->se_next->se_prev = sp;
		sprev->se_next = sp;
		sp->se_prev = sprev;
	}

	return sp;
}

/*
 * Walk the list of ttys and create sessions for each active line.
 */
state_func_t
read_ttys()
{
	int session_index = 0;
	register session_t *sp, *snext;
	register struct ttyent *typ;

	/*
	 * Destroy any previous session state.
	 * There shouldn't be any, but just in case...
	 */
	for (sp = sessions; sp; sp = snext) {
		if (sp->se_process)
			clear_session_logs(sp);
		snext = sp->se_next;
		free_session(sp);
	}
	sessions = 0;
	if (start_session_db())
		return (state_func_t) single_user;

	/*
	 * Allocate a session entry for each active port.
	 * Note that sp starts at 0.
	 */
	while (typ = getttyent())
		if (snext = new_session(sp, ++session_index, typ))
			sp = snext;

	endttyent();

	return (state_func_t) multi_user;
}

/*
 * Start a window system running.
 */
void
start_window_system(sp)
	session_t *sp;
{
	pid_t pid;
	sigset_t mask;

	if ((pid = fork()) == -1) {
		emergency("can't fork for window system on port %s: %m",
			sp->se_device);
		/* hope that getty fails and we can try again */
		return;
	}

	if (pid)
		return;

	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, (sigset_t *) 0);

	if (setsid() < 0)
		emergency("setsid failed (window) %m");

	execv(sp->se_window_argv[0], sp->se_window_argv);
	stall("can't exec window system '%s' for port %s: %m",
		sp->se_window_argv[0], sp->se_device);
	_exit(1);
}

/*
 * Start a login session running.
 */
pid_t
start_getty(sp)
	session_t *sp;
{
	pid_t pid;
	sigset_t mask;
	time_t current_time = time((time_t *) 0);

	/*
	 * fork(), not vfork() -- we can't afford to block.
	 */
	if ((pid = fork()) == -1) {
		emergency("can't fork for getty on port %s: %m", sp->se_device);
		return -1;
	}

	if (pid)
		return pid;

	if (current_time > sp->se_started &&
	    current_time - sp->se_started < GETTY_SPACING) {
		warning("getty repeating too quickly on port %s, sleeping",
		        sp->se_device);
		sleep((unsigned) GETTY_SLEEP);
	}

	if (sp->se_window) {
		start_window_system(sp);
		sleep(WINDOW_WAIT);
	}

	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, (sigset_t *) 0);

	execv(sp->se_getty_argv[0], sp->se_getty_argv);
	stall("can't exec getty '%s' for port %s: %m",
		sp->se_getty_argv[0], sp->se_device);
	_exit(1);
}

/*
 * Remove sp from the linked list and free its resources.
 */
void
remove_session(sp)
	session_t *sp;
{
	session_t *sprev = sp->se_prev;
	session_t *snext = sp->se_next;

	if (sprev == 0) {
		sessions = snext;
		if (snext)
			snext->se_prev = 0;
	} else if (snext == 0)
		sprev->se_next = 0;
	else {
		sprev->se_next = snext;
		snext->se_prev = sprev;
	}
	free_session(sp);
}

/*
 * Collect exit status for a child.
 * If an exiting login, start a new login running.
 */
void
collect_child(pid)
	pid_t pid;
{
	register session_t *sp;

	if (! sessions)
		return;

	if (! (sp = find_session(pid)))
		return;

	clear_session_logs(sp);
	del_session(sp);
	sp->se_process = 0;

	if (sp->se_flags & SE_SHUTDOWN) {
		remove_session(sp);
		return;
	}

	if ((pid = start_getty(sp)) == -1) {
		/* serious trouble */
		requested_transition = clean_ttys;
		return;
	}

	sp->se_process = pid;
	sp->se_started = time((time_t *) 0);
	add_session(sp);
}

/*
 * Catch a signal and request a state transition.
 */
void
transition_handler(sig)
	int sig;
{

	switch (sig) {
	case SIGHUP:
		requested_transition = clean_ttys;
		break;
	case SIGTERM:
		requested_transition = death;
		break;
	case SIGTSTP:
		requested_transition = catatonia;
		break;
	default:
		requested_transition = 0;
		break;
	}
}

/*
 * Take the system multiuser.
 */
state_func_t
multi_user()
{
	pid_t pid;
	register session_t *sp;

	requested_transition = 0;

	for (sp = sessions; sp; sp = sp->se_next) {
		if (sp->se_process)
			continue;
		if ((pid = start_getty(sp)) == -1) {
			/* serious trouble */
			requested_transition = clean_ttys;
			break;
		}
		sp->se_process = pid;
		sp->se_started = time((time_t *) 0);
		add_session(sp);
	}

	while (!requested_transition)
		if ((pid = waitpid(-1, (int *) 0, 0)) != -1)
			collect_child(pid);

	return (state_func_t) requested_transition;
}

/*
 * This is an n-squared algorithm.  We hope it isn't run often...
 */
state_func_t
clean_ttys()
{
	session_t *sp, *sprev, *snext;
	register struct ttyent *typ;
	register int session_index = 0;
	register int devlen;
	pid_t process;
	time_t started;

	if (! sessions)
		return (state_func_t) multi_user;

	devlen = sizeof(_PATH_DEV) - 1;
	while (typ = getttyent()) {
		++session_index;

		for (sprev = 0, sp = sessions; sp; sprev = sp, sp = sp->se_next)
			if (strcmp(typ->ty_name, sp->se_device + devlen) == 0)
				break;

		process = 0;
		started = 0;

		if (sp) {
			if (sp->se_index != session_index) {
				warning("port %s changed utmp index from %d to %d",
				       sp->se_device, sp->se_index,
				       session_index);
				sp->se_index = session_index;
			}
			if ((typ->ty_status & TTY_ON) == 0) {
				sp->se_flags |= SE_SHUTDOWN;
				kill(sp->se_process, SIGHUP);
				continue;
			}

			/* destroy the old session record and start anew */
			process = sp->se_process;
			started = sp->se_started;
			del_session(sp);
			remove_session(sp);
		}

		if (sp = new_session(sprev, session_index, typ)) {
			sp->se_process = process;
			sp->se_started = started;
			if (process)
				add_session(sp);
		}
	}

	endttyent();

	return (state_func_t) multi_user;
}

/*
 * Block further logins.
 */
state_func_t
catatonia()
{
	register session_t *sp;

	for (sp = sessions; sp; sp = sp->se_next)
		sp->se_flags |= SE_SHUTDOWN;

	return (state_func_t) multi_user;
}

/*
 * Note SIGALRM.
 */
void
alrm_handler(sig)
	int sig;
{
	clang = 1;
}

/*
 * Bring the system down to single user.
 */
state_func_t
death()
{
	register session_t *sp;
	register int i;
	pid_t pid;
	static const int death_sigs[3] = { SIGHUP, SIGTERM, SIGKILL };

	for (sp = sessions; sp; sp = sp->se_next)
		sp->se_flags |= SE_SHUTDOWN;

	/* NB: should send a message to the session logger to avoid blocking. */
	logwtmp("~", "shutdown", "");

	for (i = 0; i < 3; ++i) {
		if (kill(-1, death_sigs[i]) == -1 && errno == ESRCH)
			return (state_func_t) single_user;

		clang = 0;
		alarm(DEATH_WATCH);
		do
			if ((pid = waitpid(-1, (int *)0, 0)) != -1)
				collect_child(pid);
		while (clang == 0 && errno != ECHILD);

		if (errno == ECHILD)
			return (state_func_t) single_user;
	}

	warning("some processes wouldn't die");

	return (state_func_t) single_user;
}
