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

echo "Extracting doc/Makefile (with variable substitutions)"
cat >Makefile <<!GROK!THIS!
#
# $Id: Makefile.SH,v 5.11 1993/04/12 02:12:07 syd Exp 
#
#  Makefile for the documentation of the ELM mail system
#
#         (C) Copyright 1988-1992, USENET Community Trust
#
#  $Log: Makefile.SH,v $
# Revision 1.2  1994/01/14  00:52:07  cp
# 2.4.23
#
# Revision 5.11  1993/04/12  02:12:07  syd
# Added elmalias.1 man page.  Updated Users.guide to mention elmalias.
# From: chip@chinacat.unicom.com (Chip Rosenthal)
#
# Revision 5.10  1993/01/20  03:11:51  syd
# During 'make uninstall', an error occurs with the make for doc/,
# due to a wrongly-extended "if" statement in the Makefile.
# From: Allan Kugel <hico2!hico3!allan@tsdiag.ocpt.ccur.com>
#
# Revision 5.9  1992/12/13  18:00:05  syd
# Add one more missing rm of a previously packed man page
# From: Bo Asbjorn Muldbak <bam@jutland.ColumbiaSC.NCR.COM>
#
# Revision 5.8  1992/12/12  01:28:50  syd
# Remove the old packed manual page before trying to pack the new one.
# From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
#
# Revision 5.7  1992/12/07  05:05:27  syd
# fix ; placement
# From: Syd
#
# Revision 5.6  1992/12/07  03:28:35  syd
# add else clause with true to TEST lines as some Makefiles exit if
# the if construct doesnt execute a clause as the if returns an error
# status.
# From: Syd via requests from several people
#
# Revision 5.5  1992/12/07  02:17:14  syd
# Add missing -n flag to doc makefile.
# From: gwh@dogmatix.inmos.co.uk
#
# Revision 5.4  1992/11/22  00:47:02  syd
# Change the way Configure and doc/Makefile.SH deal with installing
# man pages.
# From: Tom Moore <tmoore@wnas.DaytonOH.NCR.COM>
#
# Revision 5.3  1992/11/07  16:13:00  syd
# Limit test arguments to -f instead of -x as several shells and
# their resulting test commands dont have -x
# From: Syd via request from Tim Pierce
#
# Revision 5.2  1992/10/30  21:06:09  syd
# add missing MAKE variable definition
# From: syd via note from Keith Petersen w8sdz@tacom-emh1.army.mil
#
# Revision 5.1  1992/10/03  20:51:50  syd
# Initial checkin as of 2.4 Release at PL0
#
# 
#
# Variables
#	Variables established by Configure
CHMOD		=	$chmod
CP		=	$cp
DEST		=	$bin
ECHO		=	$echo
FORMATTER	=	$roff
FORMATTER_OPTS	=	$roffopts
LIB		=	$lib
LN		=	$ln
MAKE		=	$make
MAN		=	$mansrc
MANEXT		=	$manext
CATMAN		=	$catmansrc
CATMANEXT	=	$catmanext
RM		= 	$rm -f
TBL       	=	$tbl
MANROFF		=	$manroff
MANROFF_OPTS	=	$manroffopts
MV		=	$mv
TEST		=	$test
SUFFIX		=	$suffix
PACKED		=	$packed
PACKER		=	$packer
!GROK!THIS!

cat >>Makefile <<'!NO!SUBS!'

#	Other variables
SHELL		=	/bin/sh

# List of installed man pages (except for wnemail.1 - handled differently)
MAN_LIST	=	$(MAN)/answer$(MANEXT)		\
			$(MAN)/autoreply$(MANEXT)	\
			$(MAN)/checkalias$(MANEXT)	\
			$(MAN)/elm$(MANEXT)		\
			$(MAN)/elmalias$(MANEXT)	\
			$(MAN)/fastmail$(MANEXT)	\
			$(MAN)/filter$(MANEXT)		\
			$(MAN)/frm$(MANEXT)		\
			$(MAN)/listalias$(MANEXT)	\
			$(MAN)/messages$(MANEXT)	\
			$(MAN)/newalias$(MANEXT)	\
			$(MAN)/newmail$(MANEXT)	\
			$(MAN)/printmail$(MANEXT)	\
			$(MAN)/readmsg$(MANEXT)

