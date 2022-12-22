#	BSDI $Id: Makefile,v 1.1.1.1 1992/12/02 18:07:40 polk Exp $
#	from @(#)Makefile	5.1 (Berkeley) 5/3/91

PROG=	tar
SRCS=	tar.c
SRCS=	buffer.c create.c extract.c list.c mangle.c names.c port.c tar.c
CFLAGS+=-DDEF_AR_FILE=\"/dev/rmt8\"
NOMAN=	noman
LDFLAGS=-N
.PATH:	/usr/src/bin/tar

.include <bsd.prog.mk>
