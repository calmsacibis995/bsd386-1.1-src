PROG=	disksetup
SRCS=	disksetup.c dkcksum.c checkget.c z.c interactive.c
LDFLAGS+=-N
NOMAN=noman
LDADD=-lcurses -ltermcap -lutil
DPADD=${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}

.PATH:	/usr/src/sbin/disksetup

.include <bsd.prog.mk>
