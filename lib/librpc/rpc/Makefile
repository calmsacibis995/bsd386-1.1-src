#	@(#)Makefile	5.11 (Berkeley) 9/6/90

LIB=	rpc
CFLAGS+=-I${.CURDIR}/..
SRCS=	auth_none.c auth_unix.c  authunix_prot.c bindresvport.c \
	clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c clnt_tcp.c \
	clnt_udp.c rpc_dtablesize.c get_myaddress.c getrpcent.c getrpcport.c \
	pmap_clnt.c pmap_getmaps.c pmap_getport.c pmap_prot.c \
	pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_commondata.c rpc_callmsg.c \
	svc.c svc_auth.c svc_auth_unix.c svc_raw.c svc_run.c svc_simple.c \
	svc_tcp.c svc_udp.c xdr.c xdr_array.c xdr_float.c xdr_mem.c \
	xdr_rec.c xdr_reference.c xdr_stdio.c

HDRS=	auth.h auth_unix.h clnt.h pmap_clnt.h \
	pmap_prot.h pmap_rmt.h rpc.h rpc_msg.h svc.h svc_auth.h types.h xdr.h

MAN3=	bindresvport.0 getrpcent.0 getrpcport.0 rpc.0 xdr.0
MLINKS=	getrpcent.3 getrpcbyname.3 getrpcent.3 getrpcbynumber.3

all: librpc.a

beforeinstall: ${HDRS}
	cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${HDRS} \
	    ${DESTDIR}/usr/include/rpc

.include <bsd.lib.mk>
