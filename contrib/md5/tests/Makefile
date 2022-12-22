#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/13 22:42:33 polk Exp $

CLEANFILES+=file1 file2 file3 md5.data md5.ddata

all: file1 file2 file3 md5.data md5.ddata

tags depend:

install:
	install -o ${BINOWN} -g ${BINGRP} -m 0444 \
		file1 file2 md5.data ${DESTDIR}/usr/contrib/lib/md5/


md5test: file1 file2 file3 md5.data md5.ddata Makefile
	@echo This md5 test should produce no output
	@${.CURDIR}/../obj/md5 -x | diff - md5.data
	@${.CURDIR}/../obj/md5 md5.data | \
	    sed '/^9638425fd6b565e9fddf642fa4889125 md5.data$$/d'
	@${.CURDIR}/../obj/md5 -q -c file3 | \
	    sed '/^0x68cfe1a8e5242c8e1e8152d0bc710631$$/d'
	@${.CURDIR}/../obj/md5 -p abc -c file2 | \
	    sed '/^0x68cfe1a8e5242c8e1e8152d0bc710631 file2$$/d'
	@${.CURDIR}/../obj/md5 -P file1 file2 | \
	    sed '/^68cfe1a8e5242c8e1e8152d0bc710631 file2$$/d'
	@${.CURDIR}/../obj/md5 md5.ddata | \
	    sed '/^78a3db06b2ea6d77fac09aa8b8afe959 md5.ddata$$/d'
	@${.CURDIR}/../obj/md5 -d -x -q | diff - md5.ddata

file1: Makefile
	rm -f file1
	echo abc | tr -d '\012' > file1

file2: file2.src
	cp ${.CURDIR}/file2.src file2

file3: file1 file2
	rm -f file3
	cat file1 file2 > file3

md5.data: md5.data.src
	cp ${.CURDIR}/md5.data.src md5.data

md5.ddata: md5.ddata.src
	cp ${.CURDIR}/md5.ddata.src md5.ddata

.include <bsd.prog.mk>
