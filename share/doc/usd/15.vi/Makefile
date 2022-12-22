#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	usd/15.vi
SRCS=	vi.in vi.chars
MACROS=	-ms
CLEANFILES+=summary.* viapwh.*

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER} summary.${PRINTER} viapwh.${PRINTER}

paper.${PRINTER}: ${SRCS} summary.${PRINTER} viapwh.${PRINTER}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

summary.${PRINTER}: vi.summary
	${TBL} vi.summary | ${ROFF}  > ${.TARGET}

viapwh.${PRINTER}: vi.apwh.ms
	${ROFF} vi.viapwh > ${.TARGET}
