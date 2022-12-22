
# Sound Tools Makefile
# 	builds libst.a and sox

FSRC= raw.c voc.c au.c sf.c aiff.c hcom.c 8svx.c sndrtool.c wav.c sbdsp.c
ESRC= copy.c avg.c pred.c stat.c vibro.c echo.c rate.c band.c lowp.c 
PSRC= sox.c

SOURCES   = $(PSRC) $(FSRC) $(ESRC) handlers.c libst.c misc.c getopt.c

HEADERS   = st.h libst.h sfheader.h patchlevel.h 

TESTS     = tests.sh monkey.au monkey.voc

MISC      = README INSTALL TODO sox.man st.man Makefile

SKEL	  = skel.c skeleff.c

FILES     = $(MISC) $(HEADERS) $(SOURCES) $(SKEL) $(TESTS)

FOBJ= raw.o voc.o au.o sf.o aiff.o hcom.o 8svx.o sndrtool.o wav.o sbdsp.o
EOBJ= copy.o avg.o pred.o stat.o vibro.o echo.o rate.o band.o lowp.o 

SOUNDLIB  = libst.a
LIBOBJS   = $(FOBJ) $(EOBJ) handlers.o libst.o misc.o getopt.o

#
# System dependency parameters
# 	not many, we don't mess with devices
#
# include -DSYSV for AT&T Unix System V
#
# define -DDOS  to compile on PC
# defines .snd to mean a DOS soundtool file (starts with SOUND)

# define -DNeXT on a NeXT workstation
# defines .snd to mean a NeXT sound format file
#  	only knows a few real sound formats.

# define -DMAC on a MacIntosh
# defines .snd to mean a Mac-style headerless unsigned byte
#  	sample, probably at 11050 hertz.  You'll have to set 
#	the speed on the command line.


# AT&T System V/386 parameters, salt to taste
# Most System V machines should use these flags

O=-g -O		# optimization flags

#CFLAGS    = $O -DSYSV 
#CC        = cc
#AR        = ar r
#RANLIB    = ar ts

# BSD-ish, salt to taste
# Sun, NeXT, Vax, Ultrix uses these

# CC        = /usr/local/bin/gcc2.3.3
CFLAGS    = $O -I/home/sanders/sys -D__bsdi__
CC        = cc
AR        = ar r
RANLIB    = ranlib

# DOS? Mac? They generally have their own makefile formats.
# I think you'll have to write one from scratch.

# For DOS, you'll need -DDOS for all files.

all: sox

sox: sox.o $(SOUNDLIB)
	$(CC) $(CFLAGS) -o sox sox.o $(SOUNDLIB) -lm

$(SOUNDLIB): $(LIBOBJS)
	rm -f $(SOUNDLIB)
	$(AR) $(SOUNDLIB) $(LIBOBJS)
	$(RANLIB) $(SOUNDLIB)

sox.o:		sox.c st.h

$(LIBOBJS):	st.h

clean:
	rm -f *.o
	rm -f *.raw
	rm -f *.sf
	rm -f core

# Shar: -F (prefix all lines with X), 
# 	-s addr (set return addr of poster)
shar: $(FILES)
	/usr2/tools/shar/shar -M -F -l 50 -o shar -n soundtools_v5 -s thinman@netcom.com $(FILES)
