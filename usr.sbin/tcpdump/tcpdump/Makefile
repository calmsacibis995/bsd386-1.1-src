#	BSDI $Id: Makefile,v 1.2 1994/01/15 23:41:30 donn Exp $

PROG=	tcpdump
COPTS+=	-DCSLIP -DPPP -DFDDI -I.
SRCS=	addrtoname.c bpf_dump.c bpf_filter.c bpf_image.c etherent.c \
	gencode.c inet.c md.c nametoaddr.c optimize.c os.c pcap.c \
	print-arp.c print-atalk.c print-bootp.c print-domain.c print-egp.c \
	print-ether.c print-fddi.c print-icmp.c print-ip.c print-nfs.c \
	print-ntp.c print-null.c print-ospf.c print-ppp.c print-rip.c \
	print-sl.c print-snmp.c print-sunrpc.c print-tcp.c print-tftp.c \
	print-udp.c savefile.c tcpdump.c util.c version.c tcpgram.y tcplex.l
DPADD+=	${LIBL}
LDADD+=	-ll
CLEANFILES+=y.tab.h

.include <../../Makefile.inc>
.include <bsd.prog.mk>
