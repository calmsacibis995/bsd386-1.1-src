#	BSDI $Id: Makefile.inc,v 1.5 1994/01/31 01:25:21 donn Exp $

.ifndef (Mk.Inc)
Mk.Inc?=defined

DATE!= date +%y/%m/%d

CONFIG?=	-DFCNTL -DUSE_TERMIOS -DBSDTCP -DHAYES -DHAYESV -DNFS '-DVERSION="\"BSDI 1.1 ${DATE}\""' -DSAFE_LIBC_INET_NTOA
INCLUDE?=	-I. -I${.CURDIR}/../i -I${.CURDIR}/../args
CFLAGS=		-O -DANSI_C ${INCLUDE} ${CONFIG}

BINGRP=		uucp
BINOWN=		uucp

CLEANFILES+=	*.0

# Make sure that these match their definition in i/site.h
LOGDIR?=	/var/log/uucp
PARAMSFILE?=	/etc/uucp/CONFIG
PARAMSDIR?=	/etc/uucp
PUBDIR?=	/var/spool/uucppublic

.if defined(PROG)
LDADD+=         ../args/libargs.a ../libacu/libacu.a ../lib/lib.a \
	../libc/libc.a
.endif
                
.endif
