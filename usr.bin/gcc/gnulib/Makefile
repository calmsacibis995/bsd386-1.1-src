#	BSDI $Id: Makefile,v 1.3 1993/02/05 23:58:52 polk Exp $
#	@(#)Makefile	6.4 (Berkeley) 1/30/91

LIB=		gnulib
CC=		GCC_EXEC_PREFIX=$(.CURDIR)/../cpp/obj/ \
		$(.CURDIR)/../cc/obj/cc -B$(.CURDIR)/../cc1/obj/
CFLAGS+=	-I. -I$(.CURDIR)/../cc1 -I$(.CURDIR)/../cc1/config
NOMAN=		noman
.PATH: $(.CURDIR)/$(MACHINE) $(.CURDIR)/g++ $(.CURDIR)/longlong

.include "$(MACHINE)/Makefile.machine"
GPLUS_SRCS=	builtin_New.c builtin_del.c builtin_new.c eprintf.c
SRCS=		$(MACHINE_SRCS) $(GPLUS_SRCS)

$(OBJS):	config.h

afterinstall:
	-rm -f $(DESTDIR)$(LIBDIR)/gcc-gnulib
	cd $(DESTDIR)$(LIBDIR); ln -s libgnulib.a gcc-gnulib

.include <bsd.lib.mk>
.include "../Makefile.symlinks"

.depend: config.h
