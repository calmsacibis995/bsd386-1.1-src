#
# Makefile to generate PostScript from psgraph examples
#

# Where to find the psgraph and psgsimp programs
PSGRAPH = psgraph
PSGSIMP = psgsimp

PLOTS = rbi.ps rbi2.ps wire-fusing.ps oneconn.ps distribb.ps long.ps \
	shorter.ps simplecolor.ps

# People like to use different suffixes
.SUFFIXES: .ps .grf .plot .data

.data.ps:
	$(PSGRAPH) -P <$*.data >$*.ps

.plot.ps:
	$(PSGRAPH) -P <$*.plot >$*.ps

.grf.ps:
	$(PSGRAPH) -P <$*.grf >$*.ps


all: $(PLOTS)

clean:
	rm -f $(PLOTS) shorter.plot

shorter.plot: long.plot Makefile
	$(PSGSIMP) 0.01 <long.plot |grep -v '^#' >shorter.plot
	wc long.plot shorter.plot
