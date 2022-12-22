# This Makefile is for the Bash/documentation directory -*- text -*-.
#
CP	    = cp

DOC_SUPPORT = ../lib/doc-support/
TEXINDEX    = $(DOC_SUPPORT)texindex
TEX	    = tex
MAKEINFO    = makeinfo

ps: bash.ps bash_builtins.ps
all: features.dvi features.info bash.txt

features.dvi: $(TEXINDEX) features.texi hsuser.texinfo rluser.texinfo
	$(TEX) features.texi
	$(TEXINDEX) features.??
	$(TEX) features.texi

dvi: features.dvi

features.info: features.texi
	$(MAKEINFO) -I../lib/readline/doc features.texi

info: features.info

bash.dvi: $(TEXINDEX) bash.texinfo hsuser.texinfo rluser.texinfo
	$(TEX) bash.texinfo
	$(TEXINDEX) bash.??
	$(TEX) bash.texinfo

bash.txt: bash.1
	nroff -man bash.1 >bash.txt

bash.ps: bash.1
	groff -man bash.1 > $@

bash_builtins.ps: bash_builtins.1 bash.1
	groff -man bash_builtins.1 > $@

$(TEXINDEX):
	(cd $(DOC_SUPPORT); $(MAKE) $(MFLAGS) CFLAGS='$(CFLAGS)' texindex)

hsuser.texinfo: ../lib/readline/doc/hsuser.texinfo
	ln -s ../lib/readline/doc/hsuser.texinfo .

rluser.texinfo: ../lib/readline/doc/rluser.texinfo
	ln -s ../lib/readline/doc/rluser.texinfo .

clean:
	rm -f *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	*.fns *.kys *.tps *.vrs *.o core texindex rluser.texinfo hsuser.texinfo

distclean:
	rm -f *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	*.dvi *.info *.info-* *.fns *.kys *.tps *.vrs *.o core texindex \
	rluser.texinfo hsuser.texinfo

realclean:	clean

install: features.info
	$(CP) bash.1 $(mandir)
	sed 's:so bash.1:so man1/bash.1:' < bash_builtins.1 > $(mandir)/bash_builtins.1
	$(CP) features.info $(infodir)/bash.info
