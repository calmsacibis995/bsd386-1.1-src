#	@(#)Makefile	5.1 (Berkeley) 5/8/91

LIB=	sa
STAND=	../../../stand
INCPATH=-I../../../sys -I../../..
CFLAGS=	-DSTANDALONE -DAT386 -O ${INCPATH}
SRCS=	bmap.c close.c dev.c getfile.c gets.c ioctl.c lseek.c open.c \
	printf.c read.c stat.c strerror.c strtol.c sys.c write.c
.PATH:	${STAND}
NOPROFILE=noprofile

.include <bsd.lib.mk>

