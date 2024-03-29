This directory contains complete sources for RCS version 5.6.0.1.

RCS, the Revision Control System, manages multiple revisions of files.
RCS can store, retrieve, log, identify, and merge revisions.
It is useful for files that are revised frequently,
e.g. programs, documentation, graphics, and papers.

/* Copyright (C) 1982, 1988, 1989 Walter Tichy
   Copyright 1990, 1991 by Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

This file is part of RCS.

RCS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

RCS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RCS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

Report problems and direct all questions to:

    rcs-bugs@cs.purdue.edu

*/

$Id: README.gnu,v 1.1.1.1 1993/12/21 04:23:00 polk Exp $


Installation notes:

  RCS requires a diff that supports the -n option.
  Get GNU diff (version 1.15 or later) if your diff lacks -n.

  RCS works best with a diff that supports -a and -L,
  and a diff3 that supports -E and -m.
  GNU diff supports these options.

  Sources for RCS are in the src directory.
  Read the directions in src/README to build RCS on your system.

  Manual entries reside in man.

  Troff source for the paper `RCS--A System for Version Control', which
  appeared in _Software--Practice & Experience_, is in rcs.ms.

  If you don't have troff, you can get GNU groff to format the documentation.


RCS compatibility notes:

  RCS version 5 reads RCS files written by any RCS version released since 1982.
  It also writes RCS files that these older versions of RCS can read,
  unless you use one of the following new features:

	checkin times after 1999/12/31 23:59:59 GMT
	checking in non-text files
	non-Ascii symbolic names
	rcs -bX, where X is nonempty
	rcs -kX, where X is not `kv'
	RCS files that exceed hardcoded limits in older RCS versions

  A working file written by RCS 5.5 or later contains four-digit years in its
  keyword strings.  If you check out a working file with RCS 5.5 or later,
  an older RCS version's `ci -k' may insist on two-digit years.
  Fix this with `co -V4', or by editing the working file.


Features new to RCS version 5.6.0.1 include:

  A new DIFF3_A configuration flag for GNU diff3 2.1 and later;
  see src/README.

Features new to RCS version 5.6 include:

  Security holes have been plugged; setgid use is no longer supported.

  co can retrieve old revisions much more efficiently.
  To generate the Nth youngest revision on the trunk,
  the old method used up to N passes through copies of the working file;
  the new method uses a piece table to generate the working file in one pass.

  When ci finds no changes in the working file,
  it automatically reverts to the previous revision unless -f is given.

  RCS follows symbolic links to RCS files instead of breaking them,
  and warns when it breaks hard links to RCS files.

  `$' stands for the revision number taken from working file keyword strings.
  E.g. if F contains an Id keyword string,
  `rcsdiff -r$ F' compares F to its checked-in revision, and
  `rcs -nL:$ F' gives the symbolic name L to F's revision.

  co and ci's new -M option sets the modification time
  of the working file to be that of the revision.
  Without -M, ci now tries to avoid changing the working file's
  modification time if its contents are unchanged.

  rcs's new -m option changes the log message of an old revision.

  RCS is portable to hosts that do not permit `,' in filenames.
  (`,' is not part of the Posix portable filename character set.)
  A new -x option specifies extensions other than `,v' for RCS files.
  The Unix default is `-x,v/', so that the working file `w' corresponds
  to the first file in the list `RCS/w,v', `w,v', `RCS/w' that works.
  The non-Unix default is `-x', so that only `RCS/w' is tried.
  Eventually, the Unix default should change to `-x/,v'
  to encourage interoperability among all Posix hosts.

  A new RCSINIT environment variable specifies defaults for options like -x.

  The separator for revision ranges has been changed from `-' to `:', because
  the range `A-B' is ambiguous if `A', `B' and `A-B' are all symbolic names.
  E.g. the old `rlog -r1.5-1.7' is now `rlog -r1.5:1.7'; ditto for `rcs -o'.
  For a while RCS will still support (but warn about) the old `-' separator.

  RCS manipulates its lock files using a method that is more reliable under NFS.

  Experimental support for MS-DOS and OS/2 is available as part of a separate
  software distribution.


