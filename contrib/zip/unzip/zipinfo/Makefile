PROG=	zipinfo
CFLAGS=	-DZIPINFO -Dunix -DBSD -I${.CURDIR}/../unzip
SRCS=	zipinfo.c envargs.c match.c misc.c

.PATH:	${.CURDIR}/../unzip

.include "../../../Makefile.inc"
.include <bsd.prog.mk>
