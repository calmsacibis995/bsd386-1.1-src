		README for gdb-4.5 release
    Stu Grossman & John Gilmore 	10 Apr 1992

This is GDB, the GNU source-level debugger, presently running under un*x.
A summary of new features is in the file `WHATS.NEW'.


Unpacking and Installation -- quick overview
==========================

In this release, the GDB debugger sources, the generic GNU include
files, the BFD ("binary file description") library, the readline library,
and other libraries all have directories of their own underneath
the gdb-4.5 directory.  The idea is that a variety of GNU tools can
share a common copy of these things.  Configuration scripts and
makefiles exist to cruise up and down this directory tree and
automatically build all the pieces in the right order.

When you unpack the gdb-4.5.tar.Z file, you'll get a directory called
`gdb-4.5', which contains:

  Makefile.in     config/         configure.man   libiberty/
  README          config.sub*     gdb/            mmalloc/
  bfd/            configure*      glob/           readline/
  cfg-paper.texi  configure.in    include/        texinfo/

To build GDB, you can just do:

	cd gdb-4.5
	./configure HOSTTYPE		(e.g. sun4, decstation)
	make
	cp gdb/gdb /usr/local/bin/gdb	(or wherever you want)

This will configure and build all the libraries as well as GDB.
If you get compiler warnings during this stage, see the `Reporting Bugs'
section below; there are a few known problems.

GDB can be used as a cross-debugger, running on a machine of one type
while debugging a program running on a machine of another type.  See below.


More Documentation
==================

   The GDB 4 release includes an already-formatted reference card,
ready for printing on a PostScript or GhostScript printer, in the `gdb'
subdirectory of the main source directory--in `gdb-4.5/gdb/refcard.ps'
of the version 4.5 release.  If you have a PostScript or GhostScript
printer, you can print the reference card by just sending `refcard.ps'
to the printer.

   The release also includes the source for the reference card.  You
can format it, using TeX, by typing:

     make refcard.dvi

   The GDB reference card is designed to print in landscape mode on US
"letter" size paper; that is, on a sheet 11 inches wide by 8.5 inches
high.  You will need to specify this form of printing as an option to
your DVI output program.

   All the documentation for GDB comes as part of the machine-readable
distribution.  The documentation is written in Texinfo format, which is
a documentation system that uses a single source file to produce both
on-line information and a printed manual.  You can use one of the Info
formatting commands to create the on-line version of the documentation
and TeX (or `texi2roff') to typeset the printed version.

   GDB includes an already formatted copy of the on-line Info version
of this manual in the `gdb' subdirectory.  The main Info file is
`gdb-VERSION-NUMBER/gdb/gdb.info', and it refers to subordinate files
matching `gdb.info*' in the same directory.

   If you want to format these Info files yourself, you need one of the
Info formatting programs, such as `texinfo-format-buffer' or
`makeinfo'.

   If you have `makeinfo' installed, and are in the top level GDB
source directory (`gdb-4.5', in the case of version 4.5), you can make
the Info file by typing:

     cd gdb
     make gdb.info

   If you want to typeset and print copies of this manual, you need
TeX, a printing program such as `lpr', and `texinfo.tex', the Texinfo
definitions file.

   TeX is typesetting program; it does not print files directly, but
produces output files called DVI files.  To print a typeset document,
you need a program to print DVI files.  If your system has TeX
installed, chances are it has such a program.  The precise command to
use depends on your system; `lpr -d' is common; another is `dvips'. 
The DVI print command may require a file name without any extension or
a `.dvi' extension.

   TeX also requires a macro definitions file called `texinfo.tex'. 
This file tells TeX how to typeset a document written in Texinfo
format.  On its own, TeX cannot read, much less typeset a Texinfo
file.  `texinfo.tex' is distributed with GDB and is located in the
`gdb-VERSION-NUMBER/texinfo' directory.

   If you have TeX and a DVI printer program installed, you can
typeset and print this manual.  First switch to the the `gdb'
subdirectory of the main source directory (for example, to
`gdb-4.5/gdb') and then type:

     make gdb.dvi

Installing GDB
**************

   GDB comes with a `configure' script that automates the process of
preparing GDB for installation; you can then use `make' to build the
program.

   The GDB distribution includes all the source code you need for GDB
in a single directory, whose name is usually composed by appending the
version number to `gdb'.

   For example, the GDB version 4.5 distribution is in the `gdb-4.5'
directory.  That directory contains:

`gdb-4.5/configure (and supporting files)'
     script for configuring GDB and all its supporting libraries.

`gdb-4.5/gdb'
     the source specific to GDB itself

`gdb-4.5/bfd'
     source for the Binary File Descriptor library

`gdb-4.5/include'
     GNU include files

`gdb-4.5/libiberty'
     source for the `-liberty' free software library

`gdb-4.5/readline'
     source for the GNU command-line interface

`gdb-4.5/glob'
     source for the GNU filename pattern-matching subroutine

`gdb-4.5/mmalloc'
     source for the GNU memory-mapped malloc package

   The simplest way to configure and build GDB is to run `configure'
from the `gdb-VERSION-NUMBER' source directory, which in this example
is the `gdb-4.5' directory.

   First switch to the `gdb-VERSION-NUMBER' source directory if you
are not already in it; then run `configure'.  Pass the identifier for
the platform on which GDB will run as an argument.

   For example:

     cd gdb-4.5
     ./configure HOST
     make

where HOST is an identifier such as `sun4' or `decstation', that
identifies the platform where GDB will run.

   This sequence of `configure' and `make' builds the `bfd',
`readline', `mmalloc', and `libiberty' libraries, then `gdb' itself. 
The configured source files, and the binaries, are left in the
corresponding source directories.

   `configure' is a Bourne-shell (`/bin/sh') script; if your system
