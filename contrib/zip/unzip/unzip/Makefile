PROG=	unzip
CFLAGS=	-Dunix -DBSD
SRCS=	envargs.c explode.c extract.c file_io.c inflate.c \
	mapname.c match.c misc.c unreduce.c unshrink.c unzip.c
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

.include "../../../Makefile.inc"
.include <bsd.prog.mk>
