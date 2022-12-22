add -8 to FLEX_FLAGS


(Message inbox:29)
Date:  Thu, 16 Aug 90 13:40:48 CDT
From:  Terry Poot <tp@mccall.com>
Subject:  Re: flex 2.3 patch #4 
To:  vern@cs.cornell.edu

>You shouldn't need either alloca() or bcopy() - try deleting them from
>the Makefile and see if it still builds.  If it does, let me know and
>I'll fix the Makefile for the 2.4 release.  (I don't keep the Makefile
>up-to-date it since I don't have a VMS system easily available for testing.)

Well, I built it. The makefile provided depended on a non-standard configuratio
n 
of the system libraries (the C library is NOT searched by default on VMS, but 
that file assumed it was, which CAN be arranged easily, but it doesn't come tha
t 
way). I've fixed this problem. I've also modified it to be able to use YACC 
rather than BISON, though BISON is the default. In addition to this file, I'm 
enclosing a link options file (needed to fix the above problem), and a VMS 
command procedure useable by sites without MMS (which is an extra cost product 
from DEC) to build FLEX. I don't have shar, and I figured a VMS_SHARE file 
wouldn't do you any good, so you'll have to unpack these by hand.

When I built it, the test failed. It looks like initscan has not been rebuilt 
with the new version. The only difference that concerns me is that yy_ec has 
gone from [128] to [256], and been padded with "1" values. I don't know if that
 
is correct or not. Perhaps initscan should be rebuilt with the new flex so the 
test will succeed. On the other hand, perhaps the problem is caused by my use o
f 
YACC rather than BISON, or that my patches are all screwed up. Any ideas?

I didn't have the original files you sent me, so I just used the new misc.c wit
h 
the rest of the files. If the only patch I screwed up was #4, this should be OK
, 
but on the other hand there may be other problems that caused the above 
behavior. In either case, the files below should be fine, since you haven't 
changed the build procedure appreciably. If my copy is messed up, I'll just get
 
it from comp.sources.unix when it comes out (is there any way you can get these
 
files into that distribution before it is posted?).

------------ Begin new file flex/misc/opt.opt --------------
sys$share:vaxcrtl.exe/share
------------ Begin new file flex/misc/vmsbuild.com ---------
$ ! vmsbuild.com
$ ! VMS build procedure for FLEX
$ goto start
$ help:
$ type sys$input

 To use:

 Define tools$$exe, tools$$library, and tools$$manual to reflect the 
 locations where you would like to store the executables, library (flex.skel)
 and the manual pages.  These names can be defined at the command line
 prompt:  eg.
 
    $ define tools$$exe disk:[dir1.dir2.etc]
   
 Your default directory should be the directory containing the flex
 source. This file (vmsbuild.com) and the file opt.opt should be in the
 [.misc] subdirectory under the default directory. You may now execute the
 following command at the DCL command line:
 
   $ @[.misc]vmsbuild
 
 If you want to use YACC rather than BISON, enter the following command
 at the DCL prompt instead:
 
   $ @[.misc]vmsbuild yacc
 
$ stop
$ start:
$ if f$trnlnm("tools$$exe").eqs."".or. -
     f$trnlnm("tools$$library").eqs."".or. -
     f$trnlnm("tools$$exe").eqs."".or. -
     f$search("[.misc]opt.opt").eqs."" then goto help
$ old_ve = f$verify(1)
$ copy flex.skel tools$$library:flex.skel
$ ! Installed tools$$library:flex.skel
$ copy flex.1 tools$$manual:flex.doc
$ ! installed tools$$manual:flex.doc
$ cc /define=(VMS,USG) ccl.c
$ cc /define=(VMS,USG) dfa.c
$ cc /define=(VMS,USG) ecs.c
$ cc /define=(VMS,USG) gen.c
$ cc /define=(VMS,USG,"DEFAULT_SKELETON_FILE=""tools$$library:FLEX.SKEL""") 
main.c
$ cc /define=(VMS,USG) misc.c
$ cc /define=(VMS,USG) nfa.c
$ if p2.eqs."YACC"
$ then
$    yacc -d -v parse.y
$    copy y-tab.c parse.c
$    del/noconfirm y-tab.c;*
$    copy y-tab.h parse.h
$    del/noconfirm y-tab.h;*
$ else
$    bison/defines/verbose/fixed_outfiles parse.y
$    copy y_tab.c parse.c
$    del/noconfirm y_tab.c;*
$    copy y_tab.h parse.h
$    del/noconfirm y_tab.h;*
$ endif
$ cc /define=(VMS,USG) parse.c
$ copy initscan.c scan.c
$ cc /define=(VMS,USG) scan.c
$ cc /define=(VMS,USG) sym.c
$ cc /define=(VMS,USG) tblcmp.c
$ cc /define=(VMS,USG) yylex.c
$ link /exe=flex.exe -
   ccl.obj,dfa.obj,ecs.obj,gen.obj,main.obj,misc.obj,nfa.obj,parse.obj, -
   scan.obj,sym.obj,tblcmp.obj,yylex.obj,[.misc]opt.opt/opt
