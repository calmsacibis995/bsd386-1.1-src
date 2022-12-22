#	BSDI $Id: Makefile,v 1.3 1994/01/12 09:59:37 donn Exp $

PROG=	awk
SRCS=	main.c eval.c builtin.c msg.c iop.c io.c field.c getopt1.c \
	getopt.c array.c \
	node.c version.c re.c awk.y regex.c dfa.c
DPADD=	${LIBM}
LDADD=	-lm
YFLAGS=
CFLAGS+=-DGAWK

.include <bsd.prog.mk>
