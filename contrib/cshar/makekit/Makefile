PROG=	makekit
SRCS=	makekit.c glue.c parser.c lcwd.c lexec.c lfiles.c lhost.c llib.c \
	lmem.c luser.c
CFLAGS+=-I${.CURDIR}/../cshar -fwritable-strings

.PATH:	${.CURDIR}/../cshar

.include "../../Makefile.inc"
.include <bsd.prog.mk>
