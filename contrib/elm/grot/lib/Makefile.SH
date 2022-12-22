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

case "$d_usenls" in
define)	mcsrc="mcprt.c mcprtlib.c msgcat.c"
	mcobj="mcprt.o mcprtlib.o msgcat.o"
	;;
*)	mcsrc= mcobj= ;;
esac

case "$d_strstr" in
define)	strsrc= strobj= ;;
*)	strsrc="strstr.c"
	strobj="strstr.o"
	;;
esac

case "$d_strftime" in
define)	ftimesrc= ftimeobj= ;;
*)	ftimesrc="strftime.c"
	ftimeobj="strftime.o"
	;;
esac

echo "Extracting lib/Makefile (with variable substitutions)"
cat >Makefile <<!GROK!THIS!
# $Id: Makefile.SH,v 5.15 1993/08/03 19:28:39 syd Exp 
#
#  Makefile for the ELM mail program.
#
#	Copyright (c) 1986,1987 Dave Taylor
#	Copyright (c) 1988-1992 USENET Community Trust
#
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Syd Weinstein - elm@DSI.COM
#			dsinc!elm
#
# $Log: Makefile.SH,v $
# Revision 1.2  1994/01/14  00:52:15  cp
# 2.4.23
#
# Revision 5.15  1993/08/03  19:28:39  syd
# Elm tries to replace the system toupper() and tolower() on current
# BSD systems, which is unnecessary.  Even worse, the replacements
# collide during linking with routines in isctype.o.  This patch adds
# a Configure test to determine whether replacements are really needed
# (BROKE_CTYPE definition).  The <ctype.h> header file is now included
# globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
# there.  Inclusion of <ctype.h> was removed from *all* the individual
# files, and the toupper() and tolower() routines in lib/opt_utils.c
# were dropped.
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.14  1993/08/03  19:17:33  syd
# Implement new timezone handling.  New file lib/get_tz.c with new timezone
# routines.  Added new TZMINS_USE_xxxxxx and TZNAME_USE_xxxxxx configuration
# definitions.  Obsoleted TZNAME, ALTCHECK, and TZ_MINUTESWEST configuration
# definitions.  Updated Configure.  Modified lib/getarpdate.c and
# lib/strftime.c to use new timezone routines.
#
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.13  1993/08/02  18:14:35  syd
# add missing references for mcprt.c/mcprtlib.c to defs.h
# From: Syd
#
# Revision 5.12  1993/06/10  03:02:46  syd
# break_down_tolist() tried to blindly split address lists at "," which
# caused bogus results with addreses that had a comma inside a comment
# or quoted text, such as "user@domain (Last, First)".  This patch steps
# through the address in quanta of RFC-822 tokens when searching for a
# delimiting comma.  It also adds "rfc822_toklen()" to the library to
# get that length.
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.11  1993/04/12  01:51:42  syd
# Added safe_malloc(), safe_realloc(), and safe_strdup().  They
# will be used in the new elmalias utility.
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.10  1993/02/03  16:21:03  syd
# add strtokq
# From: Syd
#
# Revision 5.9  1993/01/27  18:52:03  syd
# add strftime as optional component
#
# Revision 5.8  1993/01/19  05:07:05  syd
# Trim erroreous extra log entry
# From: Syd
#
# Revision 5.7  1993/01/19  04:47:12  syd
# Significant changes to provide consistent Date and From_ header
# cracking.  Overhauled date utilities and moved into library.  Moved
# real_from() into library.  Modified frm, newmail, and readmsg utilities
# to use library version of real_from().  Moved get_word() from Elm
# source into library.  Added new library routines atonum() and strfcpy().
# Fixed trailing backslash bug in len_next().
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.6  1992/12/24  19:33:59  syd
# tune dependency list further
# From: Syd
#
# Revision 5.5  1992/12/24  19:30:46  syd
# Make all object depend on config.h, so a reconfig for a new
# arch in shared sources (or copied sources) forces a recompile
# of all modules.
# From: Syd
#
# Revision 5.4  1992/12/20  05:15:58  syd
# Add a c)hange alias, -u and -t options to listalias to list only user
# and only system aliases respectively.
# From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
#
# Revision 5.3  1992/11/07  20:07:23  syd
# Add header_cmp routine
# From: Syd
#
# Revision 5.2  1992/10/27  01:43:40  syd
# Move posix_signal to lib directory
# From: tom@osf.org
#
# Revision 5.1  1992/10/03  22:41:36  syd
# Initial checkin as of 2.4 Release at PL0
#
#
#
# Variables
#	Variables established by Configure
AR		=	ar
CC		=	$cc
CCFLAGS		=	$ccflags $xencf
CHMOD		=	$chmod
LINT		=	$lint
OPTIMIZE	=	$optimize
RANLIB		=	$ranlib
RM		= 	$rm -f
TOUCH		=	$touch
TAGS		=	ctags

