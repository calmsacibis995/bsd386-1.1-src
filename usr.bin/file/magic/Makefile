#	BSDI $Id: Makefile,v 1.3 1994/01/21 19:17:28 donn Exp $

#
# Why .m?  We want 'make' to check the obj directory for locally
# modified versions of files before checking ${.CURDIR}.  If the
# files in the list don't have a legal suffix, ${.ALLSRC} won't
# contain the expanded name.  I'm not sure whether this is a bug or
# a feature.
#
.SUFFIXES:	.m

FILES=	Header.m Localstuff.m alliant.m apl.m apple.m ar.m arc.m \
	archive.m att3b.m audio.m blit.m bsdi.m c-lang.m chi.m \
	clipper.m commands.m compress.m convex.m cpio.m diamond.m \
	diff.m ditroff.m dump.m elf.m encore.m floppy.raw.m fonts.m \
	frame.m gzip.m hp.m ibm370.m ibm6000.m iff.m imagen.m \
	images.m intel.m interleaf.m iris.m ispell.m lex.m lif.m \
	linux.m magic.m mail.news.m microsoft.m mips.m mirage.m \
	mkid.m mmdf.m motorola.m ms-dos.m ncr.m netbsd.m news.m \
	pbm.m pdp.m pgp.m pkgadd.m plus5.m postscript.m psdbms.m \
	pyramid.m rle.m sc.m sccs.m sendmail.m sequent.m sgml.m \
	softquad.m sun.m sunraster.m terminfo.m tex.m troff.m \
	typeset.m unknown.m uuencode.m varied.out.m vax.m visx.m \
	x11.m zilog.m zyxel.m

MAN5=	magic.0

all:	magic ${MAN5}

magic:	${FILES}
	rm -f magic
	cat ${.ALLSRC} > magic

clean:
	rm -f magic

cleandir: clean
	rm -f ${MAN5}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 magic \
	     ${DESTDIR}/usr/share/misc/magic

.PATH:		${.CURDIR}

.include <bsd.prog.mk>
