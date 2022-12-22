/* -*-C-*- dvihead.h */
/*-->dvihead*/
/**********************************************************************/
/****************************** dvihead *******************************/
/**********************************************************************/

/**********************************************************************/
/*************************  Revision History  *************************/
/**********************************************************************/

/***********************************************************************
[Begin Revision History]

<BEEBE.TEX.DVI.NEW>DVIHEAD.H.2, 24-Jun-86 18:11:55, Edit by BEEBE
Added saving of all ten TeX  page counters in prtpage() in global  array
tex_counter[], and  function tctos()  to convert  them to  a string  for
printing.  All warning and error messages are now accompanied by a  list
of the non-zero page counters to  help relate errors to output  document
pages.

<BEEBE.TEX.DVI>DVIHEADER.H.21, 13-Mar-86 10:43:33, Edit by BEEBE
Added use of  getenv() for  Unix and PC-DOS  to pick  up definitions  of
texinputs and texfonts at runtime; if these are available, they override
built-in choices.   This makes  it  possible to  move compiled  code  to
similar machines with different  directory structures without having  to
recompile.  For TOPS-20, this is unnecessary, since the defaults already
point to these, and a name of the form /texinputs/foo.bar is  translated
by PCC-20 to texinputs:foo.bar, from which the current definition of the
logical name "texinputs:" is used to find the desired directory.  It can
even be a  chain of directories;  in Unix  and PC-DOS, we  will need  to
introduce  a  special  version  of  fopen()  to  handle  filenames  with
environment variable prefixes which might represent directory chains.

Removed variable-length  argument lists  to  fatal() and  warning()  for
portability.  Changed all preprocessor  #ifdef's and #ifndef's to  #if's
for  portability;  all  symbols  for  devices,  operating  systems,  and
implementations are now explicitly defined to be 0 or 1.  IBM PC Lattice
C will not accept the operators  "!", "||", or "&&" in #if  expressions,
although they should be perfectly legal; rearranged several  expressions
to avoid these (introduce #else, use "|" and "&").

Added procedures fontfile() and fontsub() to encapsulate construction of
system-dependent font file names  and provide for user-specifiable  font
substitutions  for   unavailable   font  files   (new   runtime   option
-ffontfile).

Removed old #ifdef FOOBAR ... #endif code sections in several procedures
which were  completely obsolete.   Added header  comment line  to  every
file; it  contains  the  EMACS  "-*-C-*-"  mode  string  and  the  exact
(case-sensive) filename,  since many  functions have  been defined  with
names  in  mixed  case  for  readability  (probably  should  have   used
underscore instead, but none do), and on Unix, the letter case matters.

Replaced index()  and  rindex(),  which have  different  definitions  in
different C  implementations, by  4.2BSD (and  coming ANSI  C  standard)
functions strchr() and strrchr(), for which .h files are provided.


<BEEBE.TEX.DVI>DVIHEADER.H.11,  8-Jan-86 16:55:59, Edit by BEEBE
Added DVITYPE  Version 2.6  MAXDRIFT  correction to  pixel  coordinates.
This  adds  function  fixpos()   called  from  movedown(),   moveover(),
setchar(), and setrule(), and #include'd  in DVI*.c with definitions  in
gblprocs.h and machdefs.h.  Revision level incremented by 0.01 (most are
now at 2.01)


<BEEBE.TEX.DVI>DVIHEADER.H.7, 22-Jul-85 12:51:02, Edit by BEEBE
Added PostScript driver for Apple LaserWriter.

Added support for multiple input DVI  files to each of which all  switch
options apply.


<BEEBE.TEX>DVIJET.C.70, 30-May-85 00:33:51, Edit by BEEBE
Revised "int" type  declarations to signed  types (INT8, INT16,  INT32),
unsigned  types  (BYTE,   UNSIGN16,  UNSIGN32),   and  coordinate   type
(COORDINATE) as  a  prelude  to moving  to  microprocessors  where  long
integers impose a serious runtime penalty.

Type casts added to many assignments and arguments.

"double" changed to "float".

Added a few more "register" declarations and deleted unused variables.

Device-dependent code  sections identified  as prelude  to code  sharing
between device  drivers via  "#include"  statements for  each  shareable
procedure.

Added -c, -o, -r, -x, -y options and inch() procedure.

Several passes with "lint" under VAX Unix to detect further problems


<BEEBE.TEX>DVIJET.C.18, 27-May-85 23:44:34, Edit by BEEBE
Revise copy of Printronix driver for output on Hewlett-Packard Laser Jet


<BEEBE.TEX>DVIPRX.C.125, 20-Oct-84 14:02:27, Edit by BEEBE
Add mag_table[] and rewrite actfact() to use it.  Add code to readfont()
and openfont()  to  choose  nearest available  font  magnification  when
required one is  unavailable.  Enable  USEGLOBALMAG since  code now  can
handle it properly.  Allow upper-case  option letters as equivalents  of
lower-case ones -- non-Unix folks abhor such distinctions.


<BEEBE.TEX>DVIPRX.C.118, 30-Sep-84 14:42:10, Edit by BEEBE
Change = to  == in "if  (...)" in procedure  warning, change g_dolog  to
BOOLEAN.


[End Revision History]
***********************************************************************/

