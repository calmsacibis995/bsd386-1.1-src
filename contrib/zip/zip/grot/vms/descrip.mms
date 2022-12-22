# VMS Makefile for Zip, ZipNote and ZipSplit

CDEB = !	/DEBUG/NOOPT
LDEB = !	/DEBUG

OBJZ = zip.obj,zipfile.obj,zipup.obj,fileio.obj,util.obj,globals.obj,-
	crypt.obj,vms.obj,VMSmunch.obj
OBJI = deflate.obj,trees.obj,bits.obj
OBJN = zipnote.obj,zipfile.obj_,zipup.obj_,fileio.obj_,globals.obj,-
	util_.obj_,vmsmunch.obj
OBJS = zipsplit.obj,zipfile.obj_,zipup.obj_,fileio.obj_,globals.obj,-
	util_.obj_,vmsmunch.obj
OBJC = zipcloak.obj,zipfile.obj_,zipup.obj_,fileio.OBJ_,util_.obj,-
		crypt.obj_,globals.obj,vmsmunch.obj

CC	=	cc $(CDEB)

LINK	=	link $(LDEB)

.suffixes
.suffixes : .obj .obj_ .c .exe

.c.obj_ :
	$(CC)/define=("UTIL")/OBJ=$(mms$target) $(mms$source)
.c.obj :
	$(cc)/OBJ=$(mms$target) $(mms$source)
.obj.exe :
	$(LINK)/exe=$(mms$target) vaxclib.opt/opt,$(mms$source)

# rules for zip, zipnote, zipsplit, and zip.doc.

default :	zip.exe,zipnote.exe,zipsplit.exe
 @ !

zipfile.obj_ :	zipfile.c
zipup.obj_   :	zipup.c
fileio.obj_  :	fileio.c
util.obj_    :	util.c

$(OBJZ) : zip.h,ziperr.h,tailor.h
$(OBJI) : zip.h,ziperr.h,tailor.h
$(OBJN) : zip.h,ziperr.h,tailor.h
$(OBJS) : zip.h,ziperr.h,tailor.h

zip.obj,zipup.obj,zipnote.obj,zipsplit.obj : revision.h

zipfile.obj, fileio.obj, VMSmunch.obj : VMSmunch.h

zip.exe : $(OBJZ),$(OBJI)
	$(LINK)/exe=zip.exe vaxclib.opt/opt,$(OBJZ),$(OBJI)

zipnote.exe : $(OBJN)
	$(LINK)/exe=zipnote.exe vaxclib.opt/opt,$(OBJN)

zipsplit.exe : $(OBJS)
	$(LINK)/exe=zipsplit.exe vaxclib.opt/opt,$(OBJS)
