PROG=build
NOMAN=1

CFLAGS += -DRETSIGTYPE=void -DHAVE_TERMIOS_H=1 -DHAVE_STRING_H=1 \
	-DDICT_LIB=\"/usr/libdata\" -I${.CURDIR}/../ispell

SRCS= access.c build.c charset.c ctbl.c downcase.c exp.c hash.c lexfsm.c \
	mklextbl.c nextprime.c suf.c util.c

.PATH: ${.CURDIR}/../ispell

all: ispell.dict

freq: freq.o util.o charset.o
	$(CC) $(CFLAGS) -o freq freq.o util.o charset.o
CLEANFILES += freq.o freq

ispell.dict: build freq
	./freq < ${.CURDIR}/dict > freqtbl
	./build -b -f freqtbl -o ispell.dict ${.CURDIR}/dict
CLEANFILES += freqtbl ispell.dict

install: ispell.dict
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		ispell.dict ${DESTDIR}/usr/libdata/ispell.dict
	
.include <bsd.prog.mk>
