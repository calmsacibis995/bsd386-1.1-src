#   This MPW makefile is designed to be used to compile an MPW version
#   of unzip using the Aztec C compiler, version 5.2a. Simply rename
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
#   Created:    Tuesday, May 5, 1992 7:05:00 PM


CFLAGS = -d MPW # -d CRYPT

LFLAGS = -m


.o Ä .c unzip.h unzip.make
        C {CFLAGS} -o {Default}.o {Default}.c

.o Ä .asm
        as -o {Default}.o {Default}.asm

OBJECTS = ¶
        unzip.o ¶
#       crypt.o ¶
        envargs.o ¶
        explode.o ¶
        extract.o ¶
        file_io.o ¶
        inflate.o ¶
        macfile.o ¶
        macstat.o ¶
        mapname.o ¶
        match.o ¶
        misc.o ¶
        unreduce.o ¶
        unshrink.o

Unzip Ä {OBJECTS} unzip.r
        ln {LFLAGS} -o Unzip {OBJECTS} -lm -lmpw -lc
        rez -append -o Unzip unzip.r

unzip.r Ä unzip.thinkc.rsrc
        derez unzip.thinkc.rsrc > unzip.r
