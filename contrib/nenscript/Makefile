#
# @(#) $Id: Makefile,v 1.1.1.1 1993/02/10 18:05:36 sanders Exp $
# BSD/386 Makefile for nenscript
#
#
# US_DATE selects date of form MMM DD YY, else date is DD MMM YY
#
# CFLAGS= -g
CFLAGS+=-traditional
#CFLAGS+=-DA4_PAPER $(DEBUG)
CFLAGS+=-DUS_DATE -DUS_PAPER $(DEBUG)

PROG=	nenscript
SRCS=	main.c postscri.c print.c fontwidt.c font_lis.c
MAN1=	$(PROG).0

.include <bsd.prog.mk>
