# RE_SID: @(%)/tmp_mnt/vol/dosnfs/shades_SCCS/unix/pcnfsd/v2/src/SCCS/s.Makefile 1.13 93/02/10 17:20:39 SMI
#
#	@(#)Makefile	1.13	10 Feb 1993
#
# Makefile for rpc.pcnfsd
#

SVR_SRCS=  \
	pcnfsd_svc.c \
	pcnfsd_xdr.c \
	pcnfsd_v1.c \
	pcnfsd_v2.c \
	pcnfsd_misc.c \
	pcnfsd_cache.c \
	pcnfsd_print.c

TST_SRCS= \
	pcnfsd_clnt.c \
	pcnfsd_test.c \
	pcnfsd_xdr.c

HDRS=	pcnfsd.h common.h

no_type:
	@echo "Usage: make {sunos | sunos.403c | svr4 | bsd | ultrix.4x}"

sunos:	$(SVR_SRCS) $(TST_SRCS) $(HDRS) Makefile.sunos
	@-mkdir sunos 
	make -f Makefile.sunos

sunos.403c:	$(SVR_SRCS) $(TST_SRCS) $(HDRS) Makefile.sunos.403c
	@-mkdir sunos.403c 
	make -f Makefile.sunos.403c

svr4:	$(SVR_SRCS) $(TST_SRCS) $(HDRS) Makefile.svr4
	@-mkdir svr4
	make -f Makefile.svr4

bsd:	$(SVR_SRCS) $(TST_SRCS) $(HDRS) Makefile.bsd
	@-mkdir bsd
	make -f Makefile.bsd

ultrix.4x:	$(SVR_SRCS) $(TST_SRCS) $(HDRS) Makefile.ultrix.4x
	@-mkdir ultrix.4x
	make -f Makefile.ultrix.4x


clean:
	rm -rf sunos sunos.403c svr4 bsd ultrix.4x

clean_sunos:
	rm -rf sunos

clean_sunos.403c:
	rm -rf sunos.403c

clean_svr4:
	rm -rf svr4

clean_bsd:
	rm -rf bsd

clean_ultrix.4x:
	rm -rf ultrix.4x
