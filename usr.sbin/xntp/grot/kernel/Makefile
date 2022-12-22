#
# /src/NTP/REPOSITORY/v3/kernel/Makefile.tmpl,v 3.4 1993/11/05 23:51:26 kardel Exp
#
#
# parse routine that could be used in two places
#
COMPILER= gcc
COPTS=      -g
AUTHDEFS=  -DDES -DMD5
LIBDEFS= -DXNTP_LITTLE_ENDIAN
RANLIB=	    ranlib
INSTALL= install
CLOCKDEFS=  -DLOCAL_CLOCK #TEST -DAS2201PPS -DCHU -DGOES -DIRIG -DMX4200PPS -DOMEGA -DPST -DPSTCLK -DTPRO -DWWVBCLK -DMSFEESPPS -DLEITCH -DPARSE -DPARSEPPS -DCLOCK_MEINBERG -DCLOCK_RAWDCF -DCLOCK_SCHMID -DCLOCK_DCF7000 -DCLOCK_TRIMSV6
DEFS=      -DSYS_BSDI
DEFS_OPT=-DDEBUG
DEFS_LOCAL= $(DEFS_OPT)  -DREFCLOCK #TEST -DPPSPPS -DKERNEL_PLL
#
INCL=-I../include
CFLAGS=	$(COPTS) $(DEFS) $(DEFS_LOCAL) $(INCL)
CC=		$(COMPILER)
#

all:
	@echo $(DEFS) $(DEFS_LOCAL) $(CLOCKDEFS) | \
		awk '/-DREFCLOCK/ && ( /-D.*CLK/ || /-D.*PPS/ ) { makeit=1; }\
			END { if (makeit) \
				{ print ""; \
				  print "### STREAMS kernel modules ppsclock, ppsclocd or line disciplines must"; \
			 	  print "### be installed manually if needed"; \
				  print "### see kernel/README* for details"; \
				  print "### The parse reclock implementation has their own support in"; \
				  print "### parse/*."; } }'

clean:
	-@rm -f *~ *.o *.out *.ln make.log Makefile.bak \
		lintlib.errs lint.errs

distclean:	clean
	-@rm -f *.orig *.rej .version Makefile
