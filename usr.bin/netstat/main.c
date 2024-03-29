/*	BSDI $Id: main.c,v 1.7 1994/01/04 18:24:12 karels Exp $	*/

/*
 * Copyright (c) 1983, 1988 Regents of the University of California.
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983, 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.23 (Berkeley) 7/1/91";
#endif /* not lint */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <kvm.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include "netstat.h"

struct nlist nl[] = {
#define	N_MBSTAT	0
	{ "_mbstat" },
#define	N_IPSTAT	1
	{ "_ipstat" },
#define	N_TCB		2
	{ "_tcb" },
#define	N_TCPSTAT	3
	{ "_tcpstat" },
#define	N_UDB		4
	{ "_udb" },
#define	N_UDPSTAT	5
	{ "_udpstat" },
#define	N_IFNET		6
	{ "_ifnet" },
#define	N_IMP		7
	{ "_imp_softc" },
#define	N_RTHOST	8
	{ "_rthost" },
#define	N_RTNET		9
	{ "_rtnet" },
#define	N_ICMPSTAT	10
	{ "_icmpstat" },
#define	N_RTSTAT	11
	{ "_rtstat" },
#define	N_NFILE		12
	{ "_nfile" },
#define	N_FILE		13
	{ "_file" },
#define	N_UNIXSW	14
	{ "_unixsw" },
#define N_RTHASHSIZE	15
	{ "_rthashsize" },
#define N_IDP		16
	{ "_nspcb"},
#define N_IDPSTAT	17
	{ "_idpstat"},
#define N_SPPSTAT	18
	{ "_spp_istat"},
#define N_NSERR		19
	{ "_ns_errstat"},
#define	N_CLNPSTAT	20
	{ "_clnp_stat"},
#define	IN_TP		21
	{ "_tp_inpcb" },
#define	ISO_TP		22
	{ "_tp_isopcb" },
#define	N_TPSTAT	23
	{ "_tp_stat" },
#define	N_ESISSTAT	24
	{ "_esis_stat"},
#define N_NIMP		25
	{ "_nimp"},
#define N_RTREE		26
	{ "_radix_node_head"},
#define N_CLTP		27
	{ "_cltb"},
#define N_CLTPSTAT	28
	{ "_cltpstat"},
#define N_IGMPSTAT	29
	{ "_igmpstat" },
#define N_MRTPROTO	30
	{ "_ip_mrtproto" },
#define N_MRTSTAT	31
	{ "_mrtstat" },
#define N_MRTTABLE	32
	{ "_mrttable" },
#define N_VIFTABLE	33
	{ "_viftable" },
	"",
};

typedef void (*F) __P((off_t, char *));

