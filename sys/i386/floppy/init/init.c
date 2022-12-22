/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * This code is derived from software contributed to Berkeley by
 * Donn Seeley at UUNET Technologies, Inc.
 *
 *	BSDI $Id: init.c,v 1.2 1993/03/21 18:27:04 polk Exp $
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <paths.h>

extern int login_tty __P((int));

int
main(argc, argv)
	int argc;
	char **argv;
{
	if (setsid() < 0)
		_exit(1);

	for (;;)
		dosingle();
}

dosingle()
{
	pid_t pid, wpid;
	int status;
	char *shell = _PATH_BSHELL;
	char *argv[2];
	int fd;

	if ((pid = fork()) == 0) {
		/*
		 * Start the single user session.
		 */
		if ((fd = open(_PATH_CONSOLE, O_RDWR)) == -1) {
			_exit(1);
		}
		if (login_tty(fd) == -1) {
			_exit(1);
		}

		/*
		 * Fire off a shell.
		 * If the default one doesn't work, try the Bourne shell.
		 */
		argv[0] = "-sh";
		argv[1] = 0;
		execv(shell, argv);
		_exit(1);
	}

	if (pid == -1) {
		/*
		 * We are seriously hosed.  Do our best.
		 */
		while (waitpid(-1, (int *) 0, WNOHANG) > 0)
			;
		return;
	}

	while ((wpid = waitpid(-1, &status, WUNTRACED)) != -1 && wpid != pid)
		;
	if (wpid == -1 && errno == EINTR ||
	    !WIFEXITED(status) && WTERMSIG(status) == SIGKILL)
		/* someone probably ran 'reboot' */
		for (;;)
			pause();
}
