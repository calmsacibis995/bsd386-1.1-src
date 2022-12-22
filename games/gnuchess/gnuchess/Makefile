#	BSDI $Id: Makefile,v 1.1.1.1 1994/01/26 19:30:13 donn Exp $

PROG=	gnuchess
SRCS=	ataks.c book.c dspcom.c eval.c genmoves.c init.c main.c \
	nondsp.c search.c util.c
CLEANFILES+=gnuchess.bk1 gnuchess.data
CC=	gcc2
CFLAGS+=-DBINBOOK=\"${LIBDIR}/gnuchess.data\" -DCACHE -DGDX \
	-DHASGETTIMEOFDAY -DHASHFILE=\"/var/games/gnuchess.hash\" \
	-DKILLT -DLANGFILE=\"${LIBDIR}/gnuchess.lang\" \
	-DNONDSP -DNULLMOVE -DOLDTIME -DQUIETBACKGROUND -DSEMIQUIETBOOKGEN \
	-O2 -funroll-loops
DPADD+=	${LIBM}
LDADD+=	-lm
MAN6=	gnuchess.0
MLINKS+=gnuchess.6 gnuchessn.6
HIDEGAME=hidegame

GZCAT=	/usr/contrib/bin/gzcat
#BOOKLEVEL=-S 320000 -P 24
#BOOKLEVEL=-S 45000 -P 16
#BOOKLEVEL=-S 20000 -P 12
BOOKLEVEL=-S 12000 -P 10
#BOOKLEVEL=-S 9000 -P 8

# We create gnuchess.data here because gnuchess refuses to run
# if ${LIBDIR}/gnuchess.lang doesn't exist.  Sigh.
# Hence this only works if DESTDIR is null or you've installed locally once.
afterinstall:
	install -c -m 0444 -o ${BINOWN} -g ${BINGRP} ${.CURDIR}/gnuchess.lang \
		${DESTDIR}${LIBDIR}/gnuchess.lang
	-rm -f gnuchess.data gnuchess.bk1
	${GZCAT} ${.CURDIR}/gnuchess.bk1.gz > gnuchess.bk1
	(echo 3 0; echo quit) | \
		./gnuchess -b gnuchess.bk1 -B gnuchess.data ${BOOKLEVEL}
	rm -f gnuchess.bk1
	install -m 0444 -o ${BINOWN} -g ${BINGRP} gnuchess.data \
		${DESTDIR}${LIBDIR}/gnuchess.data

# XXX this only works when DESTDIR = ""
# This creates a huge file on /var; check for space first
gnuchess.hash: gnuchess
	-rm -f ${DESTDIR}/var/games/gnuchess.hash
	touch ${DESTDIR}/var/games/gnuchess.hash
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/var/games/gnuchess.hash
	chmod 0666 ${DESTDIR}/var/games/gnuchess.hash
	./gnuchess -c 22

.include "../../Makefile.inc"
.include <bsd.prog.mk>
