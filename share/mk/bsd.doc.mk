#	BSDI $Id: bsd.doc.mk,v 1.5 1994/01/13 08:01:00 torek Exp $

#	@(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91

PRINTER=ps

EQN?=		eqn -C -T${PRINTER}
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic -C
REFER?=		refer
ROFF?=		groff -T${PRINTER} -m${PRINTER} ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl -C

# These aren't currently available
BIB?=		bib
GREMLIN?=	grn -P${PRINTER}

.PATH: ${.CURDIR}

.if !target(print)
print: paper.${PRINTER}
	lpr -P${PRINTER} paper.${PRINTER}
.endif

clean cleandir:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

FILES?=	${SRCS}
install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${BINDIR}/${DIR}

spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell
	
.if !target(obj)
obj:
.endif

.if !target(objdir)
objdir:
.endif

.if !target(depend)
depend:
.endif

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444
