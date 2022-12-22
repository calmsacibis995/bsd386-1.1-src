#  Makefile.st	Makefile for UnZip 4.0,
#  using Turbo C 2.0 for the Atari ST and
#  make from the Mark Williams C 3.9 (sorry for that mixture)
#

# UPPERCASE file names facilitate TD's understanding of modules.
.SUFFIXES: .o .O .c .C .s

TCHOME	= E:\tc

#####################
# MACRO DEFINITIONS #
#####################

CC	= tcc
#CFLAGS	= -DATARI_ST=1 -Y -I=$(TCHOME)\include
CFLAGS	= -DATARI_ST=1 -I=$(TCHOME)\include
#LFLAGS	= -L -Y
LFLAGS	=
LIB	= $(TCHOME)\lib
LD	= tlink

OBJS = unzip.o file_io.o mapname.o match.o misc.o\
       unimplod.o unreduce.o unshrink.o


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

ALL	: unzip.prg
	echo done

unzip.o:      unzip.c unzip.h

file_io.o:    file_io.c unzip.h

mapname.o:    mapname.c unzip.h

match.o:      match.c unzip.h

misc.o:       misc.c unzip.h

unimplod.o:   unimplod.c unzip.h

unreduce.o:   unreduce.c unzip.h

unshrink.o:   unshrink.c unzip.h

unzip.prg:     $(OBJS)
	tlink -O=$@ $(LFLAGS) -S=8192 -C=unzip.lnk

.c.o:
	tcc $(CFLAGS) $<