$ copy flex.exe tools$$exe:flex.exe
$ flex :== $ tools$$exe:flex.exe
$ ! Installed FLEX and LIBRARIES
$ x = f$verify(old_ve)
------------ Begin changed file flex/misc/makefile.vms ---------
############################ VMS MAKEFILE ##############################
#
# Define tools$$exe, tools$$library, and tools$$manual to reflect the 
# locations where you would like to store the executables, library (flex.skel)
# and the manual pages.  These names can be defined at the command line
# prompt:  eg.
#    $ define tools$$exe disk:[dir1.dir2.etc]
#   
# If you want to use YACC rather than BISON, enter the following command
# at the DCL prompt before running MMS.
#
#    $ use_yacc==1
#
# Your default directory should be the directory containing the flex
# source. This file (makefile.vms) and the file opt.opt should be in the
# [.misc] subdirectory under the default directory. You may now execute the
# following commands at the DCL command line:
#
#   $ mms/descrip=[.misc]makefile.vms install
#   $ mms/descrip=[.misc]makefile.vms test
#   $ mms/descrip=[.misc]makefile.vms clean
#
# When "mms/descrip=[.misc]makefile.vms test" is executed the diff should
# not show any differences. In fact the same effect can be achieved by
#
#   $ mms/descrip=[.misc]makefile.vms install, test, clean
#
# VMS make file for "flex" tool

# Redefine the following for your own environment
BIN = tools$$exe
LIB = tools$$library
MAN = tools$$manual

SKELETON_FILE = "DEFAULT_SKELETON_FILE=""$(LIB):FLEX.SKEL"""

CCFLAGS = VMS,USG
FLEX_FLAGS = -is

FLEXOBJS = ccl.obj dfa.obj ecs.obj gen.obj main.obj misc.obj nfa.obj  -
           parse.obj scan.obj sym.obj tblcmp.obj yylex.obj 

OBJ = ccl.obj,dfa.obj,ecs.obj,gen.obj,main.obj,misc.obj,nfa.obj,parse.obj, -
      scan.obj,sym.obj,tblcmp.obj,yylex.obj

default : flex
	! installed FLEX

install : lib man bin 
	!Installed FLEX and LIBRARIES

lib : $(LIB):flex.skel
	! Installed $(LIB):flex.skel

bin : $(BIN):flex.exe
	flex :== $ $(BIN):flex.exe

man : $(MAN):flex.doc
	! installed $(MAN):flex.doc

$(LIB):flex.skel : flex.skel
	copy flex.skel $(LIB):flex.skel
$(BIN):flex.exe : flex.exe
	copy flex.exe $(BIN):flex.exe
$(MAN):flex.doc : flex.1
	copy flex.1 $(MAN):flex.doc

flex : flex.exe
	copy flex.exe $(BIN):flex.exe

flex.exe : $(FLEXOBJS)
	link /exe=flex.exe -
		$(OBJ),[.misc]opt.opt/opt


.ifdef use_yacc
parse.c : parse.y
	yacc -d -v parse.y
	copy y-tab.c parse.c
	del/noconfirm y-tab.c;*

parse.h : parse.c
	copy y-tab.h parse.h
	del/noconfirm y-tab.h;*

.else

parse.c : parse.y
	bison/defines/verbose/fixed_outfiles parse.y
	copy y_tab.c parse.c
	del/noconfirm y_tab.c;*

parse.h : parse.c
	copy y_tab.h parse.h
	del/noconfirm y_tab.h;*

.endif

scan.c : initscan.c
	copy initscan.c scan.c

ccl.obj : ccl.c flexdef.h
	cc /define=($(CCFLAGS)) ccl.c
dfa.obj : dfa.c flexdef.h
	cc /define=($(CCFLAGS)) dfa.c
ecs.obj : ecs.c flexdef.h
	cc /define=($(CCFLAGS)) ecs.c
gen.obj : gen.c flexdef.h
	cc /define=($(CCFLAGS)) gen.c
main.obj : main.c flexdef.h
	cc /define=($(CCFLAGS),$(SKELETON_FILE)) main.c
misc.obj : misc.c flexdef.h
	cc /define=($(CCFLAGS)) misc.c
nfa.obj : nfa.c flexdef.h
	cc /define=($(CCFLAGS)) nfa.c
parse.obj : parse.c flexdef.h parse.h
	cc /define=($(CCFLAGS)) parse.c
scan.obj : scan.c parse.h flexdef.h
	cc /define=($(CCFLAGS)) scan.c
sym.obj : sym.c flexdef.h
	cc /define=($(CCFLAGS)) sym.c
tblcmp.obj : tblcmp.c flexdef.h
	cc /define=($(CCFLAGS)) tblcmp.c
yylex.obj : yylex.c parse.h flexdef.h
	cc /define=($(CCFLAGS)) yylex.c

clean :
	! Cleaning up by deleting unnecessary object files etc.
	- delete/noconfirm scan.c;*
	- delete/noconfirm parse.c;*
	- delete/noconfirm parse.h;*
	- delete/noconfirm lexyy.c;*
	- delete/noconfirm *.obj;*
	- delete/noconfirm flex*.tmp;*
	- delete/noconfirm y.output;*
	- delete/noconfirm *.diff;*
	- delete/noconfirm y_tab.*;*
	- purge/log

test :  $(BIN):flex.exe
	flex :== $ $(BIN):flex.exe
	define tools$$library 'f$environment("default")'
	sho log tools$$library
	flex $(FLEX_FLAGS) scan.l
	diff/out=flex.diff initscan.c lexyy.c
	type/page flex.diff

----------- End of file ----------------
Terry Poot <tp@mccall.com>                The McCall Pattern Company
(uucp: ...!rutgers!ksuvax1!mccall!tp)     615 McCall Road
(800)255-2762, in KS (913)776-4041        Manhattan, KS 66502, USA

