/*	BSDI $Id: ppp.c,v 1.2 1994/01/28 00:30:32 sanders Exp $	*/

/*
 * Establish a PPP connection
 *
 * ppp  - attach dial-in connection if called as a login shell
 * ppp -d [systemid]
 *      - dial-out daemon mode (calls on dropped packets)
 * ppp systemid
 *      - call the specified system explicitly
 *
 * Additional parameters:
 *      -s sysfile      - use sysfile instead of /etc/ppp.sys
 *      -x              - turn on debugging
 *
 * /etc/ppp.sys contains termcap-style records:
 *      at      (str) Auto call unit type.
 *      br      (num) Baud rate.
 *      cm      (str) Map of special characters (as in pppconfig)
 *      cu      (str) Call unit if making a phone call.
 *      di      (bool) Dial-in allowed.
 *      do      (bool) Automatic dial-out allowed.
 *      du      (bool) This is a dial-up line
 *      dv      (str) Device(s) to open to establish a
 *                    connection.
 *      e0-e9   (str) String to wait for on nth step of logging in
 *                    (after sending s0-s9)
 *      f0-f9   (str) Strings to send if failed to get an expected
 *                    string (e0-e9)
 *      id      (num) Disconnect on idle (n seconds)
 *      if      (str) Space-separated list of ifconfig parameters.
 *      in      (num) Interface number.
 *      mc      (num) Max PPP config retries.
 *      mr      (num) PPP MRU.
 *      mt      (num) Max PPP terminate retries.
 *      pf      (str) Comma-separated list of PPP flags (as in pppconfig)
 *      pn      (str) Telephone number(s) for this host.
 *      s0-s9   (str) String to send on nth step of logging in.
 *      t0-t9   (num) Timeout in seconds for e0-e9
 *      tc      (str) Cap. list continuation.
 *      to      (num) PPP retrty timeout (1/10 sec).
 */

#include "ppp.h"
#include <sys/param.h>
#include <sys/socket.h>

#include <net/pppioctl.h>

int debug = 0;
int daemon = 0;
char *sysfile = _PATH_SYS;
char *sysname;

char ifname[30];

int Socket;

static int busymsg = 0;

