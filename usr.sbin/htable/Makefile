#	@(#)Makefile	5.12 (Berkeley) 7/1/90

PROG=	htable
SRCS=	htable.c parse.c scan.c
MAN8=	htable.0
CFLAGS+=-I. -I${.CURDIR}
DPADD+=	${LIBL}
LDADD+=	-ll
CLEANFILES+=parse.c scan.c y.tab.h

.include <bsd.prog.mk>

${OBJS}: scan.c parse.c
