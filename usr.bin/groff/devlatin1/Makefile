#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:28:10 polk Exp $

.include "../Makefile.defs"

DEV=	devlatin1
FONTS=	R I B BI
DEVFILES=${FONTS} DESC

RES=	240
CPI=	10
LPI=	6

CLEANFILES+=${DEVFILES}

tags depend:

all: ${DEVFILES}

${FONTS}: ${.CURDIR}/R.proto
	@echo Making ${.TARGET}
	@(charwidth=`expr ${RES} / ${CPI}` ; \
 	 sed -e "s/^name [A-Z]*$$/name ${.TARGET}/" \
	     -e "s/^\\([^	]*\\)	[0-9]+	/\\1	$$charwidth	/" \
	     -e "s/^spacewidth [0-9]+$$/spacewidth $$charwidth/" \
	     -e "s/^internalname .*$$/internalname $@/" \
	     -e "/^internalname/s/BI/3/" \
	     -e "/^internalname/s/B/2/" \
	     -e "/^internalname/s/I/1/" \
	     -e "/^internalname .*[^ 0-9]/d" \
	     ${.CURDIR}/R.proto >${.TARGET})

DESC: DESC.proto
	@echo Making ${.TARGET}
	@sed -e "s/^res .*$$/res ${RES}/" \
	    -e "s/^hor .*$$/hor `expr ${RES} / ${CPI}`/" \
	    -e "s/^vert .*$$/vert `expr ${RES} / ${LPI}`/" \
	    -e "s/^fonts .*$$/fonts `set ${FONTS}; echo $$#` ${FONTS}/" \
	    ${.CURDIR}/DESC.proto >${.TARGET}

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			$$f ${DESTDIR}${FONTDIR}/${DEV}/; \
	done

.include <bsd.prog.mk>
