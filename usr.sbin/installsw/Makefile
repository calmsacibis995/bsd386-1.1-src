PROG=installsw
SRCS=installsw.c z.c checkget.c interactive.c
MAN8=installsw.0
LDADD=-lcurses -ltermcap
DPADD=${LIBCURSES} ${LIBTERMCAP}
#CFLAGS+=-DDEF_MOUNTPT=\"/\" -DDEF_ROOT=\"/a\" -DDEF_MEDIA=\"cdrom\"

.include <bsd.prog.mk>
