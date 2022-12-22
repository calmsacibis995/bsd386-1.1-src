#	BSDI $Id: Makefile,v 1.4 1993/06/03 17:43:30 polk Exp $

PROG=		gdb
GDBSRCS=	blockframe.c breakpoint.c buildsym.c c-exp.y \
		cmdparse.c command.c copying.c xcore.c cplus-dem.c \
		environ.c eval.c exec.c expprint.c findvar.c \
		gdbtypes.c infcmd.c inflow.c infptrace.c infrun.c \
		inftarg.c language.c main.c mem-break.c minsyms.c \
		objfiles.c parse.c printcmd.c regex.c signame.c \
		source.c stack.c symfile.c symmisc.c symtab.c \
		target.c utils.c valarith.c valops.c valprint.c \
		values.c version.c
KGDBSRCS=	remote.c kcore.c kernel.c
READLINESRCS=	readline.c history.c funmap.c keymaps.c vi_mode.c
BFDSRCS=        bsdaout.c aout32.c archive.c archures.c bfd.c \
		bfdcore.c bfdinit.c cache.c format.c libbfd.c \
		opncls.c reloc.c section.c stab-syms.c syms.c \
		targets.c trad-core.c
IBERTYSRCS=	argv.c basename.c concat.c getopt.c getopt1.c obstack.c 
GLOBSRCS=	tilde.c
MMALLOCSRCS=    attach.c detach.c keys.c mfree.c mmalloc.c \
		mmap-sup.c mmcheck.c mmemalign.c mmstats.c \
		mrealloc.c sbrk-sup.c
SRCS=		$(CONFIGSRCS) $(GDBSRCS) $(KGDBSRCS) $(READLINESRCS) \
		$(BFDSRCS) $(IBERTYSRCS) $(GLOBSRCS) $(MMALLOCSRCS) init.c
INCLUDES=	-I. -I$(.CURDIR) -I$(.CURDIR)/config -I$(.CURDIR)/../readline \
		-I$(.CURDIR)/../include -I$(.CURDIR)/../bfd \
		-I$(.CURDIR)/../mmalloc
CFLAGS+=	$(INCLUDES) -DVI_MODE -DKERNELDEBUG -DHAVE_MMAP
LDADD=		-ltermcap -lkvm
YFLAGS=
.PATH: $(.CURDIR)/config $(.CURDIR)/../readline $(.CURDIR)/../glob \
	$(.CURDIR)/../libiberty $(.CURDIR)/../bfd $(.CURDIR)/../mmalloc

# no xgdb for now
#SUBDIR=		xgdb

#depend:
.depend: init.c tm.h xm.h sysdep.h

.include "../../Makefile.inc"
.include "config/Makefile.$(MACHINE)"
.include <bsd.prog.mk>

$(CONFIGSRCS:R:S/$/.o/g) $(GDBSRCS:R:S/$/.o/g) $(KGDBSRCS:R:S/$/.o/g): \
	tm.h xm.h
$(BFDSRCS:R:S/$/.o/g) $(READLINESRCS:R:S/$/.o/g):	sysdep.h

CLEANFILES+=	init.c tm.h xm.h sysdep.h
