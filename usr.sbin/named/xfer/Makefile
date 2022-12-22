#	@(#)Makefile	5.1 (Berkeley) 5/28/90

PROG=	named-xfer
SRCS=	named-xfer.c db_glue.c
CFLAGS+=-I${.CURDIR}/.. -Ddn_skipname=__dn_skipname -Dp_type=__p_type \
	-Dp_class=__p_class
.PATH:	${.CURDIR}/..
BINDIR=	/usr/libexec
NOMAN=	noman

.include "../../Makefile.inc"
.include <bsd.prog.mk>
