#	BSDI $Id: Makefile,v 1.3 1994/02/03 05:55:39 polk Exp $

RPCGEN=	rpcgen

PROG=	rstatd
MAN8=	rstatd.0
SRCS=	rstat_svc.c rstat_proc.c rstat_xdr.c
COPTS=	-I.
DPADD+=	${LIBRPC} ${LIBUTIL} ${LIBKVM}
LDADD+=	-lrpc -lutil -lkvm
CLEANFILES+=rstat_svc.c rstat_xdr.c rstat.h rstat.x

.depend: rstat.h rstat_svc.c rstat_xdr.c

rstat.x: ${.CURDIR}/../../usr.bin/rstat/rstat.rpc
	ln -s ${.CURDIR}/../../usr.bin/rstat/rstat.rpc rstat.x
rstat.h: rstat.x
	$(RPCGEN) -h -o ${.TARGET} rstat.x
rstat_svc.c: rstat.x
	$(RPCGEN) -s udp -o ${.TARGET} rstat.x
rstat_xdr.c: rstat.x
	$(RPCGEN) -c -o ${.TARGET} rstat.x

.include <bsd.prog.mk>
