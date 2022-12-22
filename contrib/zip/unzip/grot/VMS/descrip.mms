!==========================================================================
! MMS description file for UnZip 5.0+                          26 June 1992
!==========================================================================
!
!   Original by Antonio Querubin, Jr., <querubin@uhccvx.uhcc.hawaii.edu>
!     (23 Dec 90)
!   Enhancements by Igor Mandrichenko, <mandrichenko@mx.decnet.ihep.su>
!     (9 Feb 92)

! To build UnZip that uses shared libraries,
!	mms
! (One-time users will find it easier to use the MAKE_UNZIP_VAXC.COM command
! file, which generates both UnZip and ZipInfo.  Just type "@MAKE_UNZIP_VAXC";
! or "@MAKE_UNZIP_GCC" if you have GNU C.)

! To build UnZip without shared libraries,
!	mms noshare

! To delete unnecessary .OBJ files,
!	mms clean

CRYPTF =
CRYPTO =
! To build decryption version, uncomment next two lines:
!CRYPTF = /def=(CRYPT)
!CRYPTO = crypt.obj,

CC = cc
CFLAGS = $(CRYPTF)
LD = link
LDFLAGS =
EXE =
O = .obj;
OBJS = unzip$(O), $(CRYPTO) envargs$(O), explode$(O), extract$(O),\
       file_io$(O), inflate$(O), mapname$(O), match$(O), misc$(O),\
       unreduce$(O), unshrink$(O), vms$(O)
OBJI = zipinfo$(O), envargs$(O), match$(O), misc.obj_, vms.obj_

LDFLAGS2 =

default	:	unzip.exe, zipinfo.exe
	@	!	Do nothing.

unzip.exe :	$(OBJS), vmsshare.opt
	$(LD) $(LDFLAGS) $(OBJS), \
		vmsshare.opt/options

zipinfo.exe :	$(OBJI), vmsshare.opt
	$(LD) $(LDFLAGS) $(OBJI), \
		vmsshare.opt/options


noshare :	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS), \
		sys$library:vaxcrtl.olb/library $(LDFLAGS2)

clean :
	delete $(OBJS)	! you may want to change this to 'delete *.obj;*'

crypt$(O) :	crypt.c unzip.h zip.h	! may or may not be included in distrib
envargs$(O) :	envargs.c unzip.h
explode$(O) :	explode.c unzip.h
extract$(O) :	extract.c unzip.h
file_io$(O) :	file_io.c unzip.h
inflate$(O) :	inflate.c unzip.h
mapname$(O) :	mapname.c unzip.h
match$(O) :	match.c unzip.h
misc$(O) :	misc.c unzip.h
unreduce$(O) :	unreduce.c unzip.h
unshrink$(O) :	unshrink.c unzip.h
unzip$(O) :	unzip.c unzip.h
vms$(O)	  :	vms.c unzip.h
VMSmunch$(O) :	VMSmunch.c VMSmunch.h
misc.obj_ :	misc.c unzip.h
	$(CC)/object=misc.obj_/define="ZIPINFO" misc.c

vms.obj_ :	vms.c unzip.h
	$(CC)/object=vms.obj_/define="ZIPINFO" vms.c
