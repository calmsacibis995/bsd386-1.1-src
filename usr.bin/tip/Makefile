#	BSDI $Id: Makefile,v 1.6 1993/12/18 23:21:30 sanders Exp $
#	@(#)Makefile	5.9 (Berkeley) 5/11/90
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				mode 6??
#	/var/log/aculog		ACU accounting file, owned by ${OWNER} and
#				mode 6?? {if ACULOG defined}
#
# Dialers presently supported:
#	HAYES and Hayes-compatible
#	USR COURIER (2400 baud)
#
# Configuration defines:
#	ACULOG			turn on tip logging of ACU use
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
#	CONNECT			enables ~C for running external protocols
#	COURIER			ACU -- USR COURIER (2400 baud)
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	HAYES			ACU -- HAYES and Hayes-compatible
#	PRISTINE		no phone #'s put in ACU log file

PROG=	tip
DRIVERDEFS=	-DHAYES -DCOURIER -DCONNECT
DRIVERS=	hayes.c courier.c
CFLAGS+=-I${.CURDIR} ${DRIVERDEFS} -DDEFBR=1200 \
	-DDEFFS=BUFSIZ -DACULOG -DPRISTINE -fwritable-strings
.PATH:	${.CURDIR}/aculib
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINOWN=	uucp
BINGRP=	dialer
BINMODE=4510
LINKS=	${BINDIR}/tip ${BINDIR}/cu
MLINKS=	tip.1 cu.1
SRCS=	acu.c acutab.c cmds.c cmdtab.c cu.c hunt.c log.c partab.c remcap.c \
	remote.c tip.c tipout.c uucplock.c value.c vars.c termctl.c ${DRIVERS}

# -- acutab is configuration dependent, and so depends on the Makefile
# -- remote.o depends on the Makefile because of DEFBR and DEFFS
# -- log.o depends on the Makefile because of ACULOG
acutab.o log.o remote.o: Makefile

.include <bsd.prog.mk>
