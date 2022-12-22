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

echo "Extracting filter/Makefile (with variable substitutions)"
: if mailermode is not 2755 then we dont need the chgrp
: set chgrp to a null command in this case
if test "$mailermode" = 755; then
	chgrp=":"
fi
cat >Makefile <<!GROK!THIS!
#
# @(#)Id: Makefile.SH,v 5.5 1993/06/06 17:35:04 syd Exp 
#  Makefile for the Elm system filter program
#
#         (C) Copyright 1986,1987, by Dave Taylor
#         (C) Copyright 1988-1992, USENET Community Trust
#
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Syd Weinstein - elm@DSI.COM
#			dsinc!elm
#
#  $Log: Makefile.SH,v $
# Revision 1.2  1994/01/14  00:52:10  cp
# 2.4.23
#
# Revision 5.5  1993/06/06  17:35:04  syd
# Remove useless buffer.c
#
# Revision 5.4  1993/02/03  16:22:06  syd
# add libutil.a as strtokq moved to lib
#
# Revision 5.3  1993/01/27  19:40:01  syd
# I implemented a change to filter's default verbose message format
# including %x %X style date and time along with username
# From: mark@drd.com (Mark Lawrence)
#
# Revision 5.2  1992/11/15  01:40:43  syd
# Add regexp processing to filter.
# Add execc operator
# From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
#
# Revision 5.1  1992/10/03  22:18:09  syd
# Initial checkin as of 2.4 Release at PL0
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
LIBS		=	$termlib $dbm
LINT		=	$lint
MAILGRP		=	$mailgrp
MAILERMODE	=	$mailermode
MAKE		=	$make
MV		=	$mv
OPTIMIZE	=	$optimize
RM		= 	$rm -f
TOUCH		=	$touch

!GROK!THIS!

cat >>Makefile <<'!NO!SUBS!'
#	Variables you may want to manually edit
#		If you want debug logging then you'll
#		want to uncomment the following.
#DEBUG		=	-DDEBUG

#	Other variables
BIN		=	../bin
INCLDIR		=	../hdrs
CFLAGS		=	$(CCFLAGS) $(OPTIMIZE) -I$(INCLDIR) $(DEBUG) $(DACSNET) 
LINTFLAGS	=	-I$(INCLDIR)
SHELL		=	/bin/sh

# Definitions of variables
FILTER_SRC	=	actions.c	\
			filter.c	\
			lock.c		\
			parse.c		\
			regexp.c	\
			rules.c		\
			summarize.c	\
			utils.c		\
			audit.c

FILTER_OBJ	=	actions.o	\
			filter.o	\
			lock.o		\
			parse.o		\
			regexp.o	\
			rules.o		\
			summarize.o	\
			utils.o		\
			audit.o		\
			../lib/libutil.a

# Standard targets
all:		$(BIN)/filter
		
install:	$(DEST)/filter

uninstall:	
		$(RM) $(DEST)/filter

#	This is the only target that gets installed even if not out-of-date
#	with respect the files from which it is installed.
rmt-install:	rmt-defined
		-$(MV) $(DEST)/filter $(DEST)/filter.old
		-$(RM) $(DEST)/filter.old
		$(CP) $(REMOTE)$(DEST)/filter $(DEST)/filter
		$(CHGRP) $(MAILGRP) $(DEST)/filter
		$(CHMOD) $(MAILERMODE) $(DEST)/filter

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
		$(LINT) $(LINTFLAGS) $(FILTER_SRC) > LINT.OUT

clean:		
		$(RM) $(FILTER_OBJ) $(BIN)/filter

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

#	Dependencies and rules for C object files
actions.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
filter.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
lock.o:			$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
parse.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
regexp.o:		$(INCLDIR)/defs.h $(INCLDIR)/regexp.h
rules.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
summarize.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
utils.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h
audit.o:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h $(INCLDIR)/s_filter.h

#	Dependencies and rules for compiling programs
$(BIN)/filter:	$& $(FILTER_OBJ)
		$(CC) $(LFLAGS) -o $@ $(FILTER_OBJ) ../lib/libutil.a $(LIB2)

#	Dependencies and rules for installing programs from bin directory
$(DEST)/filter:		$(BIN)/filter
			-$(MV) $(DEST)/filter $(DEST)/filter.old
			-$(RM) $(DEST)/filter.old
			$(CP) $? $@
			$(CHGRP) $(MAILGRP) $@
			$(CHMOD) $(MAILERMODE) $@
!NO!SUBS!
