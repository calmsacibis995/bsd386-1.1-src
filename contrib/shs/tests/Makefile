#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 22:42:34 polk Exp $

CLEANFILES+=file1 file2 file3 shs.data shs.ddata

all: file1 file2 file3 shs.data shs.ddata

tags depend:

install:
	install -o ${BINOWN} -g ${BINGRP} -m 0444 \
		file1 file2 shs.data ${DESTDIR}/usr/contrib/lib/shs/

shstest: file1 file2 file3 shs.data shs.ddata Makefile
	@echo This shs test should produce no output
	@${.CURDIR}/../obj/shs -x | diff - shs.data
	@${.CURDIR}/../obj/shs shs.data | \
	    sed '/^2dd09d381d4c3b73b4857ca4cce5c6f1b492c2f6 shs.data$$/d'
	@${.CURDIR}/../obj/shs -q -c file3 | \
	    sed '/^0xa9f2c8b901a7c8628133694a105fc760bdccad2c$$/d'
	@${.CURDIR}/../obj/shs -p abc -c file2 | \
	    sed '/^0xa9f2c8b901a7c8628133694a105fc760bdccad2c file2$$/d'
	@${.CURDIR}/../obj/shs -P file1 file2 | \
	    sed '/^a9f2c8b901a7c8628133694a105fc760bdccad2c file2$$/d'
	@${.CURDIR}/../obj/shs shs.ddata | \
	    sed '/^7f5fc1a68572ef19c29dab020eadce290633772b shs.ddata$$/d'
	@${.CURDIR}/../obj/shs -d -x -q | diff - shs.ddata

file1: Makefile
	rm -f file1
	echo abc | tr -d '\012' > file1

file2: ${.CURDIR}/../../md5/tests/file2.src
	cp ${.CURDIR}/../../md5/tests/file2.src file2

file3: file1 file2
	rm -f file3
	cat file1 file2 > file3

shs.data: shs.data.src
	cp ${.CURDIR}/shs.data.src shs.data

shs.ddata: shs.ddata.src
	cp ${.CURDIR}/shs.ddata.src shs.ddata

.include <bsd.prog.mk>
