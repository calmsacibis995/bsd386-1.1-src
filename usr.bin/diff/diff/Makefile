#	BSDI:	$Id: Makefile,v 1.2 1993/12/30 01:22:09 polk Exp $

.include "../../Makefile.inc"

PROG=	diff
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}
SRCS= 	analyze.c cmpbuf.c context.c diff.c dir.c ed.c getopt.c \
	getopt1.c ifdef.c io.c normal.c regex.c side.c util.c version.c

.include <bsd.prog.mk>
