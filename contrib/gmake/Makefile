PROG=	gmake
SRCS=	alloca.c ar.c arscan.c commands.c default.c dir.c expand.c \
	file.c function.c getopt.c getopt1.c \
	glob.c implicit.c job.c main.c misc.c read.c remake.c \
	remote-stub.c rule.c signame.c variable.c version.c vpath.c
CFLAGS+=-I${.CURDIR} -DHAVE_CONFIG_H -DHAVE_GLOB_H
CFLAGS+=-DLIBDIR=\"/usr/contrib/lib/gmake\" 
CFLAGS+=-DINCLUDEDIR=\"/usr/contrib/lib/gmake\"
LDADD+=	-lutil -lkvm
DPADD+=	${LIBUTIL} ${LIBKVM}

.include <bsd.prog.mk>