struct protox {
	u_char	pr_index;		/* index into nlist of cb head */
	u_char	pr_sindex;		/* index into nlist of stat block */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	void	(*pr_cblocks)
	    __P((off_t, char *));	/* control blocks printing routine */
	void	(*pr_stats)
	    __P((off_t, char *));	/* statistics printing routine */
	char	*pr_name;		/* well-known name */
} protox[] = {
	{ N_TCB,	N_TCPSTAT,	1,	protopr,
	  tcp_stats,	"tcp" },
	{ N_UDB,	N_UDPSTAT,	1,	protopr,
	  udp_stats,	"udp" },
	{ IN_TP,	N_TPSTAT,	1,	protopr,
	  (F)tp_stats,	"tpip" },
	{ -1,		N_IPSTAT,	1,	0,
	  ip_stats,	"ip" },
	{ -1,		N_ICMPSTAT,	1,	0,
	  icmp_stats,	"icmp" },
	{ -1,		N_IGMPSTAT,	1,	0,
	  igmp_stats,	"igmp" },
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct protox nsprotox[] = {
	{ N_IDP,	N_IDPSTAT,	1,	nsprotopr,
	  idp_stats,	"idp" },
	{ N_IDP,	N_SPPSTAT,	1,	nsprotopr,
	  spp_stats,	"spp" },
	{ -1,		N_NSERR,	1,	0,
	  nserr_stats,	"ns_err" },
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct protox isoprotox[] = {
	{ ISO_TP,	N_TPSTAT,	1,	iso_protopr,
	  (F)tp_stats,	"tp" },
	{ N_CLTP,	N_CLTPSTAT,	1,	iso_protopr,
	  cltp_stats,	"cltp" },
	{ -1,		N_CLNPSTAT,	1,	0,
	  clnp_stats,	"clnp"},
	{ -1,		N_ESISSTAT,	1,	0,
	  esis_stats,	"esis"},
	{ -1,		-1,		0,	0,
	  0,		0 }
};

struct protox *protoprotox[] = { protox, nsprotox, isoprotox, NULL };

static void usage __P(());
static struct protox *name2protox __P((char *));
static struct protox *knownname __P((char *));

kvm_t *kvmd;

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	register struct protoent *p;
	register struct protox *tp;	/* for printing cblocks & stats */
	struct protox *name2protox();	/* for -p */
	register char *cp;
	int ch;
	char *nlistf = NULL, *memf = NULL;

	af = AF_UNSPEC;

	while ((ch = getopt(argc, argv, "Aadf:ghI:iM:mN:np:rstuw:")) != EOF)
		switch (ch) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			if (strcmp(optarg, "ns") == 0)
				af = AF_NS;
			else if (strcmp(optarg, "inet") == 0)
				af = AF_INET;
			else if (strcmp(optarg, "unix") == 0)
				af = AF_UNIX;
			else if (strcmp(optarg, "iso") == 0)
				af = AF_ISO;
			else {
				(void)fprintf(stderr,
				    "%s: unknown address family\n", optarg);
				exit(1);
			}
			break;
		case 'g':
			gflag++;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'I': {
			char *cp;

			iflag = 1;
			for (cp = interface = optarg; isalpha(*cp); cp++);
			unit = atoi(cp);
			*cp = '\0';
			break;
		}
		case 'i':
			iflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'p':
			if ((tp = name2protox(optarg)) == NULL) {
				(void)fprintf(stderr,
				    "%s: unknown or uninstrumented protocol\n",
				    optarg);
				exit(1);
			}
			pflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag++;	/* multiple uses are significant */
			break;
		case 't':
			tflag = 1;
			break;
		case 'u':
			af = AF_UNIX;
			break;
		case 'w':
			interval = atoi(optarg);
			iflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		if (isdigit(**argv)) {
			interval = atoi(*argv);
			if (interval <= 0)
				usage();
			++argv;
			iflag = 1;
		}
		if (*argv) {
			nlistf = *argv;
			if (*++argv)
				memf = *argv;
		}
	}
#endif

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());

	if ((kvmd = kvm_open(nlistf, memf, NULL, O_RDONLY, NULL)) == NULL) {
		fprintf(stderr, "netstat: kvm_open: can't get namelist\n");
		exit(1);
	}
	if (kvm_nlist(kvmd, nl) < 0 || nl[0].n_type == 0) {
		if (nlistf)
			fprintf(stderr, "netstat: %s: no namelist\n", nlistf);
		else
			fprintf(stderr, "netstat: no namelist\n");
		exit(1);
	}
	if (mflag) {
		mbpr((off_t)nl[N_MBSTAT].n_value);
		exit(0);
	}
	if (pflag) {
		if (tp->pr_stats)
			(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
				tp->pr_name);
		else
			printf("%s: no stats routine\n", tp->pr_name);
		exit(0);
	}
	if (hflag) {
		hostpr(nl[N_IMP].n_value, nl[N_NIMP].n_value);
		exit(0);
	}
	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	sethostent(1);
	setnetent(1);
	if (iflag) {
		intpr(interval, nl[N_IFNET].n_value);
		exit(0);
	}
	if (rflag) {
		if (sflag)
			rt_stats((off_t)nl[N_RTSTAT].n_value);
		else
			routepr((off_t)nl[N_RTHOST].n_value, 
				(off_t)nl[N_RTNET].n_value,
				(off_t)nl[N_RTHASHSIZE].n_value,
				(off_t)nl[N_RTREE].n_value);
		exit(0);
	}
	if (gflag) {
		if (sflag)
			mrt_stats((off_t)nl[N_MRTPROTO].n_value,
				  (off_t)nl[N_MRTSTAT].n_value);
		else
			mroutepr((off_t)nl[N_MRTPROTO].n_value,
				 (off_t)nl[N_MRTTABLE].n_value,
				 (off_t)nl[N_VIFTABLE].n_value);
		exit(0);
	}

    if (af == AF_INET || af == AF_UNSPEC) {
	setprotoent(1);
	setservent(1);
	while (p = getprotoent()) {

		for (tp = protox; tp->pr_name; tp++)
			if (strcmp(tp->pr_name, p->p_name) == 0)
				break;
		if (tp->pr_name == 0 || tp->pr_wanted == 0)
			continue;
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					p->p_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					p->p_name);
	}
	endprotoent();
    }
    if (af == AF_NS || af == AF_UNSPEC) {
	for (tp = nsprotox; tp->pr_name; tp++) {
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					tp->pr_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					tp->pr_name);
	}
    }
    if (af == AF_ISO || af == AF_UNSPEC) {
	for (tp = isoprotox; tp->pr_name; tp++) {
		if (sflag) {
			if (tp->pr_stats)
				(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
					tp->pr_name);
		} else
			if (tp->pr_cblocks)
				(*tp->pr_cblocks)(nl[tp->pr_index].n_value,
					tp->pr_name);
	}
    }
    if ((af == AF_UNIX || af == AF_UNSPEC) && !sflag)
	    unixpr((struct protosw *)nl[N_UNIXSW].n_value);
    if (af == AF_UNSPEC && sflag)
	impstats(nl[N_IMP].n_value, nl[N_NIMP].n_value);
    exit(0);
}

