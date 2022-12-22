#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:26 polk Exp $

PROG=mixer
SRCS=mixer.c getopt.c getopt1.c
CFLAGS+=-I/sys -I${.CURDIR}/../cdsp
NOMAN=noman

.PATH: ${.CURDIR}/../cdsp

.include "../../Makefile.inc"

.include <bsd.prog.mk>
