/*
 * Copyright (c) 1983, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
char copyright[] =
"%Z% Copyright (c) 1983, 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "%W% (Berkeley) %G%";
#endif /* not lint */

#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <machine/pte.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <kvm.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <net/route.h>
#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>

struct	pklcd pklcd;
struct	socket sockb;
extern	int Aflag;
extern	int aflag;
extern	int nflag;

union	{
struct sockaddr_x25	sx25;
char	data[128];
} laddr, faddr;
#define kget(o, p) \
	(kvm_read((off_t)(o), (char *)&p, sizeof (p)))

static	int first = 1;
#define lcd_next lcd_q.q_forw
#define lcd_prev lcd_q.q_back

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
ccitt_protopr(off, name)
	off_t off;
	char *name;
{
	struct pklcd cb;
	register struct pklcd *prev, *next;

	if (off == 0) {
		if (af != AF_UNSPEC || pflag)
			printf("%s control block: symbol not in namelist\n",
			    name);
		return;
	}
	kget(off, cb);
	pklcd = cb;
	prev = (struct pklcd *)off;
	if (pklcd.lcd_next == (struct pklcd_q *)off)
		return;
	while (pklcd.lcd_next != (struct pklcd_q *)off) {
		next = pklcd.lcd_next;
		kget(next, pklcd);
		if (pklcd.lcd_prev != prev) {
			printf("prev 0x%x next 0x%x lcd_prev 0x%x lcd_next 0x%x???\n",
				prev, next, pklcd.lcd_prev, pklcd.lcd_next);
			break;
		}
		kget(pklcd.lcd_socket, sockb);
		if (first) {
			printf("Active X.25 net connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
				"Proto", "Recv-Q", "Send-Q",
				"Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			printf("%8x ", (off_t)next);
		printf("%-5.5s %6d %6d ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		if (pklcd.lcd_laddr[0] == 0)
			printf("*.*\t");
		else {
			if ((char *)pklcd.lcd_laddr == ((char *)next) +
				_offsetof(struct pklcd, lcd_sladdr))
				laddr.siso = pklcd.lcd_sladdr;
			else
				kget(pklcd.lcd_laddr, laddr);
			isonetprint(&laddr, 1);
		}
		if (pklcd.lcd_faddr[0] == 0)
			printf("*.*\t");
		else {
			if ((char *)pklcd.lcd_faddr == ((char *)next) +
				_offsetof(struct pklcd, lcd_sfaddr))
				faddr.siso = pklcd.lcd_sfaddr;
			else
				kget(pklcd.lcd_faddr, faddr);
			isonetprint(&faddr, 0);
		}
		if (istp) {
			if (tpcb.tp_state >= tp_NSTATES)
				printf(" %d", tpcb.tp_state);
			else
				printf(" %-12.12s", tp_sstring[tpcb.tp_state]);
		}
		putchar('\n');
		prev = next;
	}
}
/*
 *	Dump esis stats
 */
esis_stats(off, name)
	off_t	off;
	char	*name;
{
	struct esis_stat esis_stat;

	if (off == 0)
		return;
	kvm_read(off, (char *)&esis_stat, sizeof (struct esis_stat));
	printf("%s:\n", name);
	printf("\t%d esh sent, %d esh received\n", esis_stat.es_eshsent,
		esis_stat.es_eshrcvd);
	printf("\t%d ish sent, %d ish received\n", esis_stat.es_ishsent,
		esis_stat.es_ishrcvd);
	printf("\t%d rd sent, %d rd received\n", esis_stat.es_rdsent,
		esis_stat.es_rdrcvd);
	printf("\t%d pdus not sent due to insufficient memory\n", 
		esis_stat.es_nomem);
	printf("\t%d pdus received with bad checksum\n", esis_stat.es_badcsum);
	printf("\t%d pdus received with bad version number\n", 
		esis_stat.es_badvers);
	printf("\t%d pdus received with bad type field\n", esis_stat.es_badtype);
	printf("\t%d short pdus received\n", esis_stat.es_toosmall);
}

/*
 * Dump clnp statistics structure.
 */
clnp_stats(off, name)
	off_t off;
	char *name;
{
	struct clnp_stat clnp_stat;

	if (off == 0)
		return;
	kvm_read(off, (char *)&clnp_stat, sizeof (clnp_stat));

	printf("%s:\n\t%d total packets sent\n", name, clnp_stat.cns_sent);
	printf("\t%d total fragments sent\n", clnp_stat.cns_fragments);
	printf("\t%d total packets received\n", clnp_stat.cns_total);
	printf("\t%d with fixed part of header too small\n", 
		clnp_stat.cns_toosmall);
	printf("\t%d with header length not reasonable\n", clnp_stat.cns_badhlen);
	printf("\t%d incorrect checksum%s\n", 
		clnp_stat.cns_badcsum, plural(clnp_stat.cns_badcsum));
	printf("\t%d with unreasonable address lengths\n", clnp_stat.cns_badaddr);
	printf("\t%d with forgotten segmentation information\n", 
		clnp_stat.cns_noseg);
	printf("\t%d with an incorrect protocol identifier\n", clnp_stat.cns_noproto);
	printf("\t%d with an incorrect version\n", clnp_stat.cns_badvers);
	printf("\t%d dropped because the ttl has expired\n", 
		clnp_stat.cns_ttlexpired);
	printf("\t%d clnp cache misses\n", clnp_stat.cns_cachemiss);
	printf("\t%d clnp congestion experience bits set\n", 
		clnp_stat.cns_congest_set);
	printf("\t%d clnp congestion experience bits received\n", 
		clnp_stat.cns_congest_rcvd);
}
/*
 * Dump CLTP statistics structure.
 */
cltp_stats(off, name)
	off_t off;
	char *name;
{
	struct cltpstat cltpstat;

	if (off == 0)
		return;
	kvm_read(off, (char *)&cltpstat, sizeof (cltpstat));
	printf("%s:\n\t%u incomplete header%s\n", name,
		cltpstat.cltps_hdrops, plural(cltpstat.cltps_hdrops));
	printf("\t%u bad data length field%s\n",
		cltpstat.cltps_badlen, plural(cltpstat.cltps_badlen));
	printf("\t%u bad checksum%s\n",
		cltpstat.cltps_badsum, plural(cltpstat.cltps_badsum));
}


/*
 * Pretty print an iso address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */

#ifdef notdef
char *
isonetname(iso)
	register struct iso_addr *iso;
{
	struct sockaddr_iso sa;
	struct iso_hostent *ihe = 0;
	struct iso_hostent *iso_gethostentrybyaddr();
	struct iso_hostent *iso_getserventrybytsel();
	struct iso_hostent Ihe;
	static char line[80];
	char *index();

	bzero(line, sizeof(line));
	if( iso->isoa_afi ) {
		sa.siso_family = AF_ISO;
		sa.siso_addr = *iso;
		sa.siso_tsuffix = 0;

		if (!nflag )
			ihe = iso_gethostentrybyaddr( &sa, 0, 0 );
		if( ihe ) {
			Ihe = *ihe;
			ihe = &Ihe;
			sprintf(line, "%s", ihe->isoh_hname);
		} else {
			sprintf(line, "%s", iso_ntoa(iso));
		}
	} else {
		sprintf(line, "*");
	}
	return line;
}

isonetprint(iso, sufx, sufxlen, islocal)
	register struct iso_addr *iso;
	char *sufx;
	u_short	sufxlen;
	int islocal;
{
	struct iso_hostent *iso_getserventrybytsel(), *ihe;
	struct iso_hostent Ihe;
	char *line, *cp, *index();
	int Alen = Aflag?18:22;

	line =  isonetname(iso);
	cp = index(line, '\0');
	ihe = (struct iso_hostent *)0;

	if( islocal )
		islocal = 20;
	else 
		islocal = 22 + Alen;

	if(Aflag)
		islocal += 10 ;

	if(!nflag) {
		if( (cp -line)>10 ) {
			cp = line+10;
			bzero(cp, sizeof(line)-10);
		}
	}

	*cp++ = '.';
	if(sufxlen) {
		if( !Aflag && !nflag && (ihe=iso_getserventrybytsel(sufx, sufxlen))) {
			Ihe = *ihe;
			ihe = &Ihe;
		}
		if( ihe && (strlen(ihe->isoh_aname)>0) ) {
			sprintf(cp, "%s", ihe->isoh_aname);
		} else  {
			iso_sprinttsel(cp, sufx, sufxlen);
		}
	} else
		sprintf(cp, "*");
	/*
	fprintf(stdout, Aflag?" %-18.18s":" %-22.22s", line);
	*/

	if( strlen(line) > Alen ) {
		fprintf(stdout, " %s", line);
		fprintf(stdout, "\n %*.s", islocal+Alen," ");
	} else {
		fprintf(stdout, " %-*.*s", Alen, Alen,line);
	}
}
#endif

#ifdef notdef
x25_protopr(off, name)
	off_t off;
	char *name;
{
	static char *xpcb_states[] = {
		"CLOSED",
		"LISTENING",
		"CLOSING",
		"CONNECTING",
		"ACKWAIT",
		"OPEN",
	};
	register struct pklcd *prev, *next;
	struct x25_pcb xpcb;

	if (off == 0) {
		if (af != AF_UNSPEC || pflag)
			printf("%s control block: symbol not in namelist\n",
			    name);
		return;
	}
	kvm_read(off, &xpcb, sizeof (struct x25_pcb));
	prev = (struct pklcd *)off;
	if (xpcb.x_next == (struct pklcd *)off)
		return;
	while (xpcb.x_next != (struct pklcd *)off) {
		next = pklcd.lcd_next;
		kvm_read((off_t)next, &xpcb, sizeof (struct x25_pcb));
		if (xpcb.x_prev != prev) {
			printf("???\n");
			break;
		}
		kvm_read((off_t)xpcb.x_socket, &sockb, sizeof (sockb));

		if (!aflag &&
			xpcb.x_state == LISTENING ||
			xpcb.x_state == TP_CLOSED ) {
			prev = next;
			continue;
		}
		if (first) {
			printf("Active X25 net connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
				"Proto", "Recv-Q", "Send-Q",
				"Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		printf("%-5.5s %6d %6d ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		isonetprint(&xpcb.x_laddr.siso_addr, &xpcb.x_lport, 
			sizeof(xpcb.x_lport), 1);
		isonetprint(&xpcb.x_faddr.siso_addr, &xpcb.x_fport, 
			sizeof(xpcb.x_lport), 0);
		if (xpcb.x_state < 0 || xpcb.x_state >= x25_NSTATES)
			printf(" 0x0x0x0x0x0x0x0x0x%x", xpcb.x_state);
		else
			printf(" %-12.12s", xpcb_states[xpcb.x_state]);
		putchar('\n');
		prev = next;
	}
}
#endif

struct	tp_stat tp_stat;

tp_stats(off, name)
caddr_t off, name;
{
	if (off == 0) {
		if (af != AF_UNSPEC || pflag)
			printf("TP not configured\n\n");
		return;
	}
	printf("%s:\n", name);
	kget(off, tp_stat);
	tprintstat(&tp_stat, 8);
}

#define OUT stdout

#define plural(x) (x>1?"s":"")

tprintstat(s, indent)
register struct tp_stat *s;
int indent;
{
	fprintf(OUT,
		"%*sReceiving:\n",indent," ");
	fprintf(OUT,
		"\t%*s%d variable parameter%s ignored\n", indent," ",
		s->ts_param_ignored ,plural(s->ts_param_ignored));
	fprintf(OUT,
		"\t%*s%d invalid parameter code%s\n", indent, " ",
		s->ts_inv_pcode ,plural(s->ts_inv_pcode));
	fprintf(OUT,
		"\t%*s%d invalid parameter value%s\n", indent, " ",
		s->ts_inv_pval ,plural(s->ts_inv_pval));
	fprintf(OUT,
		"\t%*s%d invalid dutype%s\n", indent, " ",
		s->ts_inv_dutype ,plural(s->ts_inv_dutype));
	fprintf(OUT,
		"\t%*s%d negotiation failure%s\n", indent, " ",
		s->ts_negotfailed ,plural(s->ts_negotfailed));
	fprintf(OUT,
		"\t%*s%d invalid destination reference%s\n", indent, " ",
		s->ts_inv_dref ,plural(s->ts_inv_dref));
	fprintf(OUT,
		"\t%*s%d invalid suffix parameter%s\n", indent, " ",
		s->ts_inv_sufx ,plural(s->ts_inv_sufx));
	fprintf(OUT,
		"\t%*s%d invalid length\n",indent, " ", s->ts_inv_length);
	fprintf(OUT,
		"\t%*s%d invalid checksum%s\n", indent, " ",
		s->ts_bad_csum ,plural(s->ts_bad_csum));
	fprintf(OUT,
		"\t%*s%d DT%s out of order\n", indent, " ",
		s->ts_dt_ooo ,plural(s->ts_dt_ooo));
	fprintf(OUT,
		"\t%*s%d DT%s not in window\n", indent, " ",
		s->ts_dt_niw ,plural(s->ts_dt_niw));
	fprintf(OUT,
		"\t%*s%d duplicate DT%s\n", indent, " ",
		s->ts_dt_dup ,plural(s->ts_dt_dup));
	fprintf(OUT,
			"\t%*s%d XPD%s not in window\n", indent, " ",
			s->ts_xpd_niw ,plural(s->ts_xpd_niw));
		fprintf(OUT,
			"\t%*s%d XPD%s w/o credit to stash\n", indent, " ",
		s->ts_xpd_dup ,plural(s->ts_xpd_dup));
	fprintf(OUT,
		"\t%*s%d time%s local credit reneged\n", indent, " ",
		s->ts_lcdt_reduced ,plural(s->ts_lcdt_reduced));
	fprintf(OUT,
		"\t%*s%d concatenated TPDU%s\n", indent, " ",
		s->ts_concat_rcvd ,plural(s->ts_concat_rcvd));
	fprintf(OUT,
		"%*sSending:\n", indent, " ");
	fprintf(OUT,
		"\t%*s%d XPD mark%s discarded\n", indent, " ",
		s->ts_xpdmark_del ,plural(s->ts_xpdmark_del));
	fprintf(OUT,
		"\t%*sXPD stopped data flow %d time%s\n", indent, " ",
		s->ts_xpd_intheway ,plural(s->ts_xpd_intheway));
	fprintf(OUT,
		"\t%*s%d time%s foreign window closed\n", indent, " ",
		s->ts_zfcdt ,plural(s->ts_zfcdt));
	fprintf(OUT,
		"%*sMiscellaneous:\n", indent, " ");
	fprintf(OUT,
		"\t%*s%d small mbuf%s\n", indent, " ",
		s->ts_mb_small ,plural(s->ts_mb_small));
	fprintf(OUT,
		"\t%*s%d cluster%s\n", indent, " ",
		s->ts_mb_cluster, plural(s->ts_mb_cluster));
	fprintf(OUT,
		"\t%*s%d source quench \n",indent, " ", 
		s->ts_quench);
	fprintf(OUT,
		"\t%*s%d dec bit%s\n", indent, " ",
		s->ts_rcvdecbit, plural(s->ts_rcvdecbit));
	fprintf(OUT,
		"\t%*sM:L ( M mbuf chains of length L)\n", indent, " ");
	{
		register int j;

		fprintf(OUT, "\t%*s%d: over 16\n", indent, " ",
		s->ts_mb_len_distr[0]);
		for( j=1; j<=8; j++) {
			fprintf(OUT,
				"\t%*s%d: %d\t\t%d: %d\n", indent, " ",
				s->ts_mb_len_distr[j],j,
				s->ts_mb_len_distr[j<<1],j<<1
				);
		}
	}
	fprintf(OUT,
		"\t%*s%d EOT rcvd\n",  indent, " ", s->ts_eot_input);
	fprintf(OUT,
		"\t%*s%d EOT sent\n",  indent, " ", s->ts_EOT_sent);
	fprintf(OUT,
		"\t%*s%d EOT indication%s\n",  indent, " ",
		s->ts_eot_user ,plural(s->ts_eot_user));

	fprintf(OUT,
		"%*sConnections:\n", indent, " ");
	fprintf(OUT,
		"\t%*s%d connection%s used extended format\n",  indent, " ",
		s->ts_xtd_fmt ,plural(s->ts_xtd_fmt));
	fprintf(OUT,
		"\t%*s%d connection%s allowed transport expedited data\n",  indent, " ",
		s->ts_use_txpd ,plural(s->ts_use_txpd));
	fprintf(OUT,
		"\t%*s%d connection%s turned off checksumming\n",  indent, " ",
		s->ts_csum_off ,plural(s->ts_csum_off));
	fprintf(OUT,
		"\t%*s%d connection%s dropped due to retrans limit\n",  indent, " ",
		s->ts_conn_gaveup ,plural(s->ts_conn_gaveup));
	fprintf(OUT,
		"\t%*s%d tp 4 connection%s\n",  indent, " ",
		s->ts_tp4_conn ,plural(s->ts_tp4_conn));
	fprintf(OUT,
		"\t%*s%d tp 0 connection%s\n",  indent, " ",
		s->ts_tp0_conn ,plural(s->ts_tp0_conn));
	{
		register int j, div;
		register float f;
		static char *name[]= {
			"~LOCAL, PDN", 
			"~LOCAL,~PDN",
			" LOCAL,~PDN",
			" LOCAL, PDN"
		};
#define factor(i) \
	div = (s->ts_rtt[(i)].tv_sec * 1000000) + \
		s->ts_rtt[(i)].tv_usec ;\
	if(div) {\
		f = ((s->ts_rtv[(i)].tv_sec * 1000000) + \
				s->ts_rtv[(i)].tv_usec)/div;  \
		div = (int) (f + 0.5);\
	}

		fprintf(OUT, 
			"\n%*sRound trip times, listed in (sec: usec):\n", indent, " ");
		fprintf(OUT, 
			"\t%*s%11.11s  %12.12s | %12.12s | %s\n", indent, " ",
				"Category",
				"Smoothed avg", "Deviation", "Deviation/Avg");
		for( j=0; j<=3; j++) {
			factor(j);
			fprintf(OUT,
				"\t%*s%11.11s: %5d:%-6d | %5d:%-6d | %-6d\n", indent, " ",
				name[j],
				s->ts_rtt[j].tv_sec,
				s->ts_rtt[j].tv_usec,
				s->ts_rtv[j].tv_sec,
				s->ts_rtv[j].tv_usec,
				div);
		}
	}
	fprintf(OUT,
"\n%*sTpdus RECVD [%d valid, %3.6f %% of total (%d); %d dropped]\n",indent," ",
		s->ts_tpdu_rcvd ,
		((s->ts_pkt_rcvd > 0) ? 
			((100 * (float)s->ts_tpdu_rcvd)/(float)s->ts_pkt_rcvd)
			: 0),
		s->ts_pkt_rcvd,
		s->ts_recv_drop );

	fprintf(OUT,
		"\t%*sDT  %6d   AK  %6d   DR  %4d   CR  %4d \n", indent, " ",
		s->ts_DT_rcvd, s->ts_AK_rcvd, s->ts_DR_rcvd, s->ts_CR_rcvd);
	fprintf(OUT,
		"\t%*sXPD %6d   XAK %6d   DC  %4d   CC  %4d   ER  %4d\n",  indent, " ",
		s->ts_XPD_rcvd, s->ts_XAK_rcvd, s->ts_DC_rcvd, s->ts_CC_rcvd,
		s->ts_ER_rcvd);
	fprintf(OUT,
		"\n%*sTpdus SENT [%d total, %d dropped]\n",  indent, " ",
		s->ts_tpdu_sent, s->ts_send_drop);

	fprintf(OUT,
		"\t%*sDT  %6d   AK  %6d   DR  %4d   CR  %4d \n", indent, " ",
		s->ts_DT_sent, s->ts_AK_sent, s->ts_DR_sent, s->ts_CR_sent);
	fprintf(OUT,
		"\t%*sXPD %6d   XAK %6d   DC  %4d   CC  %4d   ER  %4d\n",  indent, " ",
		s->ts_XPD_sent, s->ts_XAK_sent, s->ts_DC_sent, s->ts_CC_sent,
		s->ts_ER_sent);

	fprintf(OUT,
		"\n%*sRetransmissions:\n", indent, " ");
#define PERCENT(X,Y) (((Y)>0)?((100 *(float)(X)) / (float) (Y)):0)

	fprintf(OUT,
	"\t%*sCR  %6d   CC  %6d   DR  %6d \n", indent, " ",
		s->ts_retrans_cr, s->ts_retrans_cc, s->ts_retrans_dr);
	fprintf(OUT,
	"\t%*sDT  %6d (%5.2f%%)\n", indent, " ",
		s->ts_retrans_dt,
		PERCENT(s->ts_retrans_dt, s->ts_DT_sent));
	fprintf(OUT,
	"\t%*sXPD %6d (%5.2f%%)\n",  indent, " ",
		s->ts_retrans_xpd,
		PERCENT(s->ts_retrans_xpd, s->ts_XPD_sent));
	

	fprintf(OUT,
		"\n%*sE Timers: [%6d ticks]\n", indent, " ", s->ts_Eticks);
	fprintf(OUT,
		"%*s%6d timer%s set \t%6d timer%s expired \t%6d timer%s cancelled\n",indent, " ",
		s->ts_Eset ,plural(s->ts_Eset),
		s->ts_Eexpired ,plural(s->ts_Eexpired),
		s->ts_Ecan_act ,plural(s->ts_Ecan_act));

	fprintf(OUT,
		"\n%*sC Timers: [%6d ticks]\n",  indent, " ",s->ts_Cticks);
	fprintf(OUT,
	"%*s%6d timer%s set \t%6d timer%s expired \t%6d timer%s cancelled\n",
		indent, " ",
		s->ts_Cset ,plural(s->ts_Cset),
		s->ts_Cexpired ,plural(s->ts_Cexpired),
		s->ts_Ccan_act ,plural(s->ts_Ccan_act));
	fprintf(OUT,
		"%*s%6d inactive timer%s cancelled\n", indent, " ",
		s->ts_Ccan_inact ,plural(s->ts_Ccan_inact));

	fprintf(OUT,
		"\n%*sPathological debugging activity:\n", indent, " ");
	fprintf(OUT,
		"\t%*s%6d CC%s sent to zero dref\n", indent, " ",
		s->ts_zdebug ,plural(s->ts_zdebug));
	/* SAME LINE AS ABOVE */
	fprintf(OUT,
		"\t%*s%6d random DT%s dropped\n", indent, " ",
		s->ts_ydebug ,plural(s->ts_ydebug));
	fprintf(OUT,
		"\t%*s%6d illegally large XPD TPDU%s\n", indent, " ",
		s->ts_vdebug ,plural(s->ts_vdebug));
	fprintf(OUT,
		"\t%*s%6d faked reneging of cdt\n", indent, " ",
		s->ts_ldebug );

	fprintf(OUT,
		"\n%*sACK reasons:\n", indent, " ");
	fprintf(OUT, "\t%*s%6d not acked immediately\n", indent, " ",
										s->ts_ackreason[_ACK_DONT_] );
	fprintf(OUT, "\t%*s%6d strategy==each\n", indent, " ",
										s->ts_ackreason[_ACK_STRAT_EACH_] );
	fprintf(OUT, "\t%*s%6d strategy==fullwindow\n", indent, " ",
										s->ts_ackreason[_ACK_STRAT_FULLWIN_] );
	fprintf(OUT, "\t%*s%6d duplicate DT\n", indent, " ",
										s->ts_ackreason[_ACK_DUP_] );
	fprintf(OUT, "\t%*s%6d EOTSDU\n", indent, " ",
										s->ts_ackreason[_ACK_EOT_] );
	fprintf(OUT, "\t%*s%6d reordered DT\n", indent, " ",
										s->ts_ackreason[_ACK_REORDER_] );
	fprintf(OUT, "\t%*s%6d user rcvd\n", indent, " ",
										s->ts_ackreason[_ACK_USRRCV_] );
	fprintf(OUT, "\t%*s%6d fcc reqd\n", 	indent, " ",
										s->ts_ackreason[_ACK_FCC_] );
}
#ifndef SSEL
#define SSEL(s) ((s)->siso_tlen + TSEL(s))
#define PSEL(s) ((s)->siso_slen + SSEL(s))
#endif

isonetprint(siso, islocal)
register struct sockaddr_iso *siso;
int islocal;
{
	hexprint(siso->siso_nlen, siso->siso_addr.isoa_genaddr, "{}");
	if (siso->siso_tlen || siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_tlen, TSEL(siso), "()");
	if (siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_slen, SSEL(siso), "[]");
	if (siso->siso_plen)
		hexprint(siso->siso_plen, PSEL(siso), "<>");
	putchar(' ');
}
static char hexlist[] = "0123456789abcdef", obuf[128];

hexprint(n, buf, delim)
char *buf, *delim;
{
	register u_char *in = (u_char *)buf, *top = in + n;
	register char *out = obuf;
	register int i;

	if (n == 0)
		return;
	while (in < top) {
		i = *in++;
		*out++ = '.';
		if (i > 0xf) {
			out[1] = hexlist[i & 0xf];
			i >>= 4;
			out[0] = hexlist[i];
			out += 2;
		} else
			*out++ = hexlist[i];
	}
	*obuf = *delim; *out++ = delim[1]; *out = 0;
	printf("%s", obuf);
}
