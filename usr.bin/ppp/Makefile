#	BSDI $Id: Makefile,v 1.6 1994/01/30 20:59:30 polk Exp $
#
# Files are:
#       /etc/ppp.sys    - systems file
#
# Dialers presently supported:
#	HAYES and Hayes-compatible
#	USR COURIER (2400 baud)
#
# Configuration defines:
#	ACULOG			turn on tip logging of ACU use
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
#	COURIER			ACU -- USR COURIER (2400 baud)
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	HAYES			ACU -- HAYES and Hayes-compatible
#	PRISTINE		no phone #'s put in ACU log file

PROG=   ppp
DRIVERDEFS=	-DHAYES -DCOURIER
DRIVERS=	hayes.c courier.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../tip ${DRIVERDEFS} -fwritable-strings
.PATH:	${.CURDIR}/../tip ${.CURDIR}/../tip/aculib
BINOWN=	root
BINGRP=	netdial
BINMODE=4550
MAN8=	ppp.0
MAN5=	ppp.sys.0
SRCS=   acu.c acutab.c clocal.c hunt.c log.c partab.c ppp.c readsys.c \
	termcap.c uucplock.c ifconfig.c vars.c ${DRIVERS}

# -- acutab is configuration dependent, and so depends on the Makefile
acutab.o: Makefile

.include <bsd.prog.mk>
