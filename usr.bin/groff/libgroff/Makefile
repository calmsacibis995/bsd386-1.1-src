#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:18 polk Exp $

.include "../Makefile.defs"

LIB=	groff
SRCS=	assert.cc change_lf.cc cmap.cc cset.cc device.cc errarg.cc \
	error.cc fatal.cc filename.cc font.cc fontfile.cc \
	iftoa.c illegal.cc itoa.c lf.cc lineno.cc macropath.cc \
	nametoindex.cc new.cc prime.cc progname.cc ptable.cc \
	searchpath.cc string.cc strsave.cc tmpfile.cc version.cc
NOPROFILE=noprofile
NOMAN=	noman
CLEANFILES+=version.cc

install:

version.cc: ${.CURDIR}/../VERSION
	@echo Making version.cc
	@echo const char \*version_string = \"`cat ${.CURDIR}/../VERSION`\"\; >$@

.include "../Makefile.common"
.include <bsd.lib.mk>

.SUFFIXES: .cc