Features new to RCS version 5 include:

  RCS can check in arbitrary files, not just text files, if diff -a works.
  RCS can merge lines containing just a single `.' if diff3 -m works.
  GNU diff supports the -a and -m options.

  RCS can now be used as a setuid program.
  See ci(1) for how users can employ setuid copies of ci, co, and rcsclean.
  Setuid privileges yield extra security if the effective user owns RCS files
  and directories, and if only the effective user can write RCS directories.
  RCS uses the real user for all accesses other than writing RCS directories.
  As described in ci(1), there are three levels of setuid support.

    1.  Setuid works fully if the seteuid() system call lets any
    process switch back and forth between real and effective users,
    as specified in Posix 1003.1a Draft 5.

    2.  On hosts with saved setuids (a Posix 1003.1-1990 option) and without
    a modern seteuid(), setuid works unless the real or effective user is root.

    3.  On hosts that lack both modern seteuid() and saved setuids,
    setuid does not work, and RCS uses the effective user for all accesses;
    formerly it was inconsistent.

  New options to co, rcsdiff, and rcsmerge give more flexibility to keyword
  substitution.

    -kkv substitutes the default `$Keyword: value $' for keyword strings.
    However, a locker's name is inserted only as a file is being locked,
    i.e. by `ci -l' and `co -l'.  This is normally the default.

    -kkvl acts like -kkv, except that a locker's name is always inserted
    if the given revision is currently locked.  This was the default in
    version 4.  It is now the default only with when using rcsdiff to
    compare a revision to a working file whose mode is that of a file
    checked out for changes.

    -kk substitutes just `$Keyword$', which helps to ignore keyword values
    when comparing revisions.

    -ko retrieves the old revision's keyword string, thus bypassing keyword
    substitution.

    -kv retrieves just `value'.  This can ease the use of keyword values, but
    it is dangerous because it causes RCS to lose track of where the keywords
    are, so for safety the owner write permission of the working file is
    turned off when -kv is used; to edit the file later, check it out again
    without -kv.

  rcs -ko sets the default keyword substitution to be in the style of co -ko,
  and similarly for the other -k options.  This can be useful with binary file
  formats that cannot tolerate changing the lengths of keyword strings.
  However it also renders a RCS file readable only by RCS version 5 or later.
  Use rcs -kkv to restore the usual default substitution.

  RCS can now be used by development groups that span timezone boundaries.
  All times are now displayed in GMT, and GMT is the default timezone.
  To use local time with co -d, append ` LT' to the time.
  When interchanging RCS files with sites running older versions of RCS,
  time stamp discrepancies may prevent checkins; to work around this,
  use `ci -d' with a time slightly in the future.

  Dates are now displayed using four-digit years, not two-digit years.
  Years given in -d options must now have four digits.
  This change is required for RCS to continue to work after 1999/12/31.
  The form of dates in version 5 RCS files will not change until 2000/01/01,
  so in the meantime RCS files can still be interchanged with sites
  running older versions of RCS.  To make room for the longer dates,
  rlog now outputs `lines: +A -D' instead of `lines added/del: A/D'.

  To help prevent diff programs that are broken or have run out of memory
  from trashing an RCS file, ci now checks diff output more carefully.

  ci -k now handles the Log keyword, so that checking in a file
  with -k does not normally alter the file's contents.

  RCS no longer outputs white space at the ends of lines
  unless the original working file had it.
  For consistency with other keywords,
  a space, not a tab, is now output after `$Log:'.
  Rlog now puts lockers and symbolic names on separate lines in the output
  to avoid generating lines that are too long.
  A similar fix has been made to lists in the RCS files themselves.

  RCS no longer outputs the string `Locker: ' when expanding Header or Id
  keywords.  This saves space and reverts back to version 3 behavior.

  The default branch is not put into the RCS file unless it is nonempty.
  Therefore, files generated by RCS version 5 can be read by RCS version 3
  unless they use the default branch feature introduced in version 4.
  This fixes a compatibility problem introduced by version 4.

  RCS can now emulate older versions of RCS; see `co -V'.
  This may be useful to overcome compatibility problems
  due to the above changes.

  Programs like Emacs can now interact with RCS commands via a pipe:
  the new -I option causes ci, co, and rcs to run interactively,
  even if standard input is not a terminal.
  These commands now accept multiple inputs from stdin separated by `.' lines.

  ci now silently ignores the -t option if the RCS file already exists.
  This simplifies some shell scripts and improves security in setuid sites.

  Descriptive text may be given directly in an argument of the form -t-string.

  The character set for symbolic names has been upgraded
  from Ascii to ISO 8859.

  rcsdiff now passes through all options used by GNU diff;
  this is a longer list than 4.3BSD diff.

  merge's new -L option gives tags for merge's overlap report lines.
  This ability used to be present in a different, undocumented form;
  the new form is chosen for compatibility with GNU diff3's -L option.

  rcsmerge and merge now have a -q option, just like their siblings do.

  RCS now attempts to ignore parts of an RCS file that look like they come
  from a future version of RCS.

  When properly configured, RCS now strictly conforms with Posix 1003.1-1990.
  RCS can still be compiled in non-Posix traditional Unix environments,
  and can use common BSD and USG extensions to Posix.
  RCS is a conforming Standard C program, and also compiles under traditional C.

  Arbitrary limits on internal table sizes have been removed.
  The only limit now is the amount of memory available via malloc().

  File temporaries, lock files, signals, and system call return codes
  are now handled more cleanly, portably, and quickly.
  Some race conditions have been removed.

  A new compile-time option RCSPREFIX lets administrators avoid absolute path
  names for subsidiary programs, trading speed for flexibility.

  The configuration procedure is now more automatic.

  Snooping has been removed.


