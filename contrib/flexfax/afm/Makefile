#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:03:13 torek Exp $

FILES=  AvantGarde-Book AvantGarde-BookOblique AvantGarde-Demi \
	AvantGarde-DemiOblique Bookman-Demi Bookman-DemiItalic \
	Bookman-Light Bookman-LightItalic Courier-Bold Courier-BoldOblique \
	Courier-Oblique Courier Helvetica-Bold Helvetica-BoldOblique \
	Helvetica-Narrow-Bold Helvetica-Narrow-BoldOblique \
	Helvetica-Narrow-Oblique Helvetica-Narrow Helvetica-Oblique \
	Helvetica NewCenturySchlbk-Bold NewCenturySchlbk-BoldItalic \
	NewCenturySchlbk-Italic NewCenturySchlbk-Roman Palatino-Bold \
	Palatino-BoldItalic Palatino-Italic Palatino-Oblique \
	Palatino-Roman Symbol Times-Bold Times-BoldItalic Times-Italic \
	Times-Oblique Times-Roman ZapfChancery-MediumItalic \
	ZapfDingbats

all clean cleandir depend obj:

install:
	for i in ${FILES}; do \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i.afm \
		${DESTDIR}${AFMDIR}/$$i; \
	done

.include <bsd.prog.mk>