# List of installed catman pages (except for wnemail.1 - handled differently)
CATMAN_LIST	=	$(CATMAN)/answer$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/autoreply$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/checkalias$(CATMANEXT)$(SUFFIX)\
			$(CATMAN)/elm$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/elmalias$(CATMANEXT)$(SUFFIX)\
			$(CATMAN)/fastmail$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/filter$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/frm$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/listalias$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/messages$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/newalias$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/newmail$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/printmail$(CATMANEXT)$(SUFFIX)	\
			$(CATMAN)/readmsg$(CATMANEXT)$(SUFFIX)

# List of formatted pages for catman
FORMATTED_PAGES_LIST =	catman/answer$(CATMANEXT)$(SUFFIX)	\
			catman/autoreply$(CATMANEXT)$(SUFFIX)	\
			catman/checkalias$(CATMANEXT)$(SUFFIX)\
			catman/elm$(CATMANEXT)$(SUFFIX)	\
			catman/elmalias$(CATMANEXT)$(SUFFIX)\
			catman/fastmail$(CATMANEXT)$(SUFFIX)	\
			catman/filter$(CATMANEXT)$(SUFFIX)	\
			catman/frm$(CATMANEXT)$(SUFFIX)	\
			catman/listalias$(CATMANEXT)$(SUFFIX)	\
			catman/messages$(CATMANEXT)$(SUFFIX)	\
			catman/newalias$(CATMANEXT)$(SUFFIX)	\
			catman/newmail$(CATMANEXT)$(SUFFIX)	\
			catman/printmail$(CATMANEXT)$(SUFFIX)	\
			catman/readmsg$(CATMANEXT)$(SUFFIX)

# List of remotely installed man pages (except for wnemail.1 - handled
# differently)
REMOTE_MAN_LIST	=	$(REMOTE)$(MAN)/answer$(MANEXT)	\
			$(REMOTE)$(MAN)/autoreply$(MANEXT)	\
			$(REMOTE)$(MAN)/checkalias$(MANEXT)	\
			$(REMOTE)$(MAN)/elm$(MANEXT)		\
			$(REMOTE)$(MAN)/elmalias$(MANEXT)	\
			$(REMOTE)$(MAN)/fastmail$(MANEXT)	\
			$(REMOTE)$(MAN)/filter$(MANEXT)	\
			$(REMOTE)$(MAN)/frm$(MANEXT)		\
			$(REMOTE)$(MAN)/listalias$(MANEXT)	\
			$(REMOTE)$(MAN)/messages$(MANEXT)	\
			$(REMOTE)$(MAN)/newalias$(MANEXT)	\
			$(REMOTE)$(MAN)/newmail$(MANEXT)	\
			$(REMOTE)$(MAN)/printmail$(MANEXT)	\
			$(REMOTE)$(MAN)/readmsg$(MANEXT)

# List of files installed in LIB directory
LIB_LIST	=	$(LIB)/elm-help.0	\
			$(LIB)/elm-help.1	\
			$(LIB)/elm-help.2	\
			$(LIB)/elm-help.3	\
			$(LIB)/elmrc-info

# List of files installed in remote LIB directory
REMOTE_LIB_LIST	=	$(REMOTE)$(LIB)/elm-help.0	\
			$(REMOTE)$(LIB)/elm-help.1	\
			$(REMOTE)$(LIB)/elm-help.2	\
			$(REMOTE)$(LIB)/elm-help.3	\
			$(REMOTE)$(LIB)/elmrc-info

# List of formatted guides
FORMATTED_LIST	=	Alias.fmtd	\
			Config.fmtd 	\
			Cover.fmtd	\
			Filter.fmtd	\
			Form.fmtd	\
			Ref.fmtd	\
			Users.fmtd

