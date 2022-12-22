#	BSDI $Id: Makefile,v 1.5 1994/01/15 07:13:00 polk Exp $

.include "../../Makefile.inc"

BINDIR=/usr/contrib/lib
PROG=	elvprsv
CFLAGS+=	-I${.CURDIR}/../elvis -Dbsd
SRCS=	elvprsv.c 
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
