#	@(#)Makefile	5.14 (Berkeley) 6/25/90

# USERMEM should be the amount of available user memory in bytes.
# Set it to zero, for physical memory less than 1 Meg.
USERMEM=2097152

PROG=	compress
SRCS=	compress.c
NOMAN=noman
CFLAGS+=-DBSD4_2 -DSACREDMEM=256000 -DUSERMEM=${USERMEM}
LDFLAGS+=-N

.PATH:	/usr/src/usr.bin/compress

.include <bsd.prog.mk>