# Targets
all: 
	@if $(TEST) '$(CATMAN)' != none; then $(MAKE) formatted_pages ;  else true ; fi

#	The following are no-ops
lint:
clean:	

formatted_pages: catman $(FORMATTED_PAGES_LIST)

catman:
	mkdir catman

install: $(LIB_LIST)
	@if $(TEST) '$(MAN)' != none; then $(MAKE) install_man ; else true ;  fi
	@if $(TEST) '$(CATMAN)' != none; then $(MAKE) install_catman ; else true ;  fi

install_man: $(MAN_LIST) $(MAN)/wnewmail$(MANEXT)

install_catman: $(CATMAN_LIST) $(CATMAN)/wnewmail$(CATMANEXT)$(SUFFIX)

uninstall:
	$(RM) $(LIB_LIST) 
	@if $(TEST) '$(MAN)' != none; then  \
		$(RM) $(MAN_LIST) $(MAN)/wnewmail$(MANEXT) ; \
	else \
		true ; \
	fi
	@if $(TEST) '$(CATMAN)' != none; then \
		$(RM) $(CATMAN_LIST) $(CATMAN)/wnewmail$(CATMANEXT)$(SUFFIX) ; \
	else \
		true ; \
	fi

#	Dependencies and rules for installing man pages and lib files
#	taken from a remote system
#	This is the only target that gets installed even if not out-of-date
#	with respect the files from which it is installed.
rmt-install:	rmt-defined
		$(RM) $(MAN)/newmail$(MANEXT) $(MAN)/wnewmail$(MANEXT)
		$(CP) $(REMOTE_MAN_LIST) $(MAN)
		$(CHMOD) u=rw,go=r $(MAN_LIST)
		$(LN) $(MAN)/newmail$(MANEXT) $(MAN)/wnewmail$(MANEXT)
		$(CP) $(REMOTE_LIB_LIST) $(LIB)
		$(CHMOD) u=rw,go=r $(LIB_LIST)

rmt-defined:
	@(if [ "$(REMOTE)" = "" ];\
	  then\
	    $(ECHO) "You need to define 'REMOTE' as the remote file system";\
	    $(ECHO) "for this particular command. The easiest way to do this";\
	    $(ECHO) "to type:";\
	    $(ECHO) "        make REMOTE=<remote file system> rmt-install";\
	    exit 1;\
	  fi);

#	Dependencies and rules for installing man pages and lib files
$(MAN)/answer$(MANEXT):	answer.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/autoreply$(MANEXT):	autoreply.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/checkalias$(MANEXT):	chkalias.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/elm$(MANEXT):		elm.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/elmalias$(MANEXT):	elmalias.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/fastmail$(MANEXT):	fastmail.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/filter$(MANEXT):	filter.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/frm$(MANEXT):		frm.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/listalias$(MANEXT):	listalias.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/messages$(MANEXT):	messages.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/newalias$(MANEXT):	newalias.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/newmail$(MANEXT):	newmail.1
				$(RM) $@
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/printmail$(MANEXT):	printmail.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/readmsg$(MANEXT):	readmsg.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(MAN)/wnewmail$(MANEXT):	$(MAN)/newmail$(MANEXT)
				$(RM) $@
				$(LN) $? $@

$(LIB)/elm-help.0:		elm-help.0
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(LIB)/elm-help.1:		elm-help.1
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(LIB)/elm-help.2:		elm-help.2
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(LIB)/elm-help.3:		elm-help.3
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(LIB)/elmrc-info:		elmrc-info
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

