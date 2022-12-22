#
#	BSDI $Id: Makefile,v 1.2 1993/03/03 22:15:43 sanders Exp $
#	changed to a BSD style makefile
#
#	@(#)Makefile	6.1 (Berkeley) 5/26/86
#
#	Makefile for the notesfile reference manual
#	March 29, 1982
#	Ray Essick, University of Illinois
#	uiucdcs!essick     uiucdcs!notes
#
#	Note that the macros are set up for two-sided copying.
#	The page numbers alternate between the left and right 
#	sides of the page.
#	See the file "macros" and the macro "hh" to modify this
#
#	This copy of everything has been modified to run with troff
#	and a typesetter. Hopefully the only changes to make it
#	run again under nroff will be in page length/width statements.
#
#

DIR=	usd/11.notes
SRCS=	0.long 0.short \
	1 2.1 2.2 3.1 3.2 \
	4.0 4.1 4.2 4.3 4.4 4.5 4.6 4.7 \
	5 \
	a.checklist b.interface c.changes
LONG=	0.long \
	1 2.1 2.2 3.1 3.2 4.0 4.1 4.2 4.3 4.4 4.5 4.6 4.7 \
	5 a.checklist b.interface c.changes
SHORT=	0.short \
	1 2.1 2.2 4.0 4.1 4.2 4.3 4.4 4.5
EXTRAS=	Makefile macros vtoc acks

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${LONG} macros vtoc acks
	${ROFF} macros ${LONG} vtoc acks > ${.TARGET}

short.${PRINTER}: ${SHORT} macros vtoc
	${ROFF} macros ${SHORT} vtoc > ${.TARGET}