Version 4 was the first version distributed by FSF.
Beside bug fixes, features new to RCS version 4 include:

  The notion of default branch has been added; see rcs -b.


Version 3 was included in the 4.3BSD distribution.


Further projects:

  Add format options for finer control over the output of ident and rlog.

  Be able to redo the most recent checkin with minor changes.

  Add a `-' option to take the list of pathnames from standard input.
  Perhaps the pathnames should be null-terminated, not newline-terminated,
  so that pathnames that contain newlines are handled properly.

  Add general options so that rcsdiff and rcsmerge can pass arbitrary options
  to its subsidiary co and diff processes.  E.g.

	-.OPTION to pass OPTION to the subsidiary `co'
	-/OPTION to pass OPTION to the subsidiary `diff' (for rcsdiff only)

  For example:

	rcsdiff -.-d"1991/02/09 18:09" -.-sRel -/+unified -/-C -/5 -/-d foo.c

  invokes `co -d"1991/02/09 18:09" -sRel ...' and `diff +unified -C 5 -d ...'.
  To pass an option to just one subsidiary `co', put the -. option
  after the corresponding -r option.  For example:

	rcsmerge -r1.4 -.-ko -r1.8 -.-kkv foo.c

  passes `-ko' to the first subsidiary `co', and `-kkv' to the second one.


  Permit multiple option-pathname pairs, e.g. co -r1.4 a -r1.5 b.

  Add ways to specify the earliest revision, the most recent revision,
  the earliest or latest revision on a particular branch, and
  the parent or child of some other revision.

  If a user has multiple locks, perhaps ci should fall back on ci -k's
  method to figure out which revision to use.

  Symbolic names need not refer to existing branches and revisions.
  rcs(1)'s BUGS section says this is a bug.  Is it?  If so, it should be fixed.

  Write an rcsck program that repairs corrupted RCS files,
  much as fsck repairs corrupted file systems.

  Clean up the source code with a consistent indenting style.

  Update the date parser to use the more modern getdate.y by Bellovin,
  Salz, and Berets, or the even more modern getdate by Moraes.  None of
  these getdate implementations are as robust as RCS's old warhorse in
  avoiding problems like arithmetic overflow, so they'll have to be
  fixed first.

  Break up the code into a library so that it's easier to write new programs
  that manipulate RCS files, and so that useless code is removed from the
  existing programs.  For example, the rcs command contains unnecessary
  keyword substitution baggage, and the merge command can be greatly pruned.

  Make it easier to use your favorite text editor to edit log messages,
  etc. instead of having to type them in irretrievably at the terminal.

The following projects require a change to RCS file format,
and thus must wait until at least RCS version 6.

  Be able to store RCS files in compressed format.
  Don't bother to use a .Z extension that would exceed file name length limits;
  just look at the magic number.

  Add locker commentary, e.g. `co -l -m"checkout to fix merge bug" foo'
  to tell others why you checked out `foo'.
  Also record the time when the revision was locked,
  and perhaps the working pathname (if applicable).

  Let the user mark an RCS revision as deleted; checking out such a revision
  would result in no working file.  Similarly, using `co -d' with a date either
  before the initial revision or after the file was marked deleted should
  remove the working file.  For extra credit, extend the notion of `deleted' to
  include `renamed'.  RCS should support arbitrary combinations of renaming and
  deletion, e.g. renaming A to B and B to A, checking in new revisions to both
  files, and then renaming them back.

  Use a better scheme for locking revisions; the current scheme requires
  changing the RCS file just to lock or unlock a revision.
  The new scheme should coexist as well as possible with older versions of RCS,
  and should avoid the rare NFS bugs mentioned in rcsedit.c.

  Add rcs options for changing keyword names, e.g. XConsortium instead of Id.

  Add frozen branches a la SCCS.  In general, be able to emulate all of
  SCCS, so that an SCCS-to-RCS program can be practical.

  Add support for distributed RCS, where widely separated
  users cannot easily access each others' RCS files,
  and must periodically distribute and reconcile new revisions.

  Be able to create empty branches.

  Improve RCS's method for storing binary files.
  Although it is more efficient than SCCS's,
  the diff algorithm is still line oriented,
  and often generates long output for minor changes to an executable file.

  Add a new `-kb' expansion for binary files on non-Posix hosts
  that distinguish between text and binary I/O.
  The current `text_work_stdio' compile-time switch is too inflexible.
  This fix either requires nonstandard primitives like DOS's setmode(),
  or requires that `-kb' be specified on initial checkin and never changed.
  From the user's point of view, it would be best if
  RCS detected and handled binary files without human intervention,
  switching expansion methods as needed from revision to revision.

  Extend the grammar of RCS files so that keywords need not be in a fixed order.

  Internationalize messages; unfortunately, there's no common standard yet.
  This requires a change in RCS file format because of the
  `empty log message' and `checked in with -k' hacks inside RCS files.


Credits:

  RCS was designed and built by Walter F. Tichy of Purdue University.
  RCS version 3 was released in 1983.

  Adam Hammer, Thomas Narten, and Dan Trinkle of Purdue supported RCS through
  version 4.3, released in 1990.  Guy Harris of Sun contributed many porting
  fixes.  Paul Eggert of System Development Corporation contributed bug fixes
  and tuneups.  Jay Lepreau contributed 4.3BSD support.

  Paul Eggert of Twin Sun wrote the changes for RCS version 5, released in 1991.
  Rich Braun of Kronos and Andy Glew of Intel contributed ideas for new options.
  Bill Hahn of Stratus contributed ideas for setuid support.
  Ideas for piece tables came from Joe Berkovitz of Stratus and Walter F. Tichy.
  Matt Cross of Stratus contributed test case ideas.
  Adam Hammer of Purdue QAed.
