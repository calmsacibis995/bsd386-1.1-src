#	BSDI $Id: Makefile,v 1.5 1994/01/23 17:34:18 polk Exp $

LIB=	util
SRCS=	Array.c++ AtSyntax.c++ DialRules.c++ Dictionary.c++ Fatal.c++ \
	FaxClient.c++ FaxDB.c++ Obj.c++ PageSize.c++ RegExArray.c++ \
	SendFaxClient.c++ StackBuffer.c++ Str.c++ StrArray.c++ TypeRules.c++ \
	fxassert.c
CC=	gcc2
DEFS=	-DfxSIGHANDLER=sig_t '-DfxSIGVECHANDLER=(void(*)())' -DSIGCLD=SIGCHLD
CFLAGS+=${FAXINCS} ${DEFS}
C++FLAGS+=${FAXINCS} ${DEFS}
MKDEP=	mkdep -2
NOPROFILE=noprofile

LIBFILES=	pagesizes typerules
SHSCRIPT=	faxrcvd notify pollrcvd ps2fax.gs transcript xferstats recvstats
ETCFILES=	dpsprinter.ps

install:
	(cd ${.CURDIR}; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${LIBFILES} \
		${DESTDIR}${LIBDATADIR}; \
	    for i in ${SHSCRIPT}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 555 $$i.sh \
		    ${DESTDIR}${SPOOLBINDIR}/$$i; \
	    done; \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${ETCFILES} \
		${SPOOLETC})

.include <bsd.lib.mk>
