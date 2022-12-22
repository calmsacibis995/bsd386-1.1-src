#	BSDI	$Id: Makefile,v 1.2 1994/01/05 20:26:29 polk Exp $

obj objdir tags depend all:

install:
	cp -r ${.CURDIR}/[a-z]* ${DESTDIR}/usr/contrib/lib/tk/
	chown -R ${BINOWN}.${BINGRP} ${DESTDIR}/usr/contrib/lib/tk/*
	chmod a-w ${DESTDIR}/usr/contrib/lib/tk/*

.include <bsd.prog.mk>
