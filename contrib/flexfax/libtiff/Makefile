#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:17 torek Exp $

LIB=	tiff
SRCS=	tif_aux.c tif_ccittrle.c tif_close.c tif_compress.c tif_dir.c \
	tif_dirinfo.c tif_dirread.c tif_dirwrite.c tif_dumpmode.c \
	tif_error.c tif_fax3.c tif_fax4.c tif_flush.c tif_getimage.c \
	tif_jpeg.c tif_lzw.c tif_next.c tif_open.c tif_packbits.c \
	tif_print.c tif_read.c tif_strip.c tif_swab.c tif_thunder.c \
	tif_tile.c tif_unix.c tif_version.c tif_warning.c tif_write.c
CFLAGS+=-I. -I${.CURDIR}
NOPROFILE=noprofile
CLEANFILES+=mkg3states mkg3states.o g3states.h

mkg3states: mkg3states.o
	${CC} -o mkg3states mkg3states.o

g3states.h: mkg3states
	./mkg3states -c > ${.TARGET}

depend: g3states.h

install:

.include <bsd.lib.mk>
