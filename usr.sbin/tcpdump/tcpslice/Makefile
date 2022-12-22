#	BSDI $Id: Makefile,v 1.1.1.1 1993/03/08 17:46:34 polk Exp $

PROG=	tcpslice
COPTS+=	-DCSLIP -DPPP -DFDDI
COPTS+=	-I${.CURDIR}/../tcpdump
SRCS=	tcpslice.c gwtm2secs.c search.c savefile.c bpf_filter.c md.c util.c

.PATH:	${.CURDIR}/../tcpdump

.include <../../Makefile.inc>
.include <bsd.prog.mk>
