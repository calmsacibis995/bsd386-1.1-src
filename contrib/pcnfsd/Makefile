PROG=pcnfsd
SRCS=	pcnfsd_svc.c pcnfsd_xdr.c pcnfsd_v1.c pcnfsd_v2.c \
	pcnfsd_misc.c pcnfsd_cache.c pcnfsd_print.c 
CFLAGS+=-DOSVER_BSD386
LDADD+= -lrpc
DPADD+=	${LIBRPC}
MAN8=pcnfsd.0

.include <bsd.prog.mk>