/*
 * Read kernel memory, return 0 on success.
 */
int
kread(addr, buf, size)
	off_t addr;
	char *buf;
	int size;
{

	if (kvm_read(kvmd, addr, buf, size) != size) {
		/* XXX this duplicates kvm_read's error printout */
		(void)fprintf(stderr, "netstat: kvm_read %s\n",
		    kvm_geterr(kvmd));
		return (-1);
	}
	return (0);
}

char *
plural(n)
	int n;
{
	return (n != 1 ? "s" : "");
}

/*
 * Find the protox for the given "well-known" name.
 */
static struct protox *
knownname(name)
	char *name;
{
	struct protox **tpp, *tp;

	for (tpp = protoprotox; *tpp; tpp++)
	    for (tp = *tpp; tp->pr_name; tp++)
		if (strcmp(tp->pr_name, name) == 0)
			return(tp);
	return(NULL);
}

/*
 * Find the protox corresponding to name.
 */
static struct protox *
name2protox(name)
	char *name;
{
	struct protox *tp;
	char **alias;			/* alias from p->aliases */
	struct protoent *p;

	/*
	 * Try to find the name in the list of "well-known" names. If that
	 * fails, check if name is an alias for an Internet protocol.
	 */
	if (tp = knownname(name))
		return(tp);

	setprotoent(1);			/* make protocol lookup cheaper */
	while (p = getprotoent()) {
		/* assert: name not same as p->name */
		for (alias = p->p_aliases; *alias; alias++)
			if (strcmp(name, *alias) == 0) {
				endprotoent();
				return(knownname(p->p_name));
			}
	}
	endprotoent();
	return(NULL);
}

static void
usage()
{
	(void)fprintf(stderr,
"usage: netstat [-Aan] [-f address_family] [-M core] [-N system]\n");
	(void)fprintf(stderr,
"               [-ghimnrs] [-f address_family] [-M core] [-N system]\n");
	(void)fprintf(stderr,
"               [-n] [-I interface] [-M core] [-N system] [-w wait]\n");
	(void)fprintf(stderr,
"               [-M core] [-N system] [-p protocol]\n");
	exit(1);
}
