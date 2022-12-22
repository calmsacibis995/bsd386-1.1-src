#	@(#)Makefile	5.6 (Berkeley) 6/27/91

PROG=	route
NOMAN=	noman
SRCS=	route.c ccitt_addr.c
CFLAGS+=-I.
CLEANFILES+=keywords.h
BINOWN=	root
BINMODE=4555
LDFLAGS+=-N

.PATH:	/usr/src/sbin/route

all route depend lint tags: keywords.h

keywords.h: keywords
	sed -e '/^#/d' -e '/^$$/d' /usr/src/sbin/route/keywords > _keywords.tmp
	tr a-z A-Z < _keywords.tmp | paste _keywords.tmp - | \
	    awk '{ \
		if (NF > 1) \
			printf "#define\tK_%s\t%d\n\t{\"%s\", K_%s},\n", \
			    $$2, NR, $$1, $$2 }' \
	    > ${.TARGET}
	rm -f _keywords.tmp

.include <bsd.prog.mk>
