#	BSDI	$Id: Makefile,v 1.2 1994/01/14 00:51:30 cp Exp $
CFLAGS+=-I${.CURDIR}/../hdrs
LDFLAGS+=-L${.CURDIR}/../lib/obj
LDFLAGS+=-L${.CURDIR}/../lib
LDADD+=	-lutil -ltermlib
PROG=	filter
NOMAN=	noman
SRCS=	actions.c	\
	audit.c		\
	filter.c	\
	lock.c		\
	parse.c		\
	regexp.c	\
	rules.c		\
	summarize.c	\
	utils.c

.include <../../Makefile.inc>
.include <bsd.prog.mk>
