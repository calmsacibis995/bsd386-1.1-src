#	@(#)Makefile	6.4 (Berkeley) 6/30/90

DIR=	ps1/18.curses
SRCS=	Master macros appen.A appen.B appen.C intro.0 intro.1 \
	intro.2 intro.3 intro.4 intro.5
MACROS=	-me

CLEANFILES+=win_st.gr twinkle1.gr twinkle2.gr life.gr intro.2.tbl appen.A.tbl

all:	fmt

.SUFFIXES:
.SUFFIXES: .c .gr

#
# this section formats C input source into nice troffable (or nroffable)
# versions.  It uses the capabilites of "vgrind", which sets keywords in
# bold font, and comments in italics.
#

# Don't re-run vgrind unless you want to patch the output files.
VFONT=	/usr/libexec/vfontedpr
.c.gr:
	${VFONT} $*.c | grep -v "^'wh" > $*.gr

intro.5: doc.I doc.II doc.III doc.IV
appen.B: win_st.gr
appen.C: twinkle1.gr life.gr twinkle2.gr

intro.2.tbl: intro.2
	${TBL} intro.2 > intro.2.tbl

appen.A.tbl: appen.A
	${TBL} appen.A > appen.A.tbl

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
