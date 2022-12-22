#	@(#)Makefile	5.11 (Berkeley) 2/19/91

PROG=	passwd
SRCS=	des_rw.c krb_passwd.c local_passwd.c passwd.c pw_copy.c pw_util.c
DPADD=	${LIBKRB} ${LIBDES}
CFLAGS+=-I${.CURDIR} -DKERBEROS -DCRYPT
LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555

.PATH:	${.CURDIR}/../../../usr.bin/passwd ${.CURDIR}/../../../usr.bin/chpass \
	${.CURDIR}/../../../usr.sbin/vipw ${.CURDIR}/../../../usr.bin/rlogin

.include <bsd.prog.mk>
