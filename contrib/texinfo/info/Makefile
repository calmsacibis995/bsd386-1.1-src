#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/04 09:03:21 donn Exp $

PROG=	info
SRCS=   dir.c display.c doc.c dribble.c echo_area.c filesys.c \
	footnotes.c gc.c getopt.c getopt1.c indices.c info-utils.c \
	info.c infodoc.c infomap.c m-x.c nodemenu.c nodes.c search.c \
	session.c signals.c terminal.c tilde.c variables.c window.c \
	xmalloc.c
CFLAGS+=-DSTDC_HEADERS -DHAVE_UNISTD_H -DHAVE_STRING_H -DHAVE_SYS_TIME_H \
	-DHAVE_SETVBUF -DHAVE_GETCWD -DHAVE_VFPRINTF -DHAVE_VSPRINTF \
	-DNAMED_FUNCTIONS \
	-DDEFAULT_INFOPATH='"/usr/contrib/lib/info:/usr/contrib/lib/emacs/info:."'
LDADD+=	-ltermlib
DPADD+=	${LIBTERM}

CFLAGS+=-I${.CURDIR}/../libtxi
.PATH: ${.CURDIR}/../libtxi

.include "../../Makefile.inc"
.include <bsd.prog.mk>
