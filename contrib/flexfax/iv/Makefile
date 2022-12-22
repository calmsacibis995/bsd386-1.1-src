#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:04:14 torek Exp $

LIB=	ivhack
SRCS=	dispatcher.c++ iohandler.c++ memory.c++ regexp.c++
.PATH:	${.CURDIR}/Dispatch ${.CURDIR}/OS ${.CURDIR}/InterViews
C++FLAGS+=-I${.CURDIR} -I${.CURDIR}/.. -DSIGCLD=SIGCHLD
MKDEP=	mkdep -2 ${C++FLAGS:M-[ID]*}
NOPROFILE=

install:

.include <bsd.lib.mk>