MCSRC		=	$mcsrc
MCOBJ		=	$mcobj

STRSRC		=	$strsrc
STROBJ		=	$strobj

FTIMESRC	=	$ftimesrc
FTIMEOBJ	=	$ftimeobj

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
CFLAGS		=	$(CCFLAGS) $(OPTIMIZE) -I$(INCLDIR) $(DEBUG) $(DACSNET) 
INCLDIR		=	../hdrs
LINTFLAGS	=	-I$(INCLDIR)
SHELL		=	/bin/sh

# 	Lists
LIB_SRC		=	add_site.c	\
			addrmchusr.c	\
			atonum.c	\
			mk_aliases.c	\
			aliasdb.c	\
			mk_lockname.c	\
			can_access.c	\
			can_open.c	\
			chloc.c		\
			date_util.c	\
			errno.c		\
			expand.c	\
			figadrssee.c	\
			gcos_name.c	\
			getaddrfrm.c	\
			getarpdate.c	\
			getfullnam.c	\
			getword.c	\
			get_tz.c	\
			header_cmp.c	\
			in_list.c	\
			in_string.c	\
			istrcmp.c	\
			ldstate.c	\
			len_next.c	\
			mail_gets.c	\
			$(MCSRC)	\
			move_left.c	\
			ndbz.c		\
			okay_addr.c	\
			opt_utils.c	\
			parsarpdat.c	\
			parsarpwho.c	\
			posixsig.c	\
			putenv.c	\
			realfrom.c	\
			qstrings.c	\
			remfirstwd.c	\
			reverse.c	\
			rfc822tlen.c	\
			safemalloc.c	\
			shiftlower.c	\
			strfcpy.c	\
			strincmp.c	\
			striparens.c	\
			$(STRSRC)	\
			$(FTIMESRC)	\
			strtokq.c	\
			tail_of.c	\
			validname.c

LIB_OBJ		=	add_site.o	\
			addrmchusr.o	\
			atonum.o	\
			mk_aliases.o	\
			aliasdb.o	\
			mk_lockname.o	\
			can_access.o	\
			can_open.o	\
			chloc.o		\
			date_util.o	\
			errno.o		\
			expand.o	\
			figadrssee.o	\
			gcos_name.o	\
			get_tz.o	\
			getaddrfrm.o	\
			getarpdate.o	\
			getfullnam.o	\
			getword.o	\
			header_cmp.o	\
			in_list.o	\
			in_string.o	\
			istrcmp.o	\
			ldstate.o	\
			len_next.o	\
			mail_gets.o	\
			$(MCOBJ)	\
			move_left.o	\
			ndbz.o		\
			okay_addr.o	\
			opt_utils.o	\
			parsarpdat.o	\
			parsarpwho.o	\
			posixsig.o	\
			putenv.o	\
			qstrings.o	\
			realfrom.o	\
			remfirstwd.o	\
			reverse.o	\
			rfc822tlen.o	\
			safemalloc.o	\
			shiftlower.o	\
			strfcpy.o	\
			strincmp.o	\
			striparens.o	\
			$(STROBJ)	\
			$(FTIMEOBJ)	\
			strtokq.o	\
			tail_of.o	\
			validname.o

