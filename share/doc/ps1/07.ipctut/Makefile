#	@(#)Makefile	6.5 (Berkeley) 1/2/91

DIR=	ps1/07.ipctut
SRCS=	tutor.me
MACROS=	-me
EXTRA=	accept.grn dgramread.c dgramsend.c gremlins pipe.c pipe.grn \
	socketpair.c socketpair.grn strchkread.c streamread.c streamwrite.c \
	udgramread.c udgramsend.c ustreamread.c ustreamwrite.c

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS} ${EXTRA}
	${TBL} ${SRCS} | ${GREMLIN} | ${ROFF} > ${.TARGET}
