#	BSDI $Id: Makefile,v 1.6 1993/12/22 21:26:09 polk Exp $
#
#	@(#)Makefile	5.16 (Berkeley) 5/13/91

PROG=	telnetd

CFLAGS+=-DLINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DENV_HACK -DOLD_ENVIRON
.ifnmake(depend)
CFLAGS+=-DDEFAULT_IM="\"\r\nBSDI BSD/386 1.1 (%h) (%t)\r\n\r\r\n\r\""
.endif
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c gettytab.c
DPADD=	${LIBUTIL} ${LIBTERM}
LDADD=	-lutil -ltermcap
MAN8=	telnetd.0
.PATH:	${.CURDIR}/../getty

.include <bsd.prog.mk>
