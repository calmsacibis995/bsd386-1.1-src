/*	BSDI $Id: if.c,v 1.4 1994/01/27 07:33:20 donn Exp $	*/

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
static char sccsid[] = "@(#)if.c	5.16 (Berkeley) 5/27/92";
#endif /* not lint */

#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "netstat.h"

#define	YES	1
#define	NO	0

static void sidewaysintpr __P((unsigned, off_t));
static void catchalarm __P(());

/*
 * Return a printable string representation of an Ethernet address.
 */
char *etherprint(enaddr)
	char enaddr[6];
{
	static char string[18];
	unsigned char *en = (unsigned char *)enaddr;

	sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x",
		en[0], en[1], en[2], en[3], en[4], en[5] );
	string[17] = '\0';
	return(string);
}

/*
 * Print a description of the network interfaces.
 */
void
intpr(interval, ifnetaddr)
	int interval;
	off_t ifnetaddr;
{
	struct ifnet ifnet;
	union {
		struct ifaddr ifa;
		struct in_ifaddr in;
		struct ns_ifaddr ns;
		struct iso_ifaddr iso;
	} ifaddr;
	off_t ifaddraddr, ifaddrfound, ifnetfound;
	struct sockaddr *sa;
	char name[32];
	struct arpcom ac;

	if (ifnetaddr == 0) {
		printf("ifnet: symbol not defined\n");
		return;
	}
	if (interval) {
		sidewaysintpr((unsigned)interval, ifnetaddr);
		return;
	}
	if (kread(ifnetaddr, (char *)&ifnetaddr, sizeof ifnetaddr))
		return;
	printf("%-5.5s %-5.5s %-11.11s %-17.17s %8.8s %5.5s %8.8s %5.5s",
		"Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs",
		"Opkts", "Oerrs");
	printf(" %5s", "Coll");
	if (tflag)
		printf(" %s", "Time");
	if (dflag)
		printf(" %s", "Drop");
	putchar('\n');
	ifaddraddr = 0;
	ifnetfound = 0;
	while (ifnetaddr || ifaddraddr) {
		struct sockaddr_in *sin;
		register char *cp;
		int n, m;

		ifnetfound = ifnetaddr;
		if (ifaddraddr == 0) {
			if (kread(ifnetaddr, (char *)&ifnet, sizeof ifnet) ||
			    kread((off_t)ifnet.if_name, name, 16))
				return;
			name[15] = '\0';
			ifnetaddr = (off_t) ifnet.if_next;
			if (interface != 0 &&
			    (strcmp(name, interface) != 0 || unit != ifnet.if_unit))
				continue;
			cp = index(name, '\0');
			cp += sprintf(cp, "%d", ifnet.if_unit);
			if ((ifnet.if_flags&IFF_UP) == 0)
				*cp++ = '*';
			*cp = '\0';
			ifaddraddr = (off_t)ifnet.if_addrlist;
		}
		printf("%-5.5s %-5d ", name, ifnet.if_mtu);
		ifaddrfound = ifaddraddr;
		if (ifaddraddr == 0) {
			printf("%-11.11s ", "none");
			printf("%-17.17s ", "none");
		} else {
			if (kread(ifaddraddr, (char *)&ifaddr, sizeof ifaddr)) {
				ifaddraddr = 0;
				continue;
			}
#define CP(x) ((char *)(x))
			cp = (CP(ifaddr.ifa.ifa_addr) - CP(ifaddraddr)) +
				CP(&ifaddr); sa = (struct sockaddr *)cp;
			switch (sa->sa_family) {
			case AF_UNSPEC:
				printf("%-11.11s ", "none");
				printf("%-17.17s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *)sa;
#ifdef notdef
				/* can't use inet_makeaddr because kernel
				 * keeps nets unshifted.
				 */
				in = inet_makeaddr(ifaddr.in.ia_subnet,
					INADDR_ANY);
				printf("%-11.11s ", netname(in.s_addr,
				    ifaddr.in.ia_subnetmask));
#else
				printf("%-11.11s ",
				    netname(htonl(ifaddr.in.ia_subnet),
				    ifaddr.in.ia_subnetmask));
#endif
				printf("%-17.17s ",
				    routename(sin->sin_addr.s_addr));
				break;
			case AF_NS:
				{
				struct sockaddr_ns *sns =
					(struct sockaddr_ns *)sa;
				u_long net;
				char netnum[8];

				*(union ns_net *) &net = sns->sns_addr.x_net;
		sprintf(netnum, "%lxH", ntohl(net));
				upHex(netnum);
				printf("ns:%-8s ", netnum);
				printf("%-17s ",
				    ns_phost(sns));
				}
				break;
			case AF_LINK:
				{
				struct sockaddr_dl *sdl =
					(struct sockaddr_dl *)sa;
				    cp = (char *)LLADDR(sdl);
				    n = sdl->sdl_alen;
				}
				printf("%-11s ", "<Link>");
				switch (ifnet.if_type) {
				case IFT_ETHER:
				    /*
				     * This is gross, but will suffice
				     * until drivers are fixed to set the
				     * link addr.
				     */
				    if (n == 0) {
					kread(ifnetfound, (char *)&ac,
					    sizeof(ac));
					cp = (char *)ac.ac_enaddr;
					n = 6;
				    }
				    m = 12 + 15 - m;
				    printf("%-17.17s ", etherprint(cp));
				    break;
				default:
				    m = 17 + 1;
				    goto hexprint;
				}
				break;
			default:
				m = 30 - printf("(%d)", sa->sa_family);
				for (cp = sa->sa_len + (char *)sa;
					--cp > sa->sa_data && (*cp == 0);) {}
				n = cp - sa->sa_data + 1;
				cp = sa->sa_data;
			hexprint:
				while (--n >= 0)
					m -= printf("%x%c", *cp++ & 0xff,
						    n > 0 ? '.' : ' ');
				while (m-- > 0)
					putchar(' ');
				break;
			}
			ifaddraddr = (off_t)ifaddr.ifa.ifa_next;
		}
		printf("%8d %5d %8d %5d %5d",
		    ifnet.if_ipackets, ifnet.if_ierrors,
		    ifnet.if_opackets, ifnet.if_oerrors,
		    ifnet.if_collisions);
		if (tflag)
			printf(" %3d", ifnet.if_timer);
		if (dflag)
			printf(" %3d", ifnet.if_snd.ifq_drops);
		putchar('\n');

		if (aflag && ifaddrfound) {
			/*
			 * print any internet multicast addresses
			 */
			switch (sa->sa_family) {
			case AF_INET:
			    {
				off_t multiaddr;
				struct in_multi inm;

				multiaddr = (off_t)ifaddr.in.ia_multiaddrs;
				while (multiaddr != 0) {
					kread(multiaddr, (char *)&inm,
						sizeof inm);
					multiaddr = (off_t)inm.inm_next;
					printf("%23s %-19.19s\n", "",
					       routename(inm.inm_addr.s_addr));
				}
				break;
			    }

			default:
				break;

			case AF_LINK:
			    /*
			     * print link-level addresses
			     */
			    if (ifnet.if_type == IFT_ETHER) {   /* Ethernet */
				off_t multiaddr;
				struct ether_multi enm;

				multiaddr = (off_t)ac.ac_multiaddrs;
				while (multiaddr != 0) {
					kread(multiaddr, (char *)&enm,
						sizeof enm);
					multiaddr = (off_t)enm.enm_next;
					printf("%23s %s", "",
						etherprint(&enm.enm_addrlo));
					if (bcmp(&enm.enm_addrlo,
						 &enm.enm_addrhi, 6) != 0)
						printf(" to %s",
						etherprint(&enm.enm_addrhi));
					printf("\n");
				}
			    }
			    break;
			}
		}
	}
}

#define	MAXIF	10
struct	iftot {
	char	ift_name[16];		/* interface name */
	int	ift_ip;			/* input packets */
	int	ift_ie;			/* input errors */
	int	ift_op;			/* output packets */
	int	ift_oe;			/* output errors */
	int	ift_co;			/* collisions */
	int	ift_dr;			/* drops */
} iftot[MAXIF];

u_char	signalled;			/* set if alarm goes off "early" */

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
static void
sidewaysintpr(interval, off)
	unsigned interval;
	off_t off;
{
	struct ifnet ifnet;
	off_t firstifnet;
	register struct iftot *ip, *total;
	register int line;
	struct iftot *lastif, *sum, *interesting;
	int oldmask;

	if (kread(off, (char *)&firstifnet, sizeof (off_t)))
		return;
	lastif = iftot;
	sum = iftot + MAXIF - 1;
	total = sum - 1;
	interesting = iftot;
	for (off = firstifnet, ip = iftot; off;) {
		char *cp;

		if (kread(off, (char *)&ifnet, sizeof ifnet))
			break;
		ip->ift_name[0] = '(';
		if (kread((off_t)ifnet.if_name, ip->ift_name + 1, 15))
			break;
		if (interface && strcmp(ip->ift_name + 1, interface) == 0 &&
		    unit == ifnet.if_unit)
			interesting = ip;
		ip->ift_name[15] = '\0';
		cp = index(ip->ift_name, '\0');
		sprintf(cp, "%d)", ifnet.if_unit);
		ip++;
		if (ip >= iftot + MAXIF - 2)
			break;
		off = (off_t) ifnet.if_next;
	}
	lastif = ip;

	(void)signal(SIGALRM, catchalarm);
	signalled = NO;
	(void)alarm(interval);
banner:
	printf("   input    %-6.6s    output       ", interesting->ift_name);
	if (lastif - iftot > 0) {
		if (dflag)
			printf("      ");
		printf("     input   (Total)    output");
	}
	for (ip = iftot; ip < iftot + MAXIF; ip++) {
		ip->ift_ip = 0;
		ip->ift_ie = 0;
		ip->ift_op = 0;
		ip->ift_oe = 0;
		ip->ift_co = 0;
		ip->ift_dr = 0;
	}
	putchar('\n');
	printf("%8.8s %5.5s %8.8s %5.5s %5.5s ",
		"packets", "errs", "packets", "errs", "colls");
	if (dflag)
		printf("%5.5s ", "drops");
	if (lastif - iftot > 0)
		printf(" %8.8s %5.5s %8.8s %5.5s %5.5s",
			"packets", "errs", "packets", "errs", "colls");
	if (dflag)
		printf(" %5.5s", "drops");
	putchar('\n');
	fflush(stdout);
	line = 0;
loop:
	sum->ift_ip = 0;
	sum->ift_ie = 0;
	sum->ift_op = 0;
	sum->ift_oe = 0;
	sum->ift_co = 0;
	sum->ift_dr = 0;
	for (off = firstifnet, ip = iftot; off && ip < lastif; ip++) {
		if (kread(off, (char *)&ifnet, sizeof ifnet)) {
			off = 0;
			continue;
		}
		if (ip == interesting) {
			printf("%8d %5d %8d %5d %5d",
				ifnet.if_ipackets - ip->ift_ip,
				ifnet.if_ierrors - ip->ift_ie,
				ifnet.if_opackets - ip->ift_op,
				ifnet.if_oerrors - ip->ift_oe,
				ifnet.if_collisions - ip->ift_co);
			if (dflag)
				printf(" %5d",
				    ifnet.if_snd.ifq_drops - ip->ift_dr);
		}
		ip->ift_ip = ifnet.if_ipackets;
		ip->ift_ie = ifnet.if_ierrors;
		ip->ift_op = ifnet.if_opackets;
		ip->ift_oe = ifnet.if_oerrors;
		ip->ift_co = ifnet.if_collisions;
		ip->ift_dr = ifnet.if_snd.ifq_drops;
		sum->ift_ip += ip->ift_ip;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		sum->ift_dr += ip->ift_dr;
		off = (off_t) ifnet.if_next;
	}
	if (lastif - iftot > 0) {
		printf("  %8d %5d %8d %5d %5d",
			sum->ift_ip - total->ift_ip,
			sum->ift_ie - total->ift_ie,
			sum->ift_op - total->ift_op,
			sum->ift_oe - total->ift_oe,
			sum->ift_co - total->ift_co);
		if (dflag)
			printf(" %5d", sum->ift_dr - total->ift_dr);
	}
	*total = *sum;
	putchar('\n');
	fflush(stdout);
	line++;
	oldmask = sigblock(sigmask(SIGALRM));
	if (! signalled) {
		sigpause(0);
	}
	sigsetmask(oldmask);
	signalled = NO;
	(void)alarm(interval);
	if (line == 21)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
static void
catchalarm()
{
	signalled = YES;
}
