#============================================================================
# Makefile for UnZip & ZipInfo:  VMS                             Greg Roelofs
# Version:  5.0 (inflate,explode)                                  7 May 1992
#============================================================================

# Most recent revisions:  26 June 1992


#####################
# MACRO DEFINITIONS #
#####################

CRYPTF =
CRYPTO =
# Uncomment next two lines for decryption version:
#CRYPTF = /def=(CRYPT)
#CRYPTO = crypt.obj,

CC = cc
CFLAGS = $(CRYPTF)			# use "cc $(CFLAGS)" this way
# CFLAGS := /def=(FLAG1,FLAG2)		# this also works, if use "cc 'CFLAGS'"
ZCFLAGS = /def=(ZIPINFO)
LD = link
LDFLAGS =

LN = copy
RM = delete
E = .exe
O = .obj

OBJS = unzip$O, $(CRYPTO) envargs$O, explode$O, extract$O, file_io$O,-
   inflate$O, mapname$O, match$O, misc$O, unreduce$O, unshrink$O,-
   vms$O, sys$input:/opt
ZI_OBJS = zipinfo$O, envargs$O, match$O, misc$O_, vms$O_, sys$input:/opt


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

default:	unzip.exe zipinfo.exe

*.obj:	*.c				# `*.c' necessary?
	$(CC) $(CFLAGS) $<

*.obj_:	*.c				# `$*' not legal
	$(CC) $(ZCFLAGS) $< /obj=$@

unzip$E:	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS)	# next line is data line
	sys$share:vaxcrtl.exe/shareable $

zipinfo$E:	$(ZI_OBJS)
	$(LD) $(LDFLAGS) $(ZI_OBJS)	# next line is data line
	sys$share:vaxcrtl.exe/shareable $

crypt$O:        crypt.c unzip.h zip.h	# may or may not be in distribution
envargs$O:      envargs.c unzip.h
explode$O:      explode.c unzip.h
extract$O:      extract.c unzip.h
file_io$O:      file_io.c unzip.h
inflate$O:      inflate.c unzip.h
mapname$O:      mapname.c unzip.h
match$O:        match.c unzip.h
misc$O:         misc.c unzip.h
misc$O_:	misc.c unzip.h
unreduce$O:     unreduce.c unzip.h
unshrink$O:     unshrink.c unzip.h
unzip$O:        unzip.c unzip.h
vms$O:          vms.c unzip.h
vms$O_:         vms.c unzip.h
VMSmunch$O:     VMSmunch.c VMSmunch.h
zipinfo$O:	zipinfo.c unzip.h

# system: ${SYSDIR}make.exe ${SYSDIR}make.ini
# debug:
# 	cflags := /debug/noopt
# 	lflags := /debug
# ${DISTDIR}unzip.exe: unzip.exe
#	copy unzip.exe ${DISTDIR}/log

clean:
	purge/log
#	purge/log ${DISTDIR}


# the backslash '\' is the continuation character if it occurs as
# the last non-white character on the line.
# the hyphen '-' is the DCL continuation character, so if it occurs
# as the last non-white character on the line, the next line will
# not have the dollar sign '$' prepended.


################################
# INDIVIDUAL MACHINE MAKERULES #
################################

generic:	default		# first try if unknown
generic2:	default		# second try if unknown
vax:		default
vms:		default

both:		unzip.exe zipinfo.exe
unzip:		unzip.exe
zipinfo:	zipinfo.exe
