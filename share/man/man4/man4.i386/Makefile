#	BSDI $Id: Makefile,v 1.14 1994/01/31 08:41:38 donn Exp $
#	@(#)Makefile	5.1 (Berkeley) 2/12/91

MAN4=	aha.0 autoconf.0 bms.0 com.0 cons.0 digi.0 eaha.0 ef.0 el.0 \
	eo.0 ep.0 ex.0 floppy.0 hpp.0 intro.0 lp.0 mcd.0 mem.0 \
	midi.0 ms.0 ne.0 pcaux.0 pccons.0 rc.0 re.0 rh.0 rn.0 si.0 \
	tn.0 vga.0 wd.0 we.0 wt.0
MLINKS+=cons.4 console.4
MLINKS+=mem.4 kmem.4
MLINKS+=pccons.4 kbd.4 pccons.4 pcscreens.4
MANSUBDIR=/i386

all: ${MAN4}

midi.0:	midi.4
	tbl ${.CURDIR}/midi.4 | nroff -mandoc > midi.0

clean depend lint tags:

cleandir:
	rm -f ${MAN4}

install: maninstall

.include <bsd.prog.mk>
