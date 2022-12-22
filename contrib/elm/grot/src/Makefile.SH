case $CONFIG in
'')
    if test ! -f config.sh; then
	ln ../config.sh . || \
	ln ../../config.sh . || \
	ln ../../../config.sh . || \
	(echo "Can't find config.sh."; exit 1)
    fi
    . ./config.sh
    ;;
esac
case "$0" in
*/*) cd `expr X$0 : 'X\(.*\)/'` ;;
esac

echo "Extracting src/Makefile (with variable substitutions)"
: if mailermode is not 2755 then we dont need the chgrp
: set chgrp to a null command in this case
if test "$mailermode" = 755; then
	chgrp=":"
fi
cat >Makefile <<!GROK!THIS!
# $Id: Makefile.SH,v 5.1 1992/10/03 22:58:40 syd Exp 
#
#  Makefile for the ELM mail program.
#
#	Copyright (c) 1988-1992 USENET Community Trust
#	Copyright (c) 1986,1987 Dave Taylor
#
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Syd Weinstein - elm@DSI.COM
#			dsinc!elm
#
# $Log: Makefile.SH,v $
# Revision 1.2  1994/01/14  00:52:18  cp
# 2.4.23
#
# Revision 5.1  1992/10/03  22:58:40  syd
# Initial checkin as of 2.4 Release at PL0
#
# Revision 5.1  1992/10/03  22:58:40  syd
# Initial checkin as of 2.4 Release at PL0
#
# 
#
# Variables
#	Variables established by Configure
CC		=	$cc
CCFLAGS		=	$ccflags $xencf
CHGRP		=	$chgrp
CHMOD		=	$chmod
CP		=	$cp
DEST		=	$bin
ECHO		=	$echo
LFLAGS		=	$ldflags $xenlf
LIB		=	$lib
LIB2		= 	$libs
LIBS		=	$cryptlib $termlib $dbm
LINT		=	$lint
MAILGRP		=	$mailgrp
MAILERMODE	=	$mailermode
MV		=	$mv
OPTIMIZE	=	$optimize
RM		= 	$rm -f
TOUCH		=	$touch
TAGS		=	ctags

!GROK!THIS!

cat >>Makefile <<'!NO!SUBS!'

#	Variables you may want to manually edit
#		If you want debug logging then you'll
#		want to uncomment the following.
#DEBUG		=	-DDEBUG

#		If you're on an ACSnet system (Australia) then
#		you'll want to uncomment the following.
# DACSNET	=	-DACSNET

#	Other general variables
BIN		=	../bin
CFLAGS		=	$(CCFLAGS) $(OPTIMIZE) -I$(INCLDIR) $(DEBUG) $(DACSNET) 
INCLDIR		=	../hdrs
LINTFLAGS	=	-I$(INCLDIR)
SHELL		=	/bin/sh

# 	Lists
ELM_SRC		=	addr_util.c	\
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

ELM_OBJ		=	addr_util.o	\
			alias.o		\
			aliaslib.o	\
			args.o		\
			a_edit.o	\
			a_screen.o	\
			a_sort.o	\
			a_quit.o	\
			bouncebk.o	\
			builtin.o	\
			calendar.o	\
			curses.o	\
			date.o		\
			delete.o	\
			edit.o		\
			editmsg.o	\
			elm.o		\
			encode.o	\
			exitprog.o	\
			expires.o	\
			file.o		\
			file_util.o	\
			fileio.o	\
			find_alias.o	\
			forms.o		\
			hdrconfg.o	\
			help.o		\
			in_utils.o	\
			init.o		\
			leavembox.o	\
			lock.o		\
			limit.o		\
			mailmsg1.o	\
			mailmsg2.o	\
			mime.o		\
			mkhdrs.o	\
			newmbox.o	\
			options.o	\
			out_utils.o	\
			pattern.o	\
			pmalloc.o	\
			quit.o		\
			read_rc.o	\
			remail.o	\
			reply.o		\
			returnadd.o	\
			save_opts.o	\
			savecopy.o	\
			screen.o	\
			showmsg.o	\
			showmsg_c.o	\
			signals.o	\
			softkeys.o	\
			sort.o		\
			string2.o	\
			strings.o	\
			syscall.o	\
			utils.o		\
			wildcards.o	\
			wordwrap.o

# Standard targets
all:		$(BIN)/elm

install:	$(DEST)/elm

uninstall:	
		$(RM) $(DEST)/elm

#	This is the only target that gets installed even if not out-of-date
#	with respect the files from which it is installed.
rmt-install:	rmt-defined
		-$(MV) $(DEST)/elm $(DEST)/elm.old
		-$(RM) $(DEST)/elm.old
		$(CP) $(REMOTE)$(DEST)/elm $(DEST)/elm
		$(CHGRP) $(MAILGRP) $(DEST)/elm
		$(CHMOD) $(MAILERMODE) $(DEST)/elm

rmt-defined:
	@(if [ "$(REMOTE)" = "" ];\
	  then\
	    $(ECHO) "You need to define 'REMOTE' as the remote file system";\
	    $(ECHO) "for this particular command. The easiest way to do this";\
	    $(ECHO) "to type:";\
	    $(ECHO) "        make REMOTE=<remote file system> rmt-install";\
	    exit 1;\
	fi);

lint:		
		$(LINT) $(LINTFLAGS) $(ELM_SRC) > LINT.OUT

tags:
		$(TAGS) $(ELM_SRC)

clean:		
		$(RM) $(ELM_OBJ) $(BIN)/elm

# Dependencies and rules
#	Dependencies of header files upon other header files they include
.PRECIOUS:		$(INCLDIR)/defs.h $(INCLDIR)/elm.h $(INCLDIR)/headers.h

$(INCLDIR)/defs.h:	$(INCLDIR)/../config.h $(INCLDIR)/sysdefs.h
			$(CHMOD) u+w $@
			$(TOUCH) $@

$(INCLDIR)/elm.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(CHMOD) u+w $@
			$(TOUCH) $@

$(INCLDIR)/headers.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(CHMOD) u+w $@
			$(TOUCH) $@

#	Dependencies of C object files
addr_util.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
alias.o:	$(INCLDIR)/headers.h $(INCLDIR)/ndbz.h $(INCLDIR)/s_aliases.h $(INCLDIR)/s_elm.h
aliaslib.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
args.o:		$(INCLDIR)/headers.h $(INCLDIR)/patchlevel.h $(INCLDIR)/s_elm.h
a_edit.o:	$(INCLDIR)/headers.h
a_screen.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_aliases.h
a_sort.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_aliases.h
a_quit.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_aliases.h
bouncebk.o:	$(INCLDIR)/headers.h
builtin.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
calendar.o:	$(INCLDIR)/headers.h
curses.o:	$(INCLDIR)/headers.h
date.o:		$(INCLDIR)/headers.h
delete.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
edit.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
editmsg.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
elm.o:		$(INCLDIR)/elm.h $(INCLDIR)/s_elm.h
encode.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
exitprog.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
expires.o:	$(INCLDIR)/headers.h
file.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
file_util.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
fileio.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
find_alias.o:	$(INCLDIR)/headers.h
forms.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
hdrconfg.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
help.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
in_utils.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
init.o:		$(INCLDIR)/headers.h $(INCLDIR)/patchlevel.h $(INCLDIR)/s_elm.h
leavembox.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
lock.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
limit.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h $(INCLDIR)/s_aliases.h
mailmsg1.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
mailmsg2.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
mkhdrs.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
mime.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
newmbox.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
options.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h $(INCLDIR)/save_opts.h
out_utils.o:	$(INCLDIR)/headers.h
pattern.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
pmalloc.o:	$(INCLDIR)/defs.h $(INCLDIR)/s_elm.h
quit.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
read_rc.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h $(INCLDIR)/save_opts.h
remail.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
reply.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
returnadd.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
save_opts.o:	$(INCLDIR)/save_opts.h $(INCLDIR)/headers.h $(INCLDIR)/s_elmrc.h
savecopy.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
screen.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
showmsg.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
showmsg_c.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
signals.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
softkeys.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
sort.o:		$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
string2.o:	$(INCLDIR)/headers.h
strings.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
syscall.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
utils.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
wildcards.o:	$(INCLDIR)/defs.h
wordwrap.o:	$(INCLDIR)/headers.h

#	Dependencies and rules for compiling C programs
$(BIN)/elm:	$& $(ELM_OBJ) ../lib/libutil.a
		$(CC) $(LFLAGS) -o $@ $(ELM_OBJ) ../lib/libutil.a $(LIBS) $(LIB2)

#	Dependencies and rules for installing C programs
$(DEST)/elm:	$(BIN)/elm
		-$(MV) $@ $@.old
		-$(RM) $@.old
		$(CP) $(BIN)/elm $@
		$(CHGRP) $(MAILGRP) $@
		$(CHMOD) $(MAILERMODE) $@

!NO!SUBS!
