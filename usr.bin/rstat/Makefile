#	BSDI $Id: Makefile,v 1.1.1.1 1993/03/08 06:50:46 polk Exp $

RPCGEN=	rpcgen

PROG=	rstat
SRCS=	rstat.c rstat_clnt.c rstat_xdr.c
COPTS=	-I.
DPADD+=	${LIBRPC}
LDADD+=	-lrpc
CLEANFILES+=rstat_clnt.c rstat_xdr.c rstat.h rstat.x

.depend: rstat.h rstat_clnt.c rstat_xdr.c

rstat.x: ${.CURDIR}/rstat.rpc
	ln -s ${.CURDIR}/rstat.rpc rstat.x
rstat.h: rstat.x
	$(RPCGEN) -h -o ${.TARGET} rstat.x
rstat_clnt.c: rstat.x
	$(RPCGEN) -l -o ${.TARGET} rstat.x
rstat_xdr.c: rstat.x
	$(RPCGEN) -c -o ${.TARGET} rstat.x

.include <bsd.prog.mk>
