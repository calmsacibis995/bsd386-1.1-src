#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:19 polk Exp $

PROG=	sox
SRCS=	raw.c voc.c au.c sf.c aiff.c hcom.c 8svx.c sndrtool.c wav.c sbdsp.c \
	copy.c avg.c pred.c stat.c vibro.c echo.c rate.c band.c lowp.c \
	sox.c misc.c handlers.c libst.c
LDADD+= -lm
DPADD+= ${LIBM}

.include "../../Makefile.inc"

.include <bsd.prog.mk>
