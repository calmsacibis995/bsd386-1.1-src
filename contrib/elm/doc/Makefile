#	BSDI	$Id: Makefile,v 1.3 1994/01/14 01:34:29 cp Exp $
ELMLIB=/usr/contrib/lib/elm
DOCMODE=644
NOPROG=	noprog
TBL=tbl
ROFF=groff
ROFFOPT=-mps -Tps
MAN1=	answer.0	\
	autoreply.0	\
	checkalias.0	\
	elm.0		\
	elmalias.0	\
	fastmail.0	\
	filter.0	\
	frm.0		\
	listalias.0	\
	messages.0	\
	newalias.0	\
	newmail.0	\
	printmail.0	\
	readmsg.0	\
	wnewmail.0

# List of files installed in LIB directory
LIB=	elm-help.0	\
	elm-help.1	\
	elm-help.2	\
	elm-help.3	\
	elmrc-info

# List of formatted guides
FMT=	Alias.ps	\
	Config.ps 	\
	Cover.ps	\
	Filter.ps	\
	Form.ps		\
	Ref.ps		\
	Users.ps

all:	$(FMT)

beforeinstall:
	-mkdir ${DESTDIR}${ELMLIB}
	@-chown ${BINOWN} ${DESTDIR}${ELMLIB}
	@-chgrp ${BINGRP} ${DESTDIR}${ELMLIB}
	@-chmod 755 ${DESTDIR}${ELMLIB}

afterinstall:
	for i in $(LIB); do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${DOCMODE} \
			${.CURDIR}/$$i ${DESTDIR}${ELMLIB}; \
	done
	for i in $(FMT); do \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${DOCMODE} \
			$$i ${DESTDIR}${ELMLIB}; \
	done

Alias.ps:	Alias.guide
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Config.ps:	Config.guid
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Cover.ps:	Elm.cover
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Filter.ps:	Filter.guid
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Form.ps:	Form.guide
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Ref.ps:		Ref.guide
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@
Users.ps:	Users.guide
		$(TBL) ${.CURDIR}/$? | $(ROFF) $(ROFFOPT) ${.CURDIR}/tmac.n - > $@

CLEANFILES+= Alias.ps Config.ps Cover.ps Filter.ps Form.ps Ref.ps Users.ps
CLEANFILES+= answer.0 autoreply.0 checkalias.0 elm.0 fastmail.0 filter.0 frm.0
CLEANFILES+= listalias.0 messages.0 newalias.0 newmail.0 printmail.0 readmsg.0
CLEANFILES+= wnewmail.0

.include <../../Makefile.inc>
.include <bsd.prog.mk>