/**********************************************************************/
/************************  Development History  ***********************/
/**********************************************************************/

/***********************************************************************
**
**  The code is arranged  to allow easy modification  of the output  for
**  display on other dot matrix printers, and for porting to a new  host
**  computing environment.  The  sections labelled "Device  Definitions"
**  and "Global Definitions"  below definitely  need to  be modified  in
**  such a case.  A couple of the  40 or so procedures in the  '#include
**  "xxx.h"' section may need to be adjusted as well, but almost all the
**  rest should be both host- and output-device-independent.
**
**  The runtime switches will be similar for most devices, but some will
**  require additional ones.  Device-name conditionals should be used to
**  bracket these, so that code  can be lifted without modification  for
**  use in a new dvi driver.
**
**  Instead of building up a bit map corresponding to the final  printer
**  file, we keep a  large array which  has a one-to-one  correspondence
**  with the printed page; dots in  a horizontal raster on the page  are
**  consecutive in memory.   This allows  us to OR  in character  raster
**  patterns without having to unpack every single bit.  At  end-of-page
**  time, each  raster  row  is  trimmed of  trailing  white  space  and
**  formatted into a line of data to  be sent to the printer file.   For
**  those devices which  pack dots for  6, 7,  or 8 rows  into a  single
**  character, some merging of raster lines will be necessary.
**
**  To use the program, type:
**
**  dvixxx {-b} {-c#} {-d#} {-ffontsubfile} {-l} {-m#} {-o#:#} {-o#} {-p}
**	   {-r#} {-v} {-x#units} {-y#units} dvifile(s)
**
**  The order of command options and DVI file names is not  significant;
**  all switch values apply to all  DVI files.  DVI files are  processed
**  in order from left to right.   The command options are (letter  case
**  is IGNORED):
**
**	b	Backwards order printing from the default.  For example,
**		laser printers using the Canon engine print normally
**		receive pages in reverse order because they stack printed
**		side up.  Some have page handling mechanisms that stack
**		them face down, and in such a case -b will ensure that
**		they come out in order 1,2,... instead of n,n-1,n-2,...
**
**	c#	Print # copies of each output page.
**
**	d#	Debug output to stderr if non-zero value given.
**
**	ffontsubfile	Define an alternate font substitution file which
**		is to be used instead of the default ones (see below).
**
**	l	Inhibit logging.
**
**	m#	Reset magnification  to  #.   The  default  is  "-m603",
**		corresponding   to  (1/1.2**5)  magnification of  300dpi
**		fonts.   Legal  values  are int(1000*(1.2)**(k/2)) (k  =
**		-16,16); other values will be set to the nearest in this
**		family.   Not all fonts  will be available in  this wide
**		range, and most installations will probably have  only a
**		half dozen or so magnifications.
**
**	o# and
**	o#:#	Specify a page number,  or range of page numbers,  to be
**		selected for output.  This  option may be specified  any
**		number of times.  If it is not specified, then all pages
**		will be printed.  Pages are numbered in order  1,2,3,...
**		in the file, but any page number recorded by TeX on  the
**		printed page will in general be different.  As pages are
**		selected  for  printing,  "[#{#}"  will  be  printed  on
**		stderr, where the first is the page number in the  file,
**		and the second is the value of the TeX counter, \count0,
**		which usually records the printed page number.  When the
**		page is completely output, a closing "]" will be printed
**		on stderr.  Any error  messages from processing of  that
**		page will therefore occur  between the square  brackets.
**		For example, "-o1:3 -o12 -o17:23" would select pages  1,
**		2, 3, 12, 17, 18, 19,  20, 21, 22, and 23 for  printing.
**		Pages will always be printed in an order appropriate for
**		the device so that the first document page occurs  first
**		face up in the document stack.
**
**	p	Inhibit font preloading.  This may produce output a  few
**		seconds earlier when  all pages are  output, but  should
**		have  negligible  effect  on  the  execution  time,  and
**		consequently, should  normally not  be specified.   When
**		individual pages are being printed with the -o#  option,
**		preloading is necessary (and  will be forced) to  ensure
**		that all fonts are defined before they are referenced.
**
**	q	Quiet mode.   Status displays to stderr are  suppressed,
**		unless warning or error messages are issued.
**
**	r#	(Device =  HP Laser Jet  only).  Specify the  Laser  Jet
**		output resolution in dots per inch.  "#" must be one  of
**		75, 100, 150, or 300.  The actual plot file is identical
**		in each  case;  only the  size  on the  output  page  is
**		changed, because the  resolution change  is effected  by
**		printing 1 x 1, 2 x 2, 3 x 3, or 4 x 4 pixel blocks.
**
**	r	(Device  =  Golden Laser 100 only).   Select  run-length
**		encoding of the  output file.  This  reduces disk  space
**		typically by 10% to 40%, but increases host CPU time for
**		the preparation of the output file.
**
**	r	(Device  = Apple ImageWriter only).   Select  run-length
**		encoding of the output file.
**
**	r	(Device  =  Toshiba  P-1351  only).   Select  run-length
**		encoding of the  output file.  This  reduces disk  space
**		typically by 10% to 40%, but increases host CPU time for
**		the preparation of the output file, and because of  poor
**		logic in the  printer, may double  the print time!   The
**		print quality  is  also  substantially  worse,  so  this
**		option is generally NOT recommended.
**
**	s#	(Device =  Apple  LaserWriter only).   Force  characters
**		larger than # pixels  wide or high  to be reloaded  each
**		time they  are required.   The Version  23.0  PostScript
**		interpreter has a  bug which manifests  itself in  fatal
**		'VM error' messages when  large characters are sent.   A
**		reasonable default  value has  been set  for this  which
**		should normally avoid the problem.  Specifying -s0  will
**		cause reloading of every character each time it is used.
**
**	v	(Device = Apple LaserWriter  only).  Force reloading  of
**		all required fonts at start of each page.
**
**	x#bp	big point (1in = 72bp)
**	x#cc	cicero (1cc = 12dd)
**	x#cm	centimeter
**	x#dd	didot point (1157dd = 1238pt)
**	x#in	inch
**	x#mm	millimeter (10mm = 1cm)
**	x#pc	pica (1pc = 12pt)
**	x#pt	point (72.27pt = 1in)
**	x#sp	scaled point (65536sp = 1pt)
**		Specify the left margin  of the TeX  page on the  output
**		page in any of the indicated units.  Letter case is  not
**		significant  in  the  unit  field,  which  must  not  be
**		separated from  the  number  by any  space.   #  may  be
**		fractional.    For   example,   "-x1.0in",   "-x2.54cm",
**		"-x72.27pt", and  "-x6.0225pc" all  specify a  one  inch
**		left margin.  Negative values  are permissible, and  may
**		be  used  to  shift  the  output  page  left   (possibly
**		truncating it on the  left) in order  to display a  wide
**		TeX page.
**
**	y#	inch
**	y#bp	big point (1in = 72bp)
**	y#cc	cicero (1cc = 12dd)
**	y#cm	centimeter
**	y#dd	didot point (1157dd = 1238pt)
**	y#in	inch
**	y#mm	millimeter (10mm = 1cm)
**	y#pc	pica (1pc = 12pt)
**	y#pt	point (72.27pt = 1in)
**	y#sp	scaled point (65536sp = 1pt)
**		Specify the top  margin of  the TeX page  on the  output
**		page in any of the indicated units.  Letter case is  not
**		significant  in  the  unit  field,  which  must  not  be
**		separated from  the  number  by any  space.   #  may  be
**		fractional.    For   example,   "-y1.0in",   "-y2.54cm",
**		"-y72.27pt", and "-y6.0225pc" all specify a one inch top
**		margin.  Negative  values are  permissible, and  may  be
**		used to shift the output page up (possibly truncating it
**		on the top) in order to display a long TeX page.
**
**
**  If no  -ffontsubfile  option  is given,  and  font  substitution  is
**  required,   the   files   "dvifile.sub"   (minus   any   extension),
**  "texfonts.sub", and "texinputs:texfonts.sub" will be tried in order.
**  The first two will be found  on the current directory, and the  last
**  is the system default.  This gives the option of  document-specific,
**  user-specific, and system-specific substitutions, and the -f  option
**  allows all of these to be overridden.
**
**  This program can only process DVI format 2 files.
**
**  Mark Senn  at Purdue  University wrote  the first  BitGraph  driver,
**  dvibit.  This was further worked on at the University of  Washington
**  by Stephen  Bechtolsheim,  Bob  Brown, Richard  Furuta,  and  Robert
**  Wells.  The transformation to about  ten other device drivers,  plus
**  the massive code rearrangement for many new features as well as easy
**  identification of host- and  device-dependent sections, was  carried
**  out at the University of Utah by Nelson H.F. Beebe.
**
**  Support for  pixel file  caching and  character raster  caching  was
**  added by NHFB;  this should give  a decided performance  improvement
**  provided the cache  sizes, MAXCACHE and  MAXOPEN, defined below  are
**  sufficiently large.
**
**  MAXOPEN can be  set as low  as 1,  but should ideally  be about  15,
**  since this may be more typical of the number of different font files
**  required in mathematical manuscripts.
**
**  In the current version,  MAXCACHE is not  actually used, because  it
**  appears sufficient raster storage will always be available even  for
**  relatively complex  manuscripts.  However,  a record  is kept  which
**  will permit dynamic freeing and reallocation of raster storage;  see
**  procedure loadchar for details.
**
***********************************************************************/


/**********************************************************************/
/************************  Global Definitions  ************************/
/**********************************************************************/

/* All host-specific material resides in machdefs.h and typedefs.h;
they must be revised when this dvi driver is rehosted to a new machine */

#define DEBUG	1		/* for optional massive trace printing */
#undef DEBUG			/* do not want this now */

#include "machdefs.h"		/* all host-specific defines */

#include "typedefs.h"		/* typedefs (also host-specific) */

#include <stdio.h>		/* must come after the others to */
				/* enable type checking */
#include <errno.h>		/* needed only for DISKFULL() definition */
#if    (BSD41 | BSD42)
extern int errno;		/* not in all errno.h files, sigh... */
#endif

#if    IBM_PC_MICROSOFT
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#endif

/* types.h and stat.h are needed for fstat(), which is used by special()
and the virtual font mechanism in openfont.h to get the input file size. */

#if    (OS_ATARI | OS_VAXVMS | KCC_20)
#include <types.h>
#include <stat.h>
#else /* NOT (OS_ATARI | OS_VAXVMS | KCC_20) */
#include <sys/types.h>
#include <sys/stat.h>
#endif /* (OS_ATARI | OS_VAXVMS | KCC_20) */

