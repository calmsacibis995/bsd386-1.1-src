This is a much modified version of the public domain spread sheet sc,
posted several years ago by Mark Weiser as vc, originally by James Gosling.

CHANGES lists the changes since 6.1 to 6.21.
	Sc6.16 was released to comp.sources.misc and four sets of patches
	bring Sc6.16->6.17->6.18->6.19->6.21.

Current maintainer: nstar!sawmill!prslnk!buhrt (Jeff Buhrt)

When you get it built, try "sc tutorial.sc" for a simple introduction
to the basic commands.

To print a quick reference card, type the command:
	scqref | [your_printer_commmand]

If you have the command 'file' that uses /etc/magic add the line:
38	string		Spreadsheet	sc file

Psc formats ascii files for use in the spreadsheet.  If you don't have
getopts, there is a public domain version by Henry Spencer hidden away in
the VMS_NOTES file.

I have modified the Makefile to make it easy for you to call the
program what you want.  Just change "name=sc" and "NAME=SC" to
"name=myfavoritename" and "NAME=MYFAVORITENAME" and try "make
myfavoritename".

Similarly, you can make the documentation with "make myfavoritename.man".
"make install" will make and install the code in EXDIR.  The
installation steps and documentation all key off of the name.  The
makefile even changes the name in the nroffable man page.  If you don't
have nroff, you will have to change sc.man yourself.

This release has been tested against a Sequent S81 running DYNIX 3.0.17
(BSD 4.2):cc, atscc, gcc, AT&T SysV 3.2.2:cc, gcc, ESIX SysV 3.2 Rev D:cc, gcc.
Just check the Makefile for the system flags.   I have heard
reports of lots of other machines that work. If you have problems with
lex.c, and don't care about arrow keys, define SIMPLE (-DSIMPLE in the
makefile).  SIMPLE causes the arrow keys to not be used.

If you have problems with your yacc saying: too many terminals ...127...
Find a different yacc: bison, Berkeley yacc, Pd yacc, etc. AT&T's Sys V
yacc has small, fixed sized tables. SCO will allow dynamic yacc tables
when given the correct flags.

Guidelines for Hackers:

If you want to send changes you have made to SC, please feel free to do
so.  If they work :-) and seem worthwhile, I'll put them in.

a) Please refrain from wholesale "style" or "cleanup" changes.  It is easy
	to add your changes but it makes it hard to merge in the next guy's
	stuff if he used the release as a base.  Suggestions are welcome.

b) Leave my $Revision:  identifiers alone-they help me track what you used
	as a base.  If you check the code into rcs, delete the "$"s on the
	Revison lines before you do.
c) Please abide by the style in the code when you make your changes.  It is easy
 	to break things trying to make them look "right".
d) If you do string functions, please, PLEASE pay attention to null pointers,
	use scxmalloc, scxrealloc, and scxfree; and scxfree those arguments.
e) Please don't forget to document your changes in both help.c and sc.doc.
f) WHEN sending diffs: please use diff -c (context diff, and possibly
	more than three lines on each side), I get a lot of code and
	things move.
g) Please send an explanation of what your code does and how to use/test it.

Disclaimer:

Starting 4/4/90: (I will be maintaining Sc for a while at least,
	Robert Bond has been very busy lately)

Archives:
1) (FTP) jpd@usl.edu James Dugal
	pc.usl.edu	in the pub/unix directory

2) (UUCP) marc@dumbcat.sf.ca.us (Marco S Hyman)
    dumbcat Any ACU 9600 14157850194 "" \d\r in:--in: nuucp word: guest
    dumbcat Any ACU 2400 14157850194 "" \d\r in:-BREAK-in: nuucp word: guest
    dumbcat Any ACU 1200 14157850194 "" \d\r in:-BREAK-in:-BREAK-in: nuucp word: guest
  Note: dumbcat speaks 9600 at V.32 -- sorry, this is not a Telebit modem.
  (Grab dumbcat!~/INDEX for a complete list)

3) Sc6.16 was posted in comp.sources.misc, about Jun 4 1991, Volume 20, Issues
	035-041. There were then 4 patches: Sc6.16->Sc6.17, Sc6.17->Sc6.18,
	Sc6.18->Sc6.19, and Sc6.19->Sc6.21.

4) (FTP) Paul A Vixie <uunet!decwrl!vixie>
	gatekeeper.dec.com	in /pub/misc/sc-6.21.tar.Z

Sc is not a product of ProsLink, Inc.  It is supplied as is with no
warranty, express or implied, as a service to Usenet readers.  It is not
copyrighted, either.  Have at it.

					Jeff Buhrt
					ProsLink, Inc.
					nstar!sawmill!prslnk!buhrt

