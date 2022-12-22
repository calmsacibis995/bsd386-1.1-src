#------------------------------------------------------------------------------
# Makefile for UnZip 5.x and ZipInfo 1.x                Greg Roelofs and others
# Version:  Microsoft C 5.x / Turbo C                              24 June 1992
#------------------------------------------------------------------------------

# Comment/uncomment appropriate sections for your compiler.  Users of MSC 6
# and NMAKE should use the main Makefile, targets msc_dos and zi_dos.
#
# Latest revisions:  26 June 1992


#####################
# MACRO DEFINITIONS #
#####################

CRYPTF =
CRYPTO =
# Uncomment the following two lines for decryption version:
#CRYPTF = -DCRYPT
#CRYPTO = crypt.obj

STRIP=rem
#    If you don't have lzexe, get it. Then define:
#STRIP=lzexe
#    and remove /e from LDFLAGS
#    This makes a big difference in .exe size (and possibly load time).


# MSC for MS-DOS:
# --------------
CC = cl
CFLAGS = -AS -Oait -Gs -G2 $(CRYPTF)   # add -G2 and/or -FPi87 for 80286/80x87
INCL =                                 # (-Ox does not work for inflate.c)
LD = link
LDFLAGS = /NOI/e/st:0x1000
# remove /e in above line if you have lzexe
LDFLAGS2 = ,$*;

# Turbo C 2.0 for MS-DOS:
# ----------------------
## tcc is usually configured with -I and -L set appropriately...
#CC = tcc
#CFLAGS = -ms -O -Z $(CRYPTF)           # add -1 for 80286 instructions
#INCL = #-Ic:\turboc\include
#LD = tcc
#LDFLAGS = -ms #-Lc:\turboc\lib
#LDFLAGS2 =


OBJS1 = unzip.obj $(CRYPTO) envargs.obj explode.obj extract.obj file_io.obj
OBJS2 = inflate.obj mapname.obj match.obj misc.obj unreduce.obj unshrink.obj

ZI_OBJS = zipinfo.obj envargs.obj match.obj misc_.obj


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

default:        unzip.exe zipinfo.exe

.c.obj:
        $(CC) -c $(CFLAGS) $(INCL) $*.c

unzip.obj:      unzip.c unzip.h

crypt.obj:      crypt.c unzip.h zip.h	# may or may not be in distribution

envargs.obj:    envargs.c unzip.h

explode.obj:    explode.c unzip.h

extract.obj:    extract.c unzip.h

file_io.obj:    file_io.c unzip.h

inflate.obj:    inflate.c unzip.h

mapname.obj:    mapname.c unzip.h

match.obj:      match.c unzip.h

misc.obj:       misc.c unzip.h

misc_.obj:      misc.c unzip.h
	copy misc.c misc_.c
        $(CC) -c $(CFLAGS) -DZIPINFO $(INCL) misc_.c
	del misc_.c

unreduce.obj:   unreduce.c unzip.h

unshrink.obj:   unshrink.c unzip.h



# DOS/MS make:
# -----------
unzip.exe:      $(OBJS1) $(OBJS2)
	echo $(OBJS1)+ > unzip.rsp
	echo $(OBJS2); >> unzip.rsp
	$(LD) $(LDFLAGS) @unzip.rsp
	del unzip.rsp
	$(STRIP) unzip.exe

# DOS/Borland tmake:  (not tested:  may need to use tlink instead)
# -----------------
#unzip.exe:     $(OBJS1) $(OBJS2)
#	$(LD) $(LDFLAGS) @&&|
#$(OBJS1)+
#$(OBJS2)
#|
#	$(STRIP) unzip.exe

# DOS/better makes which know how to deal with 128 char limit on command line:
# ---------------------------------------------------------------------------
#unzip.exe:     $(OBJS)
#	$(LD) $(LDFLAGS) $(OBJS) $(LDFLAGS2)



# Both makes:  (not tested)
# ----------
zipinfo.exe:    $(ZI_OBJS)
	$(LD) $(LDFLAGS) $(ZI_OBJS) $(LDFLAGS2)
	$(STRIP) zipinfo.exe
