/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "$Id: pppconfig.c,v 1.2 1994/01/28 00:11:26 sanders Exp $";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

/*
 * Copyright (c) 1983 The Regents of the University of California.
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

/* The skeleton code literally repeats ifconfig */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <net/pppioctl.h>

struct  ifreq     ifr, ifr1;

struct ppp_ioctl *pio  = ifr_pppioctl(&ifr);
struct ppp_ioctl *pio1 = ifr_pppioctl(&ifr1);

int chaged;
char	name[30];

int setflags(), setcmap(), mru(), idletime(), maxconf(), maxterm(), timeout();

#define	NEXTARG		0xffffff
#define ZERO            0xfffffe

struct	cmd {
	char	*c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	int	(*c_func)();
} cmds[] = {
	/* cmap */
	{ "cmap",       NEXTARG,        setcmap },

	/* mru */
	{ "mru",        NEXTARG,        mru },

	/* idletime */
	{ "idletime",   NEXTARG,        idletime },
	{ "-idletime",  ZERO,           idletime },

	/* flags */
	{ "pfc",        PPP_PFC,        setflags },
	{ "-pfc",       -PPP_PFC,       setflags },
	{ "acfc",       PPP_ACFC,       setflags },
	{ "-acfc",      -PPP_ACFC,      setflags },
	{ "tcpc",       PPP_TCPC,       setflags },
	{ "-tcpc",      -PPP_TCPC,      setflags },
	{ "ftel",       PPP_FTEL,       setflags },
	{ "-ftel",      -PPP_FTEL,      setflags },
	{ "trace",      PPP_TRACE,      setflags },
	{ "-trace",     -PPP_TRACE,     setflags },

	/* maxconf */
	{ "maxconf",    NEXTARG,        maxconf },
	{ "-maxconf",   ZERO,           maxconf },

	/* maxterm */
	{ "maxterm",    NEXTARG,        maxterm },

	/* "timeout" */
	{ "timeout",    NEXTARG,        timeout },

	{ 0,            0,              0 }
};

main(argc, argv)
	int argc;
	char *argv[];
{
	int s;

	if (argc < 2) {
		fprintf(stderr, "usage: pppconfig interface\n%s\n%s\n%s\n",
			"\t[ cmap hex-map ] [ mru n ] [ idletime secs | -idletime ]",
			"\t[ pfc | -pfc ] [ acfc | -acfc ] [ tcpc | -tcpc ] [ ftel | -ftel ]",
			"\t[ maxconf n | -maxconf ] [ maxterm n ] [ timeout 1/10secs ]");
		exit(1);
	}
	argc--, argv++;
	strncpy(name, *argv, sizeof(name));
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	argc--, argv++;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		exit(1);
	}
	if (ioctl(s, PPPIOCGPAR, (caddr_t)&ifr) < 0) {
		Perror("ioctl (PPPIOCGPAR)");
		exit(1);
	}

	if (argc == 0) {
		strncpy(ifr1.ifr_name, name, sizeof ifr1.ifr_name);
		if (ioctl(s, PPPIOCNPAR, (caddr_t)&ifr1) < 0) {
			Perror("ioctl (PPPIOCGPAR)");
			exit(1);
		}

		printf("initial:\tcmap=0x%x, mru=%d, %spfc, %sacfc, %stcpc\n",
			pio->ppp_cmap, pio->ppp_mru,
			(pio->ppp_flags & PPP_PFC) ? "" : "-",
			(pio->ppp_flags & PPP_ACFC) ? "" : "-",
			(pio->ppp_flags & PPP_TCPC) ? "" : "-");
		printf("negotiated:\tcmap=0x%x, mru=%d, %spfc, %sacfc, %stcpc\n",
			pio1->ppp_cmap, pio1->ppp_mru,
			(pio1->ppp_flags & PPP_PFC) ? "" : "-",
			(pio1->ppp_flags & PPP_ACFC) ? "" : "-",
			(pio1->ppp_flags & PPP_TCPC) ? "" : "-");
		printf("non-negotiable:\t");
		if (pio->ppp_flags & PPP_FTEL)
			printf("ftel,");
		else
			printf("-ftel,");
		if (pio->ppp_flags & PPP_TRACE)
			printf(" trace,");
		else
			printf(" -trace,");
		if (pio->ppp_idletime == 0)
			printf(" -idletime,");
		else
			printf(" idletime=%d,", pio->ppp_idletime);
		printf("\n\t\t");
		if (pio->ppp_maxconf == 0)
			printf("-maxconf,");
		else
			printf("maxconf=%d,", pio->ppp_maxconf);
		printf(" maxterm=%d, timeout=%d\n",
		       pio->ppp_maxterm, pio->ppp_timeout);
		exit(0);
	}

	*pio1 = *pio;
	while (argc > 0) {
		register struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_name == 0) {
			fprintf(stderr, "pppconfig: illegal parameter name %s\n", *argv);
			exit(1);
		}
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				(*p->c_func)(argv[1]);
				argc--, argv++;
			} else if (p->c_parameter == ZERO)
				(*p->c_func)("0");
			else
				(*p->c_func)(p->c_parameter);
		}
		argc--, argv++;
	}
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	if (memcmp(pio, pio1, sizeof *pio) &&
	    ioctl(s, PPPIOCSPAR, (caddr_t)&ifr) < 0) {
		Perror("ioctl (PPPIOCSPAR)");
		exit(1);
	}
	exit(0);
}

setflags(val)
	int val;
{

	if (val < 0)
		pio->ppp_flags &= ~(-val);
	else
		pio->ppp_flags |= val;
}

setcmap(arg)
	char *arg;
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
	fprintf(stderr, "pppconfig: bad character map\n");
	exit(1);
}

mru(arg)
	char *arg;
{
	int v = atoi(arg);

	if (v < 128 || v > 1500) {
		fprintf(stderr, "pppconf: bad value for mru -- must be from 128 to 1500\n");
		exit(1);
	}
	pio->ppp_mru = v;
}

idletime(arg)
	char *arg;
{

	pio->ppp_idletime = atoi(arg);
}

maxconf(arg)
	char *arg;
{

	pio->ppp_maxconf = atoi(arg);
}

maxterm(arg)
	char *arg;
{

	if ((pio->ppp_maxterm = atoi(arg)) == 0) {
		fprintf(stderr, "pppconf: bad value for maxterm -- should be non-zero\n");
		exit(1);
	}
}

timeout(arg)
	char *arg;
{

	if ((pio->ppp_timeout = atoi(arg)) == 0) {
		fprintf(stderr, "pppconf: bad value for timeout -- should be non-zero\n");
		exit(1);
	}
}

Perror(cmd)
	char *cmd;
{
	extern int errno;

	fprintf(stderr, "pppconfig: ");
	switch (errno) {

	case ENXIO:
		fprintf(stderr, "%s: no such interface\n", cmd);
		break;

	case EPERM:
		fprintf(stderr, "%s: permission denied\n", cmd);
		break;

	default:
		perror(cmd);
	}
	exit(1);
}
