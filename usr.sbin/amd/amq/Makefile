#	@(#)Makefile	5.4 (Berkeley) 5/12/91

PROG=	amq
MAN8=	amq.0
SRCS=	amq.c amq_clnt.c amq_xdr.c misc_rpc.c
LDADD+=	-lrpc
CFLAGS+=-I${.CURDIR}/../include
CFLAGS+=-I${.CURDIR}/../rpcx
CFLAGS+=-I${.CURDIR}/../config
CFLAGS+=-DARCH_REP=\"${MACHINE}\"
CFLAGS+=-DOS_REP=\"bsd44\"
CFLAGS+=-DOS_HDR=\"os-bsd44.h\"
.PATH: ${.CURDIR}/../rpcx ${.CURDIR}/../amd

.include "../../Makefile.inc"
.include <bsd.prog.mk>
