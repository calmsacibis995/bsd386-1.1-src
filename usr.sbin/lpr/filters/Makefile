#	@(#)Makefile	5.9 (Berkeley) 5/13/90

PROG=	lpf
NOMAN=	noman
BINDIR=	/usr/libexec/lpr

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/txt2jep-filter.sh $(DESTDIR)$(BINDIR)/txt2jep-filter
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/dvi2jep-filter.sh $(DESTDIR)$(BINDIR)/dvi2jep-filter
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/ps2jep-filter.sh $(DESTDIR)$(BINDIR)/ps2jep-filter
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/txt2ps-filter.sh $(DESTDIR)$(BINDIR)/txt2ps-filter
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/dvi2ps-filter.sh $(DESTDIR)$(BINDIR)/dvi2ps-filter

.include <bsd.prog.mk>
