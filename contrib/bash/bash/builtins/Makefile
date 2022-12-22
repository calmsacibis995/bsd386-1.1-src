#	BSDI $Id: Makefile,v 1.3 1994/01/05 17:54:51 polk Exp $

CFLAGS+=-I. -I${.CURDIR} -I$(.CURDIR)/../.. -I$(.CURDIR)/../../readline
CFLAGS+=-DHAVE_ALLOCA -DHAVE_DUP2 -DHAVE_GETGROUPS -DHAVE_GETWD \
	-DHAVE_RESOURCE -DHAVE_SETLINEBUF -DHAVE_UNISTD_H -DHAVE_WAIT_H \
	-DBsd -DHOSTTYPE=\"$(MACHINE)\" -DSHELL -DVOID_SIGHANDLER \
	-D"$(MACHINE)"
LIB=	builtins
DEF=	alias bind break builtin cd colon command declare echo enable \
	eval exec exit fc fg_bg getopts hash help history jobs kill let \
	read return set setattr shift source suspend test times trap type \
	ulimit umask wait
DSRC=	${DEF:S/^/${.CURDIR}\//g:S/$/.def/g} ${.CURDIR}/reserved.def
BUILT=	${DEF:S/$/.c/g}
SRCS=	getopt.c common.c ${BUILT} builtins.c
NOMAN=	noman
NOPROFILE=noprofile
CLEANFILES+=${BUILT} builtext.h builtins.c mkbuiltins pipesize.h

all:	builtext.h

builtext.h builtins.c: ${DSRC} mkbuiltins
	./mkbuiltins -externfile builtext.h \
		-structfile builtins.c -noproduction ${DSRC}

mkbuiltins: mkbuiltins.c
	${CC} ${CFLAGS} -o mkbuiltins ${.CURDIR}/mkbuiltins.c

.depend: builtext.h mkbuiltins.c

install: 

.include <bsd.lib.mk>

# These must go after <bsd.lib.mk> as it clobbers the suffixes.
.SUFFIXES: .def

.def.c:
	./mkbuiltins ${.IMPSRC}

# ulimit.o includes pipesize.h when it does not need to; we make a
# dummy one here.
ulimit.o .depend: pipesize.h
pipesize.h:
	: > ${.TARGET}
