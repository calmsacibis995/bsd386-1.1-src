/*	BSDI $Id: ifconfig.c,v 1.2 1994/01/28 00:30:30 sanders Exp $	*/

#include "ppp.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/pppioctl.h>

/*
 * Call ifconfig and make sure that interface is up
 */
void
ifconfig(ifname, arg)
	char *ifname;
	char *arg;
{
	char *ap[100], **app;
	char buf[512], *bp;
	int status;
	struct ifreq ifr;

	if (arg != 0) {
		/*
		 * Compose the argument list
		 */
		bp = buf;
		app = ap;
		*app++ = "ifconfig";
		*app++ = ifname;
		while (*arg) {
			while (*arg == ' ')
				arg++;
			if (*arg == 0)
				break;
			*app++ = bp;
			while (*arg && *arg != ' ')
				*bp++ = *arg++;
			*bp++ = 0;
		}
		*app = 0;

		/*
		 * Execute ifconfig
		 */
		if (fork() == 0) {
			/* the new process */
			execv(_PATH_IFCONFIG, ap);
			log("exec %s failed\n", _PATH_IFCONFIG);
			fprintf(stderr, "ppp: cannot execute %s\n", _PATH_IFCONFIG);
			exit(1);
		}
		(void) wait(&status);

		if (status != 0) {
			fprintf(stderr, "ppp: ifconfig failed on %s for %s\n",
					ifname, sysname);
			log("ifconfig failed on %s for %s\n", ifname, sysname);
			exit(1);
		}
	}

	/*
	 * Read flags and if IFF_UP is not set, raise it
	 */
	strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
	if (ioctl(Socket, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		ifperror("ioctl (SIOCGIFFLAGS)", ifname);

	if ((ifr.ifr_flags & IFF_UP) == 0) {
		strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
		ifr.ifr_flags |= IFF_UP;
		if (ioctl(Socket, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
			ifperror("ioctl (SIOCSIFFLAGS)", ifname);
	}
}

/*
 * Configure PPP parameters
 */
void
pppconfig(ifname)
{
	struct ifreq ifr, ifr1;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	/*
	 * Read PPP defaults
	 */
	strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
	if (ioctl(Socket, PPPIOCGPAR, (caddr_t)&ifr) < 0)
		ifperror("ioctl (PPPIOGPAR)", ifname);
	ifr1 = ifr;

	/*
	 * Set parameters
	 */
	if (CM != 0)
		pppsetcmap(pio, CM);
	if (MC != -1)
		pio->ppp_maxconf = MC;
	if (MT != -1)
		pio->ppp_maxterm = MT;
	if (MR != -1) {
		if (MR < 128 || MR > 1500) {
			fprintf(stderr, "ppp: bad MRU %d for %s\n",
					MR, sysname);
			exit(1);
		}
		pio->ppp_mru = MR;
	}
	if (ID != -1)
		pio->ppp_idletime = ID;
	if (TO > 0)
		pio->ppp_timeout = TO;
	if (PF != 0)
		pppsetflags(pio, PF);

	/*
	 * Load parameters
	 */
	if (!bcmp(&ifr, &ifr1, sizeof ifr))
		return;
	strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
	if (ioctl(Socket, PPPIOCSPAR, (caddr_t)&ifr) < 0)
		ifperror("ioctl (PPPIOSPAR)", ifname);
}

/*
 * Parse PPP control characters map (cm=)
 */
pppsetcmap(pio, arg)
	char *arg;
	struct ppp_ioctl *pio;
{
	u_long map = 0;
	char c;

	if (*arg == '0') {
		/* Hexadecimal? */
		if (*++arg = 'x' || *arg == 'X') {
			while (c = *++arg) {
				map <<= 4;
				if ('0' <= c && c <= '9')
					map |= c - '0';
				else if ('A' <= c && c <= 'F')
					map |= c - 'A' + 10;
				else if ('a' <= c && c <= 'f')
					map |= c - 'a' + 10;
				else
					goto err;
			}
		} else {
			/* Octal? */
			while (c = *arg++) {
				map <<= 3;
				if ('0' <= c && c <= '7')
					map |= c - '0';
				else
					goto err;
			}
		}
	} else {
		/* List of characters? */
		while (c = *arg++)
			map |= 1 << (c & 037);
	}
	pio->ppp_cmap = map;
	return;
err:
	fprintf(stderr, "ppp: bad character map for %s\n", sysname);
	exit(1);
}

/*
 * Parse PPP flags (pf=)
 */
pppsetflags(pio, arg)
	struct ppp_ioctl *pio;
	char *arg;
{
	char sc;
	char *p;
	int neg;
	static struct {
		char    *n;
		int     v;
	} *t, tab[] = {
		"pfc",  PPP_PFC,
		"acfc", PPP_ACFC,
		"tcpc", PPP_TCPC,
		"ftel", PPP_FTEL,
		0,      0,
	};

	while (*arg) {
		while (*arg == ',' || *arg == ' ')
			arg++;
		if (*arg == 0)
			break;
		p = arg;
		while (*arg && *arg != ',' && *arg != ' ')
			arg++;
		sc = *arg;
		*arg = 0;

		neg = 0;
		if (*p == '-')
			p++, neg++;
		for (t = tab; t->n && strcmp(t->n, p); t++)
			;
		if (t->n == 0) {
			fprintf(stderr, "ppp: bad flag %s for %s\n",
				p, sysname);
			exit(1);
		}
		*arg = sc;
		if (neg)
			pio->ppp_flags &= ~(t->v);
		else
			pio->ppp_flags |= t->v;
	}
}

/*
 * Wait for a dropped packet on a ppp interface
 */
void
waitdrop(ifname)
	char *ifname;
{
	struct ifreq ifr;

	if (debug)
		printf("Wait for a packet on %s...", ifname);
	strncpy(ifr.ifr_name, ifname, sizeof ifr.ifr_name);
	if (ioctl(Socket, PPPIOCWAIT, (caddr_t)&ifr) < 0)
		ifperror("ioctl (PPPIOCWAIT)", ifname);
	if (debug)
		printf(" Got it.\n");
}

/*
 * Print interface-related error msg
 */
ifperror(msg, ifname)
	char *msg;
	char *ifname;
{
	extern int errno;

	fprintf(stderr, "ppp: %s on %s: ", msg, ifname);
	switch (errno) {

	case ENXIO:
		log("%s on %s: no such interface\n", msg, ifname);
		fprintf(stderr, "no such interface\n");
		break;

	case EPERM:
		log("%s on %s: permission denied\n", msg, ifname);
		fprintf(stderr, "permission denied\n");
		break;

	default:
		log("%s on %s: errno=%d\n", msg, ifname, errno);
		perror("");
	}
	exit(1);
}
