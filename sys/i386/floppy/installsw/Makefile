SRCS=installsw.c z.c checkget.c interactive.c
PROG=installsw
.ifnmake depend
CFLAGS+=-DDEF_MOUNTPT=\"/\" -DDEF_ROOT=\"/a\" -DDEF_MEDIA=\"tape\"
CFLAGS+=-D_PATH_UNCOMPRESS=\"gzip\" -DUNCOMPRESS_ARGS=\"-d -c\"
.endif
# LDFLAGS+=-N
NOMAN=noman
LDADD=-lcurses -ltermcap
DPADD=${LIBCURSES} ${LIBTERMCAP}

.PATH:	/usr/src/usr.sbin/installsw

.include <bsd.prog.mk>
