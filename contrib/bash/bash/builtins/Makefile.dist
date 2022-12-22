# This Makefile for building libbuiltins.a is in -*- text -*- for Emacs.
#
MKBUILTINS = mkbuiltins
RANLIB = /usr/bin/ranlib
CFLAGS = -g -I.. -I.
SHELL = /bin/sh
# CC = cc
AR = ar
RM = rm -f
CP = cp

.SUFFIXES:
.SUFFIXES: .def .c .o
# How to make a .o file from a .def file.
.def.o:
	$(RM) $@
	./$(MKBUILTINS) $(DIRECTDEFINE) $*.def
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $*.c
	$(RM) $*.c

# How to make a .c file from a .def file.
.def.c:
	$(RM) $@
	./$(MKBUILTINS) $(DIRECTDEFINE) $*.def

# Here is a rule for making .o files from .c files that does not
# force the type of the machine (like -M_MACHINE) into the flags.
.c.o:
	$(RM) $@
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $*.c

DEFS =  alias.def bind.def break.def builtin.def cd.def colon.def \
	command.def declare.def echo.def enable.def eval.def \
	exec.def exit.def fc.def fg_bg.def hash.def help.def history.def \
	jobs.def kill.def let.def read.def return.def set.def setattr.def \
	shift.def source.def suspend.def test.def times.def trap.def \
	type.def ulimit.def umask.def wait.def getopts.def reserved.def

STATIC_SOURCE = common.c getopt.c bashgetopt.c getopt.h 

OFILES = builtins.o \
	alias.o bind.o break.o builtin.o cd.o colon.o command.o \
	common.o declare.o echo.o enable.o eval.o exec.o exit.o \
	fc.o fg_bg.o hash.o help.o history.o jobs.o kill.o \
	let.o read.o return.o set.o setattr.o shift.o source.o \
	suspend.o test.o times.o trap.o type.o ulimit.o umask.o \
	wait.o getopts.o getopt.o bashgetopt.o

THINGS_TO_TAR = $(DEFS) $(STATIC_SOURCE) Makefile ChangeLog

CREATED_FILES = builtext.h builtins.c psize.aux pipesize.h

all: $(MKBUILTINS) libbuiltins.a

libbuiltins.a: $(MKBUILTINS) $(OFILES)
	$(RM) $@
	$(AR) cq $@ $(OFILES)
	-if [ -f "$(RANLIB)" ]; then $(RANLIB) $@; fi

builtext.h builtins.c: $(MKBUILTINS) $(DEFS)
	$(RM) builtext.h builtins.c
	./$(MKBUILTINS) -externfile builtext.h -structfile builtins.c \
	-noproduction $(DIRECTDEFINE) $(DEFS)

mkbuiltins: mkbuiltins.c ../config.h
	$(CC) $(CFLAGS) -o $(MKBUILTINS) mkbuiltins.c

ulimit.o: ulimit.def pipesize.h

pipesize.h:	psize.aux
	$(SHELL) ./psize.sh > pipesize.h

psize.aux:	psize.c
	$(CC) $(CFLAGS) -o $@ psize.c

documentation: builtins.texi

$(OFILES):	$(MKBUILTINS) ../config.h

builtins.texi: $(MKBUILTINS)
	./$(MKBUILTINS) -documentonly $(DEFS)

clean:
	$(RM) $(OFILES) $(CREATED_FILES) $(MKBUILTINS) libbuiltins.a

alias.o: alias.def
bind.o: bind.def
break.o: break.def
builtin.o: builtin.def
cd.o: cd.def
colon.o: colon.def
command.o: command.def
declare.o: declare.def
echo.o: echo.def
enable.o: enable.def
eval.o: eval.def
exec.o: exec.def
exit.o: exit.def
fc.o: fc.def
fg_bg.o: fg_bg.def
hash.o: hash.def
help.o: help.def
history.o: history.def
jobs.o: jobs.def
kill.o: kill.def
let.o: let.def
read.o: read.def
return.o: return.def
set.o: set.def
setattr.o: setattr.def
shift.o: shift.def
source.o: source.def
suspend.o: suspend.def
test.o: test.def
times.o: times.def
trap.o: trap.def
type.o: type.def
umask.o: umask.def
wait.o: wait.def
getopts.o: getopts.def
reserved.o: reserved.def

