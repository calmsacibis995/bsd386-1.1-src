#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/04 09:03:18 donn Exp $

PROG=	makeinfo
SRCS=   getopt.c getopt1.c makeinfo.c
CFLAGS+=-DSTDC_HEADERS -DHAVE_UNISTD_H -DHAVE_STRING_H -DHAVE_SYS_TIME_H \
	-DHAVE_SETVBUF -DHAVE_GETCWD -DHAVE_VFPRINTF -DHAVE_VSPRINTF \
	-DNAMED_FUNCTIONS \
	-DDEFAULT_INFOPATH='"/usr/contrib/lib/info:/usr/contrib/lib/emacs/info:."'
NOMAN=	noman

CFLAGS+=-I${.CURDIR}/../libtxi
.PATH: ${.CURDIR}/../libtxi

.include "../../Makefile.inc"
.include <bsd.prog.mk>
