#	@(#)Makefile	2.1 88/08/01 4.0 RPCSRC

PROG=	rpcinfo
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil
MAN5=	rpc.0
MAN8=	rpcinfo.0

RPCOWN=	${BINOWN}
RPCGRP=	${BINGRP}
RPCMODE=444

afterinstall:
	install -c -o ${RPCOWN} -g ${RPCGRP} -m ${RPCMODE} \
				${.CURDIR}/rpc ${DESTDIR}/etc/rpc

.include <bsd.prog.mk>
