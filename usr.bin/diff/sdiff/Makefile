#	BSDI:	$Id: Makefile,v 1.1.1.1 1993/12/21 04:22:14 polk Exp $

.include "../../Makefile.inc"

PROG=	sdiff
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -I${.CURDIR}/../diff
SRCS= 	sdiff.c getopt.c getopt1.c version.c

.PATH:	${.CURDIR}/../diff

.include <bsd.prog.mk>
