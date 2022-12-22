#	BSDI $Id: Makefile.inc,v 1.1 1993/12/16 00:55:51 torek Exp $

# LIBEXECDIR is where cc1, cc1plus, cpp, etc, are found.
LIBEXECDIR?=	/usr/libexec/gcc2
BINDIR?=	/usr/bin

# Build gcc2 using gcc2, rather than gcc1 -- needed until
# gcc2 is the default compiler.
CC=gcc2
CFLAGS+=-O2