#	Dependencies and rules for installing catman
$(CATMAN)/answer$(CATMANEXT)$(SUFFIX):	catman/answer$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/autoreply$(CATMANEXT)$(SUFFIX): catman/autoreply$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/checkalias$(CATMANEXT)$(SUFFIX): catman/checkalias$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/elm$(CATMANEXT)$(SUFFIX): catman/elm$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/elmalias$(CATMANEXT)$(SUFFIX): catman/elmalias$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/fastmail$(CATMANEXT)$(SUFFIX): catman/fastmail$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/filter$(CATMANEXT)$(SUFFIX): catman/filter$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/frm$(CATMANEXT)$(SUFFIX): catman/frm$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/listalias$(CATMANEXT)$(SUFFIX): catman/listalias$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/messages$(CATMANEXT)$(SUFFIX): catman/messages$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/newalias$(CATMANEXT)$(SUFFIX): catman/newalias$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/newmail$(CATMANEXT)$(SUFFIX): catman/newmail$(CATMANEXT)$(SUFFIX)
				$(RM) $@
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/printmail$(CATMANEXT)$(SUFFIX): catman/printmail$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/readmsg$(CATMANEXT)$(SUFFIX): catman/readmsg$(CATMANEXT)$(SUFFIX)
				$(CP) $? $@
				$(CHMOD) u=rw,go=r $@

$(CATMAN)/wnewmail$(CATMANEXT)$(SUFFIX): $(CATMAN)/newmail$(CATMANEXT)$(SUFFIX)
				$(RM) $@
				$(LN) $? $@


#	Dependencies and rules for making catman pages
catman/answer$(CATMANEXT)$(SUFFIX):		answer.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/answer$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/answer$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/autoreply$(CATMANEXT)$(SUFFIX):		autoreply.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/autoreply$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/autoreply$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/checkalias$(CATMANEXT)$(SUFFIX):		chkalias.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/checkalias$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/checkalias$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/elm$(CATMANEXT)$(SUFFIX):		elm.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/elm$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/elm$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/elmalias$(CATMANEXT)$(SUFFIX):		elmalias.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/elmalias$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/elmalias$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/fastmail$(CATMANEXT)$(SUFFIX):		fastmail.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/fastmail$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/fastmail$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/filter$(CATMANEXT)$(SUFFIX):		filter.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/filter$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/filter$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/frm$(CATMANEXT)$(SUFFIX):		frm.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/frm$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/frm$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/listalias$(CATMANEXT)$(SUFFIX):		listalias.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/listalias$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/listalias$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/messages$(CATMANEXT)$(SUFFIX):		messages.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/messages$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/messages$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/newalias$(CATMANEXT)$(SUFFIX):		newalias.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/newalias$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/newalias$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/newmail$(CATMANEXT)$(SUFFIX):		newmail.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/newmail$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/newmail$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/printmail$(CATMANEXT)$(SUFFIX):		printmail.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/printmail$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/printmail$(CATMANEXT) ; \
	else \
		true ; \
	fi

catman/readmsg$(CATMANEXT)$(SUFFIX):		readmsg.1
	$(MANROFF) $(MANROFF_OPTS) -man $? > catman/readmsg$(CATMANEXT)
	@if $(TEST) "$(PACKED)" = y ; then \
		$(RM) $@ ; \
		$(PACKER) catman/readmsg$(CATMANEXT) ; \
	else \
		true ; \
	fi


#	Dependencies and rules for formatting guides
documentation:	$(FORMATTED_LIST)

Alias.fmtd:	Alias.guide
		$(TBL) $? | $(FORMATTER) $(FORMATTER_OPTS) tmac.n - > $@

Config.fmtd:	Config.guid
		$(TBL) $? | $(FORMATTER) $(FORMATTER_OPTS) tmac.n - > $@

Cover.fmtd:	Elm.cover
		$(FORMATTER) $(FORMATTER_OPTS) tmac.n $? > $@

Filter.fmtd:	Filter.guid
		$(TBL) $? | $(FORMATTER) $(FORMATTER_OPTS) tmac.n - > $@

Form.fmtd:	Form.guide
		$(FORMATTER) $(FORMATTER_OPTS) tmac.n $? > $@

Ref.fmtd:	Ref.guide
		$(FORMATTER) $(FORMATTER_OPTS) tmac.n $? > $@

Users.fmtd:	Users.guide
		$(TBL) $? | $(FORMATTER) $(FORMATTER_OPTS) tmac.n - > $@

!NO!SUBS!