main(ac, av)
	int ac;
	char **av;
{
	extern int optind;
	extern char *optarg;
	extern char *getlogin();
	int dialin = 0;
	char *linkname;
	int i, timo;
	int ld, oldld;
	char *reason;

	for (;;) {
		switch (getopt(ac, av, "dxs:")) {
		default:
		usage:
			fprintf(stderr, "usage: ppp [-s sysfile] [-d] [systemname]\n");
			exit(1);

		case 'd':
			daemon++;
			continue;

		case 'x':
			debug++;
			continue;

		case 's':
			sysfile = optarg;
			continue;

		case EOF:
			break;
		}
		break;
	}

	if (optind < ac - 1)
		goto usage;

	/* Check permissions */
	if ((daemon || debug) && getuid() != 0) {
		fprintf(stderr, "ppp: you're not superuser\n");
		exit(1);
	}

	setbuf(stdout, NULL);

	/* Get the system name */
	if (ac > optind)
		sysname = av[optind];
	else if (daemon)
		sysname = 0;
	else {
		sysname = getlogin();
		dialin++;
		if (sysname == 0) {
			fprintf(stderr, "ppp: can't get the login name\n");
			exit(1);
		}
	}

	/*
	 * Daemon mode... look out for the dropped packets!
	 */
	if (daemon && sysname == 0) {
		/*
		 * Scan the system file for all automatic dial-outs
		 * and do forks for every system name.
		 */
		fprintf(stderr, "daemon w/o sysname IS NOT IMPLEMENTED (yet)\n");
		exit(1);
	}

	getsyscap(sysname, sysfile);

	/*
	 * Get a socket for ifreqs
	 */
	Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (Socket < 0)
		ifperror("socket", "AF_INET");

	/*
	 * Allocate an interface by looking for one which is not UP
	 * or RUNNING if interface # is not supplied.
	 */
	if (IN == -1) {
		if (IF == 0) {
			fprintf(stderr, "ppp: %s \"in\" or \"if\" should be specified\n", sysname);
			exit(1);
		}

		fprintf(stderr, "dynamic interface allocation IS NOT IMPLEMENTED (yet)\n");
		exit(1);
	}
	snprintf(ifname, sizeof ifname, "ppp%d", IN);

	if (dialin && DI == 0) {
		fprintf(stderr, "ppp: dialing is not allowed for %s\n", sysname);
		exit(1);
	}

	/*
	 * Load interface and PPP parameters
	 */
	ifconfig(ifname, IF);
	pppconfig(ifname);

	/* Dial-in? Just set the line discipline and wait */
	if (dialin) {
		log("dialin: %s\n", sysname);
		FD = 0;

		/* Set working line parameters */
		set_lparms(FD);
		goto start_protocol;
	}

	/*
	 * Wait for a dropped packet at the interface
	 */
daemon_loop:
	if (daemon) {
		if (DO == 0) {
			fprintf(stderr, "ppp: daemon mode is not allowed for %s\n", sysname);
			exit(1);
		}
		waitdrop(ifname);
	}

	/*
	 * Hunt for an available device
	 */
	if ((linkname = hunt()) == 0) {
		if (busymsg++ == 0)
			log("%s: all ports busy\n", sysname);
		if (daemon) {
			if (debug)
				printf("All ports busy...\n");
			sleep(60);
			goto daemon_loop;
		}
		fprintf(stderr, "ppp: all ports busy\n");
		exit(1);
	}
	busymsg = 0;
	if (debug)
		printf("Dialing on %s...\n", linkname);

	/*
	 * Dial out
	 */
	reason = Connect(linkname);
	if (reason) {
		close(FD);
		(void) uu_unlock(uucplock);
		log("%s: %s (%s)\n", sysname, reason, linkname);
		if (daemon)
			goto daemon_loop;
		fprintf(stderr, "ppp: %s (%s)\n", reason, linkname);
		exit(1);
	}
	set_lparms(FD);

	/*
	 * Do all the log sequence
	 */
	if (debug)
		printf("\nLog in...\n");
	for (i = 0; i < 10; i++) {
		if (S[i] != 0) {
			if (debug)
				printf("Send: <%s>\n", printable(S[i]));
			write(FD, S[i], strlen(S[i]));
		}
		timo = T[i];
		if (timo == -1) {
			if (E[i] == 0)
				continue;
			timo = 15;
		}
		if (E[i] == 0)
			sleep(timo);
		else if (expect(FD, E[i], timo)) {
			if (F[i] != 0) {
				if (debug)
					printf("Send F: <%s>\n", printable(F[i]));
				write(FD, F[i], strlen(F[i]));
				if (!expect(FD, E[i], timo))
					continue;
			}
			close(FD);
			(void) uu_unlock(uucplock);
			log("%s: login failure (%s, expected %s)\n",
				sysname, linkname, printable(E[i]));
			if (daemon)
				goto daemon_loop;
			fprintf(stderr, "ppp: login failed (expected %s)\n",
					printable(E[i]));
			exit(1);
		}
	}

	/*
	 * Get old line discipline
	 */
	oldld = 0;      /* default... */
	(void) ioctl(FD, TIOCGETD, &oldld);

	if (debug)
		printf("Set PPP line disc.\n");

	/*
	 * Set PPP line discipline
	 */
start_protocol:
	ld = PPPDISC;
	if (ioctl(FD, TIOCSETD, &ld) < 0) {
		(void) uu_unlock(uucplock);
		fprintf(stderr, "ioctl(TIOCSETD): ");
		perror(linkname);
		exit(1);
	}

	if (ioctl(FD, PPPIOCSUNIT, &IN) < 0) {
		switch (errno) {
		case ENXIO:
			fprintf(stderr, "ppp: no interface\n");
			break;
		case EBUSY:
			fprintf(stderr, "ppp: interface is busy\n");
			break;
		default:
			fprintf(stderr, "ioctl(PPPIOCSUNIT): ");
			perror(linkname);
			break;
		}
		(void) uu_unlock(uucplock);
		exit(1);
	}
	log("%s: connected\n", sysname);

	/*
	 * End of session
	 */
	if (debug)
		printf("End PPP session...\n");
	(void) ioctl(FD, PPPIOCWEOS);
	if (debug)
		printf("Restore & close line\n");
	(void) ioctl(FD, TIOCSETD, &oldld);
	close(FD);      /* hang up! */
	(void) uu_unlock(uucplock);
	log("%s: end of session\n", sysname);
	if (daemon)
		goto daemon_loop;
	exit(0);
}

static expect_interrupted;

/* Timeout signal handler */
static void
expalarm()
{
	expect_interrupted = 1;
}


/*
 * Look up for an expected string or until the timeout is expired
 * Returns 0 if successfull.
 */
int
expect(fd, str, timo)
	int fd;
	char *str;
	int timo;
{
	struct sigaction act, oact;
	char buf[128];
	char nchars, expchars;
	char c;

	if (debug)
		printf("Expect <%s> (%d sec)\nGot: <", printable(str), timo);
	nchars = 0;
	expchars = strlen(str);
	expect_interrupted = 0;
	act.sa_handler = expalarm;
	act.sa_mask = 0;
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oact);
	alarm(timo);

	do {
		if (read(fd, &c, 1) != 1 || expect_interrupted) {
			alarm(0);
			sigaction(SIGALRM, &oact, 0);
			if (debug)
				printf("> FAILED\n");
			return (1);
		}
		c &= 0177;
		if (debug) {
			if (c < 040)
				printf("^%c", c | 0100);
			else if (c == 0177)
				printf("^?");
			else
				printf("%c", c);
		}
		buf[nchars++] = c;
		if (nchars > expchars)
			bcopy(buf+1, buf, --nchars);
	} while (nchars < expchars || bcmp(buf, str, expchars));
	alarm(0);
	sigaction(SIGALRM, &oact, 0);
	if (debug)
		printf("> OK\n");
	return (0);
}

/*
 * Make string suitable to print out
 */
char *
printable(str)
	char *str;
{
	static char buf[128];
	char *p = buf;
	register c;

	while (c = *str++) {
		c &= 0377;
		if (c < 040) {
			*p++ = '^';
			*p++ = c | 0100;
		} else if (c == 0177) {
			*p++ = '^';
			*p++ = '?';
		} else if (c & 0200) {
			*p++ = '\\';
			*p++ = '0' + (c >> 6);
			*p++ = '0' + ((c >> 3) & 07);
			*p++ = '0' + (c & 07);
		} else
			*p++ = c;
	}
	*p = 0;
	return (buf);
}
