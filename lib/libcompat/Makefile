#	BSDI $Id: Makefile,v 1.6 1994/01/12 23:38:06 donn Exp $

LIB=compat
SRCS=   cfree.c cuserid.c doprnt.c doscan.c ecvt.c fcvt.c fgetline.c \
	ftime.c gcvt.c insque.c regcompat.c stty.c

CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../libc/stdio

.include <bsd.lib.mk>
