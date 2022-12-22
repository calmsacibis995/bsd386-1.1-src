#	BSDI	$Id: Makefile,v 1.2 1994/01/14 23:22:03 polk Exp $
#	Krystal $Id: Makefile,v 1.2 1994/01/14 23:22:03 polk Exp $

PROG=	doscmd
SRCS=	doscmd.c trap.c exe.c mem.c i386-pinsn.c tty.c trace.c port.c \
	cmos.c cwd.c bios.c config.c disktab.c device.c setver.c \
	int10.c int11.c int12.c int13.c int14.c int15.c int16.c int17.c \
	int1a.c int20.c int21.c int2f.c int33.c intff.c AsyncIO.c \
	ParseBuffer.c
CFLAGS+=-I/usr/X11/include -I. -DDISASSEMBLER -DVER11
LDFLAGS+=-L/usr/X11/lib
LDADD+=	-lX11 -lc -lgnulib
DPADD+=	/usr/X11/lib/libX11.a ${LIBC} /usr/lib/libgnulib.a

afterinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		doscmd.kernel ${DESTDIR}/usr/libexec/doscmd.kernel
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/instbsdi.exe ${DESTDIR}/usr/dos/

doscmd: doscmd.kernel doscmd_loader.o
	ld -N -o doscmd ${LDFLAGS} /usr/lib/crt0.o doscmd_loader.o ${LDADD}

.include <bsd.prog.mk>

.depend: doscmd_loader.c

doscmd.kernel: crt0.o ${OBJS}
	ld -N -T 110000 -o doscmd.kernel ${LDFLAGS} crt0.o ${OBJS} ${LDADD}

doscmd.0: doscmd.1
	tbl ${.IMPSRC} | nroff -mandoc > ${.TARGET}
