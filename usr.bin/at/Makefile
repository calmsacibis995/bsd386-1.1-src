#
# Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI Makefile,v 1.1 1992/08/24 19:13:06 sanders Exp
#
# Compile time options:
#    -UDEBUG					general debug info
#    -UDEBUG_PARSER				to debug parser
#    -UDEBUG_WHEN				debug when.c
#    -UNEED_USLEEP				system doesn't have usleep()
#    -DPROGNAME=varname				where to find argv[0]
#    -UNOHDRS					no headers on list
#    -DCACHESIZE=41				cache size for user names
#    -DAT_DEFSHELL=\"/bin/sh\"			if no env["SHELL"]
#    -DAT_DIR=\"/var/spool/at/\"		at spool directory
#    -DAT_LOCKDIR=\"past\"			dir in AT_DIR for exec'd files
#    -DAT_LOCK=\"at.counter\"			job id counter
#    -DAT_RUNLOCK=\"atrun.lock\"		lockfile for atrun
#    -DAT_ALLOWED=\"/usr/libexec/at_allowed\"	access control
#    -DAT_TMP=\"/tmp\"				where cmds are exec'd from

.if defined(DEBUG)
CFLAGS=-g -DDEBUG=${DEBUG} -DYYDEBUG=1
.endif

.if defined(GPROF)
CFLAGS+=-pg
.endif

CFLAGS+=-I.
LFLAGS+=-i

.if defined(DEBUG) || defined(GPROF)
NOMAN=	noman
YFLAGS+=-tv
.y.o:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c y.tab.c -o ${.TARGET}
.l.o:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c lex.yy.c -o ${.TARGET}
.endif

CFLAGS+=-DPROGNAME=progname
PROG=	at
SRCS=	at.c readqueue.c queuejob.c sched.c when.c \
	utils.c errlib.c dateyacc.y datelex.l
DPADD+=	${LIBL}
LDADD+=	-ll
BINOWN=	root
BINMODE=4555
# BINDIR=/usr/local/bin
LIBEXEC=/usr/libexec
MLINKS+=at.1 atq.1
MLINKS+=at.1 atrm.1
#MLINKS+=at.1 atrun.8
MLINKS+=at.1 at_allowed.8

afterinstall:
	-mkdir ${DESTDIR}/var ${DESTDIR}/var/spool 2>/dev/null
	-mkdir ${DESTDIR}/var/spool/at
	chown root ${DESTDIR}/var/spool/at
	chmod 755 ${DESTDIR}/var/spool/at
	:
	-mkdir ${DESTDIR}/var/spool/at/past
	chown root ${DESTDIR}/var/spool/at/past
	chmod 755 ${DESTDIR}/var/spool/at/past
	:
	-rm -f ${DESTDIR}${BINDIR}/atq
	ln -s ${PROG} ${DESTDIR}${BINDIR}/atq
	-rm -f ${DESTDIR}${BINDIR}/atrm
	ln -s ${PROG} ${DESTDIR}${BINDIR}/atrm
	@: -rm -f ${DESTDIR}${LIBEXEC}/atrun
	@: ln -s ../bin/${PROG} ${DESTDIR}${LIBEXEC}/atrun
	:
	install -c -o root -g bin -m 644 \
			${.CURDIR}/at.counter ${DESTDIR}/var/spool/at
	install -c -o root -g bin -m 755 \
			${.CURDIR}/at_allowed ${DESTDIR}${LIBEXEC}

.include <bsd.prog.mk>
