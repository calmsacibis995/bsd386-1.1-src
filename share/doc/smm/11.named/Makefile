#	@(#)Makefile	6.4 (Berkeley) 6/30/90

DIR=	smm/11.named
SRCS=	00macs.me 00title.me intro.me build.me types.me setup.me \
	files.me named.boot.primary named.boot.secondary named.boot.cache \
	resolv.conf root.cache named.local ucbhosts ucbhosts.rev manage.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}
