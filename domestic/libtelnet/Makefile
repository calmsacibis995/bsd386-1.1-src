#	BSDI $Id: Makefile,v 1.3 1994/01/08 18:26:42 donn Exp $
#
#	@(#)Makefile	5.4 (Berkeley) 5/7/91

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c gettytab.c misc.c
SRCS+=	kerberos.c enc_des.c
#SRCS+=	kerberos5.c
CFLAGS+=-DENCRYPTION -DAUTHENTICATION
CFLAGS+=-DKRB4 -DDES_ENCRYPTION -I/usr/include/kerberosIV
NOPROFILE=noprofile

.PATH:  ${.CURDIR}/../../libexec/getty
.include <bsd.lib.mk>