common.o: ../shell.h ../command.h ../config.h ../general.h ../error.h
common.o: ../variables.h ../input.h hashcom.h
common.o: ../quit.h  ../unwind_prot.h ../maxpath.h ../jobs.h ../builtins.h
alias.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
alias.o: ../quit.h
alias.o: ../shell.h ../unwind_prot.h ../variables.h common.h
bind.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
bind.o: ../shell.h ../unwind_prot.h ../variables.h bashgetopt.h
break.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
break.o: ../quit.h 
break.o: ../shell.h ../unwind_prot.h ../variables.h 
builtin.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
builtin.o: ../quit.h common.h
builtin.o: ../shell.h ../unwind_prot.h ../variables.h 
cd.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
cd.o: ../shell.h ../unwind_prot.h ../variables.h common.h
command.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
command.o: ../quit.h bashgetopt.h
command.o: ../shell.h ../unwind_prot.h ../variables.h 
declare.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
declare.o: ../quit.h 
declare.o: ../shell.h ../unwind_prot.h ../variables.h 
echo.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
echo.o: ../quit.h 
echo.o: ../shell.h ../unwind_prot.h ../variables.h 
enable.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
enable.o: ../quit.h 
enable.o: ../shell.h ../unwind_prot.h ../variables.h 
eval.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
eval.o: ../shell.h ../unwind_prot.h ../variables.h 
exec.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
exec.o: ../shell.h ../unwind_prot.h ../variables.h common.h
exit.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
exit.o: ../quit.h
exit.o: ../shell.h ../unwind_prot.h ../variables.h 
fc.o: ../builtins.h ../command.h bashgetopt.h
fc.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
fc.o: ../flags.h ../unwind_prot.h ../variables.h ../shell.h 
fg_bg.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
fg_bg.o: ../quit.h 
fg_bg.o: ../shell.h ../unwind_prot.h ../variables.h 
getopts.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
getopts.o: ../shell.h ../unwind_prot.h ../variables.h 
hash.o: ../builtins.h ../command.h 
hash.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
hash.o: ../shell.h ../unwind_prot.h ../variables.h common.h
help.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
help.o: ../quit.h 
help.o: ../shell.h ../unwind_prot.h ../variables.h 
history.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
history.o: ../quit.h
history.o: ../filecntl.h ../shell.h ../unwind_prot.h ../variables.h 
inlib.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h ../quit.h 
inlib.o: ../shell.h ../unwind_prot.h ../variables.h 
jobs.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
jobs.o: ../quit.h bashgetopt.h
jobs.o: ../shell.h ../unwind_prot.h ../variables.h 
kill.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
kill.o: ../quit.h
kill.o: ../shell.h ../trap.h ../unwind_prot.h ../variables.h 
let.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
let.o: ../quit.h 
let.o: ../shell.h ../unwind_prot.h ../variables.h 
read.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
read.o: ../quit.h 
read.o: ../shell.h ../unwind_prot.h ../variables.h 
return.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
return.o: ../quit.h 
return.o: ../shell.h ../unwind_prot.h ../variables.h 
set.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
set.o: ../quit.h 
set.o: ../shell.h ../unwind_prot.h ../variables.h 
setattr.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
setattr.o: ../quit.h common.h bashgetopt.h
setattr.o: ../shell.h ../unwind_prot.h ../variables.h 
shift.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
shift.o: ../quit.h 
shift.o: ../shell.h ../unwind_prot.h ../variables.h 
source.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
source.o: ../quit.h 
source.o: ../shell.h ../unwind_prot.h ../variables.h 
suspend.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
suspend.o: ../quit.h
suspend.o: ../shell.h ../unwind_prot.h ../variables.h 
test.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
test.o: ../quit.h 
test.o: ../shell.h ../unwind_prot.h ../variables.h 
times.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
times.o: ../quit.h
times.o: ../shell.h ../unwind_prot.h ../variables.h 
trap.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
trap.o: ../quit.h common.h
trap.o: ../shell.h ../unwind_prot.h ../variables.h 
type.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
type.o: ../quit.h common.h
type.o: ../shell.h ../unwind_prot.h ../variables.h 
ulimit.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
ulimit.o: ../quit.h 
ulimit.o: ../shell.h ../unwind_prot.h ../variables.h 
umask.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
umask.o: ../quit.h 
umask.o: ../shell.h ../unwind_prot.h ../variables.h 
wait.o: ../command.h ../config.h ../error.h ../general.h ../maxpath.h
wait.o: ../quit.h 
wait.o: ../shell.h ../unwind_prot.h ../variables.h 
