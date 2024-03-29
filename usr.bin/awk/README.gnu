README:

This is GNU Awk 2.15. It should be upwardly compatible with the
System V Release 4 awk.  It is almost completely compliant with draft 11.3
of POSIX 1003.2.

This release adds new features -- see NEWS for details.

See the installation instructions, below.

Known problems are given in the PROBLEMS file.  Work to be done is
described briefly in the FUTURES file.  Verified ports are listed in
the PORTS file.  Changes in this version are summarized in the CHANGES file.
Please read the LIMITATIONS and ACKNOWLEDGMENT files.

Read the file POSIX for a discussion of how the standard says comparisons
should be done vs. how they really should be done and how gawk does them.
  
To format the documentation with TeX, you must use texinfo.tex 2.53
or later.  Otherwise footnotes look unacceptable.

If you wish to remake the Info files, you should use makeinfo. The 2.15 
version of makeinfo works with no errors.

The man page is up to date.

INSTALLATION:

Check whether there is a system-specific README file for your system.

Makefile.in may need some tailoring.  The only changes necessary should
be to change installation targets or to change compiler flags.
The changes to make in Makefile.in are commented and should be obvious.

All other changes should be made in a config file.  Samples for
various systems are included in the config directory.  Starting with
2.11, our intent has been to make the code conform to standards (ANSI,
POSIX, SVID, in that order) whenever possible, and to not penalize
standard conforming systems.  We have included substitute versions of
routines not universally available.  Simply add the appropriate define
for the missing feature(s) on your system.

If you have neither bison nor yacc, use the awktab.c file here.  It was
generated with bison, and should have no AT&T code in it.  (Note that
modifying awk.y without bison or yacc will be difficult, at best.  You might
want to get a copy of bison from the FSF too.)

If no config file is included for your system,  start by copying one
for a similar system.  One way of determining the defines needed is to
try to load gawk with nothing defined and see what routines are
unresolved by the loader.  This should give you a good idea of how to
proceed.

The next release will use the FSF autoconfig program, so we are no longer 
soliciting new config files.

If you have an MS-DOS or OS/2 system, use the stuff in the pc directory.
For an Atari there is an atari directory and similarly one for VMS.

Chapter 16 of The GAWK Manual discusses configuration in detail.
(However, it does not discuss OS/2 configuration, see README.pc for
the details. The manual is being massively revised for 2.16.)

After successful compilation, do 'make test' to run a small test
suite.  There should be no output from the 'cmp' invocations except in
the cases where there are small differences in floating point values.
If there are other differences, please investigate and report the
problem.

PRINTING THE MANUAL

The 'support' directory contains texinfo.tex 2.65, which will be necessary
for printing the manual, and the texindex.c program from the texinfo
distribution which is also necessary.  See the makefile for the steps needed
to get a DVI file from the manual.

CAVEATS

The existence of a patchlevel.h file does *N*O*T* imply a commitment on
our part to issue bug fixes or patches.  It is there in case we should
decide to do so.

BUG REPORTS AND FIXES (Un*x systems):

Please coordinate changes through David Trueman and/or Arnold Robbins.

David Trueman
Department of Mathematics, Statistics and Computing Science,
Dalhousie University, Halifax, Nova Scotia, Canada

UUCP:		{uunet utai watmath}!dalcs!david
INTERNET:	david@cs.dal.ca

Arnold Robbins
1736 Reindeer Drive
Atlanta, GA, 30329, USA

INTERNET:	arnold@skeeve.atl.ga.us
UUCP:		{ gatech, emory, emoryu1 }!skeeve!arnold

BUG REPORTS AND FIXES (non-Unix ports):

MS-DOS:
	Scott Deifik
	AMGEN Inc.
	Amgen Center, Bldg.17-Dept.393
	Thousand Oaks, CA  91320-1789
	Tel-805-499-5725  ext.4677
	Fax-805-498-0358
	scottd@amgen.com

VMS:
	Pat Rankin
	rankin@eql.caltech.edu (e-mail only)

Atari ST:
	Michal Jaegermann
	NTOMCZAK@vm.ucs.UAlberta.CA  (e-mail only)

OS/2:
	Kai Uwe Rommel
	rommel@ars.muc.de (e-mail only)
