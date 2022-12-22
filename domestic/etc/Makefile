#	@(#)Makefile	5.11 (Berkeley) 5/21/91

NOOBJ=	oobj

# -rw-r--r--
BIN1=	inetd.conf	

all clean cleandir depend etc install lint:

distribution:
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} ${DESTDIR}/etc

.include <bsd.prog.mk>
