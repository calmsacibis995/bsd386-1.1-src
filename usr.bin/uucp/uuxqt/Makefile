#       BSDI $Id: Makefile,v 1.3 1994/02/01 15:03:43 polk Exp $

PROG=   uuxqt
SRCS=   main.c do_uuxqt.c uuxqt_args.c
MAN8=	uuxqt.0
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

BINDIR= /usr/libexec
BINMODE=6110

.include <bsd.prog.mk>
.include <bsd.man.mk>