does not recognize this automatically when you run a different shell,
you may need to run `sh' on it explicitly:

     sh configure HOST

   If you run `configure' from a directory that contains source
directories for multiple libraries or programs, such as the `gdb-4.5'
source directory for version 4.5, `configure' creates configuration
files for every directory level underneath (unless you tell it not to,
with the `--norecursion' option).

   You can run the `configure' script from any of the subordinate
directories in the GDB distribution, if you only want to configure
that subdirectory; but be sure to specify a path to it.

   For example, with version 4.5, type the following to configure only
the `bfd' subdirectory:

     cd gdb-4.5/bfd
     ../configure HOST

   You can install `gdb' anywhere; it has no hardwired paths. 
However, you should make sure that the shell on your path (named by
the `SHELL' environment variable) is publicly readable.  Remember that
GDB uses the shell to start your program--some systems refuse to let
GDB debug child processes whose programs are not readable.


Compiling GDB in Another Directory
==================================

   If you want to run GDB versions for several host or target machines,
you'll need a different `gdb' compiled for each combination of host
and target.  `configure' is designed to make this easy by allowing you
to generate each configuration in a separate subdirectory, rather than
in the source directory.  If your `make' program handles the `VPATH'
feature (GNU `make' does), running `make' in each of these directories
then builds the `gdb' program specified there.

   To build `gdb' in a separate directory, run `configure' with the
`--srcdir' option to specify where to find the source.  (You'll also
need to specify a path to find `configure' itself from your working
directory.  If the path to `configure' would be the same as the
argument to `--srcdir', you can leave out the `--srcdir' option; it
will be assumed.)

   For example, with version 4.5, you can build GDB in a separate
directory for a Sun 4 like this:

     cd gdb-4.5
     mkdir ../gdb-sun4
     cd ../gdb-sun4
     ../gdb-4.5/configure sun4
     make

   When `configure' builds a configuration using a remote source
directory, it creates a tree for the binaries with the same structure
(and using the same names) as the tree under the source directory.  In
the example, you'd find the Sun 4 library `libiberty.a' in the
directory `gdb-sun4/libiberty', and GDB itself in `gdb-sun4/gdb'.

   One popular use for building several GDB configurations in separate
directories is to configure GDB for cross-compiling (where GDB runs on
one machine--the host--while debugging programs that run on another
machine--the target).  You specify a cross-debugging target by giving
the `--target=TARGET' option to `configure'.

   When you run `make' to build a program or library, you must run it
in a configured directory--whatever directory you were in when you
called `configure' (or one of its subdirectories).

   The `Makefile' generated by `configure' for each source directory
also runs recursively.  If you type `make' in a source directory such
as `gdb-4.5' (or in a separate configured directory configured with
`--srcdir=PATH/gdb-4.5'), you will build all the required libraries,
then build GDB.

   When you have multiple hosts or targets configured in separate
directories, you can run `make' on them in parallel (for example, if
they are NFS-mounted on each of the hosts); they will not interfere
with each other.


Specifying Names for Hosts and Targets
======================================

   The specifications used for hosts and targets in the `configure'
script are based on a three-part naming scheme, but some short
predefined aliases are also supported.  The full naming scheme encodes
three pieces of information in the following pattern:

     ARCHITECTURE-VENDOR-OS

   For example, you can use the alias `sun4' as a HOST argument or in
a `--target=TARGET' option, but the equivalent full name is
`sparc-sun-sunos4'.

   The following table shows all the architectures, hosts, and OS
prefixes that `configure' recognizes in GDB version 4.5.  Entries in
the "OS prefix" column ending in a `*' may be followed by a release
number.


     ARCHITECTURE  VENDOR                     OS prefix
     ------------+--------------------------+---------------------------
                 |                          |
      580        | altos        hp          | aix*          msdos*
      a29k       | amd          ibm         | amigados      newsos*
      alliant    | amdahl       intel       | aout          nindy*
      arm        | aout         isi         | bout          osf*
      c1         | apollo       little      | bsd*          sco*
      c2         | att          mips        | coff          sunos*
      cray2      | bcs          motorola    | ctix*         svr4
      h8300      | bout         ncr         | dgux*         sym*
      i386       | bull         next        | dynix*        sysv*
      i860       | cbm          nyu         | ebmon         ultrix*
      i960       | coff         sco         | esix*         unicos*
      m68000     | convergent   sequent     | hds           unos*
      m68k       | convex       sgi         | hpux*         uts
      m88k       | cray         sony        | irix*         v88r*
      mips       | dec          sun         | isc*          vms*
      ns32k      | encore       unicom      | kern          vxworks*
      pyramid    | gould        utek        | mach*
      romp       | hitachi      wrs         |
      rs6000     |                          |
      sparc      |                          |
      tahoe      |                          |
      tron       |                          |
      vax        |                          |
      xmp        |                          |
      ymp        |                          |

     *Warning:* `configure' can represent a very large number of
     combinations of architecture, vendor, and OS.  There is by no
     means support available for all possible combinations!

   The `configure' script accompanying GDB does not provide any query
facility to list all supported host and target names or aliases. 
`configure' calls the Bourne shell script `config.sub' to map
abbreviations to full names; you can read the script, if you wish, or
you can use it to test your guesses on abbreviations--for example:

     % sh config.sub sun4
     sparc-sun-sunos4
     % sh config.sub sun3
     m68k-sun-sunos4
     % sh config.sub decstation
     mips-dec-ultrix
     % sh config.sub hp300bsd
     m68k-hp-bsd
     % sh config.sub i386v
     i386-none-sysv
     % sh config.sub i786v
     *** Configuration "i786v" not recognized

`config.sub' is also distributed in the GDB source directory
(`gdb-4.5', for version 4.5).


`configure' Options
===================

   Here is a summary of all the `configure' options and arguments that
you might use for building GDB:

     configure [--srcdir=PATH]
               [--norecursion] [--rm]
               [--target=TARGET] HOST

You may introduce options with a single `-' rather than `--' if you
prefer; but you may abbreviate option names if you use `--'.

`--srcdir=PATH'
     *Warning: using this option requires GNU `make', or another
     `make' that implements the `VPATH' feature.*
      Use this option to make configurations in directories separate
     from the GDB source directories.  Among other things, you can use
     this to build (or maintain) several configurations
     simultaneously, in separate directories.  `configure' writes
     configuration specific files in the current directory, but
     arranges for them to use the source in the directory PATH. 
     `configure' will create directories under the working directory
     in parallel to the source directories below PATH.

`--norecursion'
     Configure only the directory level where `configure' is executed;
     do not propagate configuration to subdirectories.

`--rm'
     Remove the configuration that the other arguments specify.

`--target=TARGET'
     Configure GDB for cross-debugging programs running on the
     specified TARGET.  Without this option, GDB is configured to debug
     programs that run on the same machine (HOST) as GDB itself.

     There is no convenient way to generate a list of all available
     targets.

`HOST ...'
     Configure GDB to run on the specified HOST.

     There is no convenient way to generate a list of all available
     hosts.

`configure' accepts other options, for compatibility with configuring
other GNU tools recursively; but these are the only options that
affect GDB or its supporting libraries.


		Languages other than C

GDB provides some support for debugging C++ progams.  Partial Modula-2
support is now in GDB.  GDB should work with FORTRAN programs.  (If you
have problems, please send a bug report; you may have to refer to some
FORTRAN variables with a trailing underscore).  I am not aware of
anyone who is working on getting gdb to use the syntax of any other
language.  Pascal programs which use sets, subranges, file variables,
or nested functions will not currently work.


		Kernel debugging

I have't done this myself so I can't really offer any advice.
Remote debugging over serial lines works fine, but the kernel debugging
code in here has not been tested in years.  Van Jacobson has
better kernel debugging, but the UC lawyers won't let FSF have it.


		Remote debugging

The files m68k-stub.c and i386-stub.c contain two examples of remote
stubs to be used with remote.c.  They are designeded to run standalone
on a 68k or 386 cpu and communicate properly with the remote.c stub
over a serial line.

The file rem-multi.shar contains a general stub that can probably
run on various different flavors of unix to allow debugging over a
serial line from one machine to another.

Some working remote interfaces for talking to existing ROM monitors
are:
	remote-eb.c	 AMD 29000 "EBMON"
	remote-nindy.c   Intel 960 "Nindy"
	remote-adapt.c	 AMD 29000 "Adapt"
	remote-mm.c	 AMD 29000 "minimon"

Remote-vx.c and the vx-share subdirectory contain a remote interface for the
VxWorks realtime kernel, which communicates over TCP using the Sun
RPC library.  This would be a useful starting point for other remote-
via-ethernet back ends.


		Reporting Bugs

The correct address for reporting bugs found in gdb is
"bug-gdb@prep.ai.mit.edu".  Please email all bugs to that address.
Please include the GDB version number (e.g. gdb-4.5), and how
you configured it (e.g. "sun4" or "mach386 host, i586-intel-synopsys
target").

A known bug:

  * If you run with a watchpoint enabled, breakpoints will become
    erratic and might not stop the program.  Disabling or deleting the
    watchpoint will fix the problem.

GDB can produce warnings about symbols that it does not understand.  By
default, these warnings are disabled.  You can enable them by executing
`set complaint 10' (which you can put in your ~/.gdbinit if you like).
I recommend doing this if you are working on a compiler, assembler,
linker, or gdb, since it will point out problems that you may be able
to fix.  Warnings produced during symbol reading indicate some mismatch
between the object file and GDB's symbol reading code.  In many cases,
it's a mismatch between the specs for the object file format, and what
the compiler actually outputs or the debugger actually understands.

If you port gdb to a new machine, please send the required changes to
bug-gdb@prep.ai.mit.edu.  There's lots of information about doing your
own port in the file gdb-4.5/gdb/doc/gdbint.texinfo, which you can
print out, or read with `info' (see the Makefile.in there).  If your
changes are more than a few lines, obtain and send in a copyright
assignment from gnu@prep.ai.mit.edu, as described in the section
`Writing Code for GDB'.


		X Windows versus GDB

xgdb is obsolete.  We are not doing any development or support of it.

There is an "xxgdb", which shows more promise, which was posted to
comp.sources.x.

For those intersted in auto display of source and the availability of
an editor while debugging I suggest trying gdb-mode in gnu-emacs
(Try typing M-x gdb RETURN).  Comments on this mode are welcome.


		Writing Code for GDB

We appreciate having users contribute code that is of general use, but
for it to be included in future GDB releases it must be cleanly
written.  We do not want to include changes that will needlessly make
future maintainance difficult.  It is not much harder to do things
right, and in the long term it is worth it to the GNU project, and
probably to you individually as well.

If you make substantial changes, you'll have to file a copyright
assignment with the Free Software Foundation before we can produce a
release that includes your changes.  Send mail requesting the copyright
assignment to gnu@prep.ai.mit.edu.  Do this early, like before the
changes actually work, or even before you start them, because a manager
or lawyer on your end will probably make this a slow process.

Please code according to the GNU coding standards.  If you do not have
a copy, you can request one by sending mail to gnu@prep.ai.mit.edu.

Please try to avoid making machine-specific changes to
machine-independent files.  If this is unavoidable, put a hook in the
machine-independent file which calls a (possibly) machine-dependent
macro (for example, the IGNORE_SYMBOL macro can be used for any
symbols which need to be ignored on a specific machine.  Calling
IGNORE_SYMBOL in dbxread.c is a lot cleaner than a maze of #if
defined's).  The machine-independent code should do whatever "most"
machines want if the macro is not defined in param.h.  Using #if
defined can sometimes be OK (e.g. SET_STACK_LIMIT_HUGE) but should be
conditionalized on a specific feature of an operating system (set in
tm.h or xm.h) rather than something like #if defined(vax) or #if
defined(SYSV).  If you use an #ifdef on some symbol that is defined
in a header file (e.g. #ifdef TIOCSETP), *please* make sure that you
have #include'd the relevant header file in that module!

It is better to replace entire routines which may be system-specific,
rather than put in a whole bunch of hooks which are probably not going
to be helpful for any purpose other than your changes.  For example,
if you want to modify dbxread.c to deal with DBX debugging symbols
which are in COFF files rather than BSD a.out files, do something
along the lines of a macro GET_NEXT_SYMBOL, which could have
different definitions for COFF and a.out, rather than trying to put
the necessary changes throughout all the code in dbxread.c that
currently assumes BSD format.

When generalizing GDB along a particular interface, please use an
attribute-struct rather than inserting tests or switch statements
everywhere.  For example, GDB has been generalized to handle multiple
kinds of remote interfaces -- not by #ifdef's everywhere, but by
defining the "target_ops" structure and having a current target (as
well as a stack of targets below it, for memory references).  Whenever
something needs to be done that depends on which remote interface we
are using, a flag in the current target_ops structure is tested (e.g.
`target_has_stack'), or a function is called through a pointer in the
current target_ops structure.  In this way, when a new remote interface
is added, only one module needs to be touched -- the one that actually
implements the new remote interface.  Other examples of
attribute-structs are BFD access to multiple kinds of object file
formats, or GDB's access to multiple source languages.

Please avoid duplicating code.  For example, in GDB 3.x all the stuff
in infptrace.c was duplicated in *-dep.c, and so changing something
was very painful.  In GDB 4.x, these have all been consolidated
into infptrace.c.  infptrace.c can deal with variations between
systems the same way any system-independent file would (hooks, #if
defined, etc.), and machines which are radically different don't need
to use infptrace.c at all.  The same was true of core_file_command
and exec_file_command.


		Debugging gdb with itself

If gdb is limping on your machine, this is the preferred way to get it
fully functional.  Be warned that in some ancient Unix systems, like
Ultrix 4.0, a program can't be running in one process while it is being
debugged in another.  Rather than doing "./gdb ./gdb", which works on
Suns and such, you can copy gdb to gdb2 and then do "./gdb ./gdb2".

When you run gdb in the gdb source directory, it will read a ".gdbinit"
file that sets up some simple things to make debugging gdb easier.  The
"info" command, when executed without a subcommand in a gdb being
debugged by gdb, will pop you back up to the top level gdb.  See
.gdbinit for details.

I strongly recommend printing out the reference card and using it.
Send reference-card suggestions to bug-gdb@prep.ai.mit.edu, just like bugs.

If you use emacs, you will probably want to do a "make TAGS" after you
configure your distribution; this will put the machine dependent
routines for your local machine where they will be accessed first by a
M-period.

Also, make sure that you've either compiled gdb with your local cc, or
have run `fixincludes' if you are compiling with gcc.

(this is for editing this file with GNU emacs)
Local Variables:
mode: text
End:
