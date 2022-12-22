#	BSDI $Id: Makefile,v 1.5 1994/01/08 19:38:58 donn Exp $
#
# Copyright (c) 1992 Keith Muller.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by Keith Muller.
# 4. The name of the developer may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# DEVELOPER (KEITH MULLER) BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#       @(#)Makefile	1.3 (BSDI) 4/12/93
#
# Makefile for Pax
#
#	To install on versions prior to BSD 4.4 the following may have to be
#	defined with CFLAGS +=
#
#	-DNET2_STAT	Use NET2 or older stat structure. The version of the
#			stat structure is easily determined by looking at the
#			basic type of an off_t (often defined in the file:
#			/usr/include/sys/types.h). If off_t is a long (and is
#			NOT A quad) then you must define NET2_STAT.
#			This define is important, as if you do have a quad_t
#			off_t and define NET2_STAT, pax will compile but will
#			NOT RUN PROPERLY.
#
#	-DNET2_FTS	Use the older NET2 fts. To identify the version,
#			examine the file: /usr/include/fts.h. If FTS_COMFOLLOW
#			is not defined then you must define NET2_FTS.
#			Pax may not compile if this not (un)defined properly.
#
#	-DNET2_REGEX	Use the older regexp.h not regex.h. The regex version
#			is determined by looking at the value returned by
#			regexec() (man 3 regexec). If regexec return a 1 for
#			success (and NOT a 0 for success) you have the older
#			regex routines and must define NET2_REGEX.
#			Pax may not compile if this not (un)defined properly.

PROG=   pax
SRCS=	ar_io.c ar_subs.c buf_subs.c cache.c cpio.c file_subs.c ftree.c\
	gen_subs.c options.c pat_rep.c pax.c sel_subs.c tables.c tar.c\
	tty_subs.c

MAN1=	pax.0 tar.0
LINKS=	${BINDIR}/pax ${BINDIR}/tar
MLINKS=	pax.1 cpio.1

CFLAGS+=-DNET2_STAT

.include <bsd.prog.mk>
