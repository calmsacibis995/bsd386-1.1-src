#	BSDI $Id: Makefile,v 1.3 1994/01/03 20:41:28 polk Exp $
#
#	@(#)Makefile	5.16 (Berkeley) 5/13/91

PROG=	telnetd
CFLAGS+=-DLINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DENV_HACK -DOLD_ENVIRON
.ifnmake(depend)
CFLAGS+=-DDEFAULT_IM="\"\r\nBSDI BSD/386 1.1 (%h) (%t)\r\n\r\r\n\r\""
.endif
CFLAGS+=-DENCRYPTION -DAUTHENTICATION
CFLAGS+=-I${.CURDIR}/../..
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
LDADD=	-lutil -ltermcap -ltelnet
LDADD+=	-lkrb -ldes
DPADD=	${LIBUTIL} ${LIBTERM}
MAN8=	telnetd.0

.PATH:	${.CURDIR}/../../../libexec/telnetd

.include <bsd.prog.mk>
