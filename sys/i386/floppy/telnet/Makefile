#	@(#)Makefile	5.2 (Berkeley) 5/10/91

PROG=	telnet
SRCS=	authenc.c commands.c main.c network.c ring.c sys_bsd.c telnet.c \
	terminal.c utilities.c
CFLAGS+=-DTERMCAP -DUSE_TERMIO \
	-I/usr/src/lib
LDFLAGS+=-s -N
LDADD=  -ltelnet -ltermcap
DPADD=  ${LIBTELNET} ${LIBTERMCAP}

NOMAN=	noman
.PATH:	/usr/src/usr.bin/telnet

.include <bsd.prog.mk>
