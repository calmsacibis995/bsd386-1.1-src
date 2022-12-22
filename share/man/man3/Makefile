#	BSDI $Id: Makefile,v 1.2 1993/12/18 06:49:59 donn Exp $
#	@(#)Makefile	5.16.1.1 (Berkeley) 5/7/91

MAN3=	assert.0 bitstring.0 end.0 intro.0 queue.0 stdarg.0
MLINKS+=end.3 edata.3 end.3 etext.3
MLINKS+=queue.3 LIST_ENTRY.3 queue.3 LIST_HEAD.3 queue.3 LIST_INIT.3 \
	queue.3 LIST_INSERT_AFTER.3 queue.3 LIST_INSERT_HEAD.3 \
	queue.3 LIST_REMOVE.3 queue.3 TAILQ_ENTRY.3 \
	queue.3 TAILQ_HEAD.3 queue.3 TAILQ_INIT.3 \
	queue.3 TAILQ_INSERT_AFTER.3 queue.3 TAILQ_INSERT_HEAD.3 \
	queue.3 TAILQ_INSERT_TAIL.3 queue.3 TAILQ_REMOVE.3 \
	queue.3 CIRCLEQ_ENTRY.3 queue.3 CIRCLEQ_HEAD.3 \
	queue.3 CIRCLEQ_INIT.3 queue.3 CIRCLEQ_INSERT_AFTER.3 \
	queue.3 CIRCLEQ_INSERT_BEFORE.3 queue.3 CIRCLEQ_INSERT_HEAD.3 \
	queue.3 CIRCLEQ_INSERT_TAIL.3 queue.3 CIRCLEQ_REMOVE.3
MLINKS+=stdarg.3 varargs.3

all: ${MAN3}

clean depend lint tags:

cleandir:
	rm -f ${MAN3}

install: maninstall

.include <bsd.prog.mk>
