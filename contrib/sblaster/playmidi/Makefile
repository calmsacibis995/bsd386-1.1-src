#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:25 polk Exp $

PROG=	playmidi
SRCS=	playmidi.c midifile.c getopt.c getopt1.c
CFLAGS+=-I/sys -I${.CURDIR}/../cdsp

.PATH: ${.CURDIR}/../cdsp

.include "../../Makefile.inc"
.include <bsd.prog.mk>
