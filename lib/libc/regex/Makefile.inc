#	BSDI $Id: Makefile.inc,v 1.2 1993/11/12 10:50:10 donn Exp $
#	@(#)Makefile.inc	5.1 (Berkeley) 8/6/92

# regex sources
.PATH: ${.CURDIR}/regex

CFLAGS+=-DPOSIX_MISTAKE

SRCS+=	regcomp.c regerror.c regexec.c regfree.c

MAN3+=	regex.0
MAN7+=	re_format.0

MLINKS+=regex.3 regcomp.3 regex.3 regexec.3 regex.3 regerror.3
MLINKS+=regexec.3 regfree.3
MLINKS+=re_format.7 regex.7
