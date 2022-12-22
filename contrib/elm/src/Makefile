#	BSDI	$Id: Makefile,v 1.2 1994/01/14 00:54:15 cp Exp $

BINOWN=	root
BINMODE=4511

CFLAGS+=-I${.CURDIR}/../hdrs
LDFLAGS+=-L${.CURDIR}/../lib/obj
LDFLAGS+=-L${.CURDIR}/../lib
LDADD+=	-lutil -ltermlib
PROG=	elm
NOMAN=	noman
SRCS=	addr_util.c	\
	alias.c		\
	aliaslib.c	\
	args.c		\
	a_edit.c	\
	a_screen.c	\
	a_sort.c	\
	a_quit.c	\
	bouncebk.c	\
	builtin.c	\
	calendar.c	\
	curses.c	\
	date.c		\
	delete.c	\
	edit.c		\
	editmsg.c	\
	elm.c		\
	encode.c	\
	exitprog.c	\
	expires.c	\
	file.c		\
	file_util.c	\
	fileio.c	\
	find_alias.c	\
	forms.c		\
	hdrconfg.c	\
	help.c		\
	in_utils.c	\
	init.c		\
	leavembox.c	\
	lock.c		\
	limit.c		\
	mailmsg1.c	\
	mailmsg2.c	\
	mime.c		\
	mkhdrs.c	\
	newmbox.c	\
	options.c	\
	out_utils.c	\
	pattern.c	\
	pmalloc.c	\
	quit.c		\
	read_rc.c	\
	remail.c	\
	reply.c		\
	returnadd.c	\
	save_opts.c	\
	savecopy.c	\
	screen.c	\
	showmsg.c	\
	showmsg_c.c	\
	signals.c	\
	softkeys.c	\
	sort.c		\
	string2.c	\
	strings.c	\
	syscall.c	\
	utils.c		\
	wildcards.c	\
	wordwrap.c

.include <../../Makefile.inc>
.include <bsd.prog.mk>
