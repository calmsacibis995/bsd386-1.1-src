#	BSDI	$Id: Makefile,v 1.2 1994/02/01 15:33:17 polk Exp $

PROG=	xmix
CFLAGS+=-I/usr/X11/include 
LDFLAGS+=-L/usr/X11/lib
LDADD+=	-lXaw -lXmu -lXext  -lXt -lX11

BINDIR?=/usr/X11/bin
MANDIR?=/usr/X11/man/cat

.include <bsd.prog.mk>
