#!smake
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/man/Makefile,v 1.1.1.1 1994/01/14 23:10:09 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
# Copyright (c) 1991, 1992 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#
DEPTH=..
include ${DEPTH}/defs

CLIENTAPPS=flexfax.1 fax2ps.1 faxalter.1 faxcover.1 faxmail.1 faxrm.1 \
	faxstat.1 sendfax.1 sgi2fax.1 textfmt.1
CLIENTFILES=pagesizes.4f typerules.4f
SERVERAPPS=dialtest.1m faxaddmodem.1m faxanswer.1m faxd.1m faxd.recv.1m \
	faxquit.1m faxrcvd.1m notify.1m pollrcvd.1m ps2fax.1m recvstats.1m \
	transcript.1m xferstats.1m
SERVERFILES=intro.4f cinfo.4f config.4f dialrules.4f hosts.4f info.4f \
	log.4f recvq.4f sendq.4f status.4f tsi.4f xferlog.4f
#
# The defs file defines these macros to reflect whether
# or not to install pre-formatted manual pages or not.
#
PAGES=	${MANCAPP} ${MANCFILE} ${MANSAPP} ${MANSFILE}
LDIRT=	${PAGEDIRT}

#
# System-specific manual page formatting commands should
# apply this sed command to the manual page source before
# handing it to the formatter.  This sets any pathnames
# in the man pages to reflect local conventions.
#
MANSED=${SED}\
	-e 's;\$${SPOOL};${SPOOL};g'\
	-e 's;\$${BIN};${BIN};g'\
	-e 's;\$${LIBDATA};${LIBDATA};g'\
	-e 's;\$${LIBEXEC};${LIBEXEC};g'\
	-e 's;\$${USRETC};${USRETC};g'\
	-e 's;\$${AFMDIR};${AFMDIR};g'\
	${NULL}

default all targets:: ${PAGES}

include ${COMMONRULES}

installClientManPages:: ${MANCAPP} ${MANCFILE}
	-${INSTALL} -m 755 -dir -idb flexfax.man.client \
	    ${MAN}/${MANAPPS} ${MAN}/${MANFILES}
	${INSTALL} -m 444 -F ${MAN}/${MANAPPS} \
	    -idb ${PRODUCT}.man.client -O ${MANCAPP}
	${INSTALL} -m 444 -F ${MAN}/${MANFILES} \
	    -idb ${PRODUCT}.man.client -O ${MANCFILE}

installServerManPages:: ${MANSAPP} ${MANSFILE}
	-${INSTALL} -m 755 -dir -idb flexfax.man.server \
	    ${MAN}/${MANSYS} ${MAN}/${MANFILES}
	${INSTALL} -m 444 -F ${MAN}/${MANSYS} \
	    -idb ${PRODUCT}.man.server -O ${MANSAPP}
	${INSTALL} -m 444 -F ${MAN}/${MANFILES} \
	    -idb ${PRODUCT}.man.server -O ${MANSFILE}

install: installClientManPages installServerManPages
