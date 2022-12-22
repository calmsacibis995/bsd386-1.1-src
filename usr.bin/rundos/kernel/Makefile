PROG=	kernel
SRCS=	kernel.c emulate.c service.c config.c queue.c \
	keyboard.c console.c interrupt.c timer.c cmos.c trace.c int13.c \
	video.c menu.c printer.c mouse.c xms.c input.c
NOMAN=noman

.include "../Makefile.inc"

kernel: crt0.o $(OBJS)
	ld -N -T 110000 -o kernel crt0.o $(OBJS) -lc -lgnulib

install:
	install -c -s kernel ${DESTDIR}${KERNEL_FILE_NAME}

.include <bsd.prog.mk>
