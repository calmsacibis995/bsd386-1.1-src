#	BSDI	$Id: Makefile,v 1.3 1994/01/14 01:35:12 cp Exp $
CFLAGS+=-I${.CURDIR}/../hdrs
LDFLAGS+=-L${.CURDIR}/../lib/obj
LDFLAGS+=-L${.CURDIR}/../lib
LDADD+=	-lutil -ltermlib
NOPROG=	noprog
NOMAN=	noman
SRCS=	answer.c		\
	arepdaem.c 		\
	autoreply.c		\
	elmalias.c		\
	fastmail.c		\
	from.c			\
	newalias.c		\
	newmail.c		\
	prlong.c		\
	readmsg.c
CLEANFILES+= answer answer.o arepdaem arepdaem.o autoreply autoreply.o
CLEANFILES+= elmalias.o fastmail fastmail.o from from.o listalias.o newalias
CLEANFILES+= newalias.o newmail newmail.o prlong.o readmsg readmsg.o
CMDS=	answer arepdaem autoreply fastmail from newalias newmail readmsg

all: $(CMDS)

answer:	answer.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} answer.o ${LDADD}
arepdaem: arepdaem.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} arepdaem.o ${LDADD}
autoreply: autoreply.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} autoreply.o ${LDADD}
fastmail: fastmail.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} fastmail.o ${LDADD}
from: from.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} from.o ${LDADD}
newalias: newalias.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} newalias.o ${LDADD}
newmail: newmail.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} newmail.o ${LDADD}
readmsg: readmsg.o ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} -o ${.TARGET} readmsg.o ${LDADD}

afterinstall:
	for i in $(CMDS); do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
			$$i ${DESTDIR}${BINDIR}; \
	done
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/checkalias ${DESTDIR}${BINDIR}
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/messages ${DESTDIR}${BINDIR}
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/printmail ${DESTDIR}${BINDIR}
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/mailrc.awk ${DESTDIR}${BINDIR}
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.CURDIR}/listalias ${DESTDIR}${BINDIR}

.include <../../Makefile.inc>
.include <bsd.prog.mk>