all:			libutil.a

install:		libutil.a

uninstall:		libutil.a

libutil.a:		$(LIB_OBJ)
			$(AR) r $@ $?
			$(RANLIB) $@

lint:		
		$(LINT) $(LINTFLAGS) $(LIB_SRC) > LINT.OUT

tags:
		$(TAGS) $(LIB_SRC)

clean:		
		$(RM) $(LIB_OBJ) libutil.a

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
add_site.o:	$(INCLDIR)/headers.h
addrmchusr.o:	$(INCLDIR)/headers.h
atonum.o:	$(INCLDIR)/defs.h
mk_aliases.o:	$(INCLDIR)/headers.h $(INCLDIR)/ndbz.h $(INCLDIR)/s_newalias.h
aliasdb.o:	$(INCLDIR)/headers.h $(INCLDIR)/ndbz.h
mk_lockname.o:	$(INCLDIR)/headers.h
can_access.o:	$(INCLDIR)/headers.h
can_open.o:	$(INCLDIR)/headers.h
chloc.o:	$(INCLDIR)/headers.h
date_util.o:	$(INCLDIR)/headers.h
errno.o:	$(INCLDIR)/headers.h
expand.o:	$(INCLDIR)/defs.h $(INCLDIR)/s_elmrc.h
figadrssee.o:	$(INCLDIR)/headers.h
gcos_name.o:	$(INCLDIR)/headers.h
get_tz.o:	$(INCLDIR)/defs.h
getaddrfrm.o:	$(INCLDIR)/headers.h
getarpdate.o:	$(INCLDIR)/headers.h
getfullnam.o:	$(INCLDIR)/headers.h
getword.o:	$(INCLDIR)/defs.h
header_cmp.o:	$(INCLDIR)/headers.h
in_list.o:	$(INCLDIR)/headers.h
in_string.o:	$(INCLDIR)/headers.h
istrcmp.o:	$(INCLDIR)/headers.h
ldstate.o:	$(INCLDIR)/defs.h
len_next.o:	../config.h
mail_gets.o:	../config.h
mcprt.o:	$(INCLDIR)/mcprt.h $(INCLDIR)/mcprtlib.h $(INCLDIR)/defs.h
mcprtlib.o:	$(INCLDIR)/mcprtlib.h $(INCLDIR)/nl_types.h $(INCLDIR)/defs.h
move_left.o:	$(INCLDIR)/headers.h
msgcat.o:	$(INCLDIR)/msgcat.h $(INCLDIR)/nl_types.h
ndbz.o:		$(INCLDIR)/headers.h $(INCLDIR)/ndbz.h
okay_addr.o:	$(INCLDIR)/headers.h
opt_utils.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_error.h
parsarpdat.o:	$(INCLDIR)/headers.h
parsarpwho.o:	$(INCLDIR)/headers.h
posixsig.o:	$(INCLDIR)/headers.h
putenv.o:	$(INCLDIR)/headers.h
realfrom.o:	$(INCLDIR)/headers.h $(INCLDIR)/s_elm.h
qstrings.o:	../config.h
remfirstwd.o:	$(INCLDIR)/headers.h
reverse.o:	$(INCLDIR)/headers.h
safemalloc.o:	$(INCLDIR)/defs.h
shiftlower.o:	$(INCLDIR)/headers.h
strincmp.o:	$(INCLDIR)/headers.h
striparens.o:	$(INCLDIR)/headers.h
strstr.o:	$(INCLDIR)/headers.h
strtokq.o:	$(INCLDIR)/headers.h
tail_of.o:	$(INCLDIR)/headers.h
validname.o:	$(INCLDIR)/defs.h

!NO!SUBS!
