# From: Richard A Seay <g0rick@cdf.toronto.edu>, 4 Apr 92
#
# I successfully compiled both Zip and UnZip for MSDOS using the Microsoft
# Quick C 2.0 compiler and linker.  Here are the NMAKE files that I used.

PROJ	=UNZIP
CC	=qcl
CFLAGS  = /AS /W1 /Za /Olt
LFLAGS  = /NOI /E
OBJS_EXT =
LIBS_EXT =

all:	$(PROJ).exe

unzip.obj:      unzip.c unzip.h

envargs.obj:    envargs.c unzip.h

explode.obj:    explode.c unzip.h

extract.obj:    extract.c unzip.h

file_io.obj:    file_io.c unzip.h

inflate.obj:    inflate.c unzip.h

mapname.obj:    mapname.c unzip.h

match.obj:      match.c unzip.h

misc.obj:       misc.c unzip.h

unreduce.obj:   unreduce.c unzip.h

unshrink.obj:   unshrink.c unzip.h

$(PROJ).exe:	unzip.obj envargs.obj explode.obj extract.obj file_io.obj\
	inflate.obj mapname.obj match.obj misc.obj unreduce.obj unshrink.obj\
	$(OBJS_EXT)
	echo >NUL @<<$(PROJ).crf
unzip.obj +
envargs.obj +
explode.obj +
extract.obj +
file_io.obj +
inflate.obj +
mapname.obj +
match.obj +
misc.obj +
unreduce.obj +
unshrink.obj +
$(OBJS_EXT)
$(PROJ).exe
$(LIBS_EXT);
<<
	link $(LFLAGS) @$(PROJ).crf
	del $(PROJ).crf
