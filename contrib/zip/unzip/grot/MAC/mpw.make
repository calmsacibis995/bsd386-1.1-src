#   This MPW makefile is designed to be used to compile an MPW version
#   of unzip using the MPW C compiler, version 3.2. Simply rename
#   this file as unzip.make and do an MPW build.


#   File:       unzip.make
#   Target:     Unzip
#   Sources:    unzip.c
#               crypt.c
#               envargs.c
#               explode.c
#               extract.c
#               file_io.c
#               inflate.c
#               macfile.c
#               macstat.c
#               mapname.c
#               match.c
#               misc.c
#               unreduce.c
#               unshrink.c
#   Created:    Friday, May 9, 1992 9:00:00 PM


CFLAGS = -d MPW # -d CRYPT

LFLAGS = -m


.c.o Ä .c unzip.h unzip.make
        C {CFLAGS} {Default}.c

OBJECTS = ¶
        unzip.c.o ¶
#       crypt.c.o ¶
        envargs.c.o ¶
        explode.c.o ¶
        extract.c.o ¶
        file_io.c.o ¶
        inflate.c.o ¶
        macfile.c.o ¶
        macstat.c.o ¶
        mapname.c.o ¶
        match.c.o ¶
        misc.c.o ¶
        unreduce.c.o ¶
        unshrink.c.o

Unzip Ä {OBJECTS} unzip.r
        Link -d -c 'MPS ' -t MPST ¶
                {OBJECTS} ¶
                "{CLibraries}"StdClib.o ¶
                "{Libraries}"Stubs.o ¶
                "{Libraries}"Runtime.o ¶
                "{Libraries}"Interface.o ¶
                -o Unzip
        rez -append -o Unzip unzip.r

unzip.r Ä unzip.thinkc.rsrc
        derez unzip.thinkc.rsrc > unzip.r
