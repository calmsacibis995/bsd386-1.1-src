#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/03 20:50:52 polk Exp $

PROG=	tn3270
SRCS=	authenc.c commands.c main.c network.c ring.c sys_bsd.c telnet.c \
	terminal.c tn3270.c utilities.c genget.c
MAN1=	tn3270.0
MAN5=	map3270.0
CFLAGS+=-DTERMCAP -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270
CFLAGS+=-DENV_HACK -DOLD_ENVIRON -I${.CURDIR}

.if exists(${.CURDIR}/../ctlr/obj)
LIBCTLR=${.CURDIR}/../ctlr/obj/libctlr.a
.else
LIBCTLR=${.CURDIR}/../ctlr/libctlr.a
.endif

.if exists(${.CURDIR}/../ascii/obj)
LIBASCII=${.CURDIR}/../ascii/obj/libascii.a
.else
LIBASCII=${.CURDIR}/../ascii/libascii.a
.endif

.if exists(${.CURDIR}/../general/obj)
LIBGENERAL=${.CURDIR}/../general/obj/libgeneral.a
.else
LIBGENERAL=${.CURDIR}/../general/libgeneral.a
.endif

.if exists(${.CURDIR}/../sys_curses/obj)
LIBSYSC=${.CURDIR}/../sys_curses/obj/libsys_curses.a
.else
LIBSYSC=${.CURDIR}/../sys_curses/libsys_curses.a
.endif

.if exists(${.CURDIR}/../api/obj)
LIBAPI=${.CURDIR}/../api/obj/libapi.a
.else
LIBAPI=${.CURDIR}/../api/libapi.a
.endif

LDADD+=	${LIBCTLR} ${LIBASCII} ${LIBGENERAL} ${LIBSYSC} ${LIBAPI}
DPADD+=	${LIBCTLR} ${LIBASCII} ${LIBGENERAL} ${LIBSYSC} ${LIBAPI} 

LDADD+= -lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}

.PATH:	${.CURDIR}/../../telnet

.include "../../Makefile.inc"
.include <bsd.prog.mk>
