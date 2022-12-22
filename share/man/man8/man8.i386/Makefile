#	BSDI $Id: Makefile,v 1.2 1993/03/08 16:12:46 polk Exp $
#	@(#)Makefile	5.1 (Berkeley) 3/22/91

MAN8=	MAKEDEV.0 boot.0
MANSUBDIR=/i386

all: ${MAN8}

clean depend lint tags:

cleandir:
	rm -f ${MAN8}

install: maninstall

.include <bsd.prog.mk>
