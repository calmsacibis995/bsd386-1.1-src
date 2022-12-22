#	BSDI $Id: Makefile,v 1.2 1993/12/20 17:48:22 donn Exp $

# use -traditional -Dconst= until function definitions become ANSI C compliant
CFLAGS+=        -traditional -Dconst= -DFONTPATH="\"/usr/contrib/lib/tex/fonts/latexfonts/:/usr/contrib/lib/tex/fonts/cmfonts:/usr/contrib/lib/tex/fonts/utilityfonts\"" -DSUBPATH="\"/usr/contrib/lib/tex/inputs/\""

PROG=           dvijep
SRCS=		dvijep.c fndfil.c 

LDADD=-lm

.include <bsd.prog.mk>
