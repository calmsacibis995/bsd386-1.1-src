#	BSDI $Id: Makefile,v 1.3 1993/02/05 21:02:45 polk Exp $
#	@(#)Makefile	5.16 (Berkeley) 4/26/91

PROG=	routed
SRCS=	af.c if.c input.c main.c output.c startup.c tables.c timer.c \
	trace.c inet.c
MAN8=	routed.0
# SUBDIR=	query trace
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>

.if (${MACHINE} == "vax")
# The following can be deleted where not appropriate to use the kernel's
# inline code expansions.
INLINE=	/sys/vax/inline/obj/inline
C2=	/usr/libexec/c2
.c.o:
	${CC} -S ${CFLAGS} ${.CURDIR}/${.PREFIX}.c
	@${C2} ${.PREFIX}.s | ${INLINE} | ${AS} -o ${.PREFIX}.o
	@rm -f ${.PREFIX}.s
.endif
