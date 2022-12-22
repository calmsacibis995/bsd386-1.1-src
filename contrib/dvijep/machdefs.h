/* -*-C-*- machdefs.h */
/*-->machdefs*/
/**********************************************************************/
/****************************** machdefs ******************************/
/**********************************************************************/

/***********************************************************************

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This file should contain definitions for symbols used for all  operating
system /  implementation  dependencies, and  if  the driver  family  has
already been implemented on  you machine, this should  be the only  file
requiring changes.

How to change this file:
	* locate the operating system and implementation definitions;
	  they are surrounded by "=====" comment strings.
	* comment out the definitions you do not want, and select the
	  ones for your system
	* if adding a new operating system, create a new symbol OS_xxx
	  for it and add a new #if OS_xxx ... #endif section for its
	  changes.
        * if adding a new implementation for an existing operating
	  system, create a new symbol for it and add appropriate
	  conditionals inside its #if OS_xxx ... #end section.
	* if you must replace a standard C library function, replace
	  instances of its use in the source code with an upper-case
	  equivalent (e.g. ungetc --> UNGETC), then define the
	  upper-case name below in the generic section, plus the
	  operating-system section.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

List of symbols actually used for #if's [14-Aug-87]

Flags and strings:
	ALLOW_INTERRUPT	-- allow interactive interrupt
	ANSI_PROTOTYPES	-- draft ANSI C function prototype declarations are
			   supported
	ANSI_LIBRARY	-- draft ANSI C library functions conformance
	ARITHRSHIFT	-- implementation uses arithmetic (not logical) right
			   shift
	DVIHELP		-- how to find documentation (for usage())
	DVIPREFIX	-- prefix to standard 3-character extension of output
			   and log files
	FASTZERO	-- fast bitmap zeroing by external assembly language
			   routine
	FONTLIST	-- font type search list (PK, GF, PXL)
	FONTPATH	-- font directory path
	HIRES		-- high resolution variant of bitmap output
	HOST_WORD_SIZE	-- host integer word size in bits
        PS_MAXWIDTH	-- approximate line width limit for PostScript output
	PS_SHORTLINES	-- shorter output lines in PostScript
	PS_XONXOFFBUG	-- PostScript version 23.0 Xon/Xoff bug workaround
	PXLID		-- TeX PXL file ID
	RB_OPEN		-- fopen() mode flags for binary read
	SEGMEM		-- segmented memory (Intel); bitmap is raster vector
	STDRES		-- standard resolution (200 dpi)
	SUBPATH		-- substitution font file path
	TEXFONTS	-- TeX font file path environment variable
	TEXINPUTS	-- TeX input file path environment variable
	USEGLOBALMAG	-- allow runtime global magnification scaling
	VIRTUAL_FONTS	-- implement virtual font caching
	WB_OPEN		-- fopen() mode flags for binary write
	ZAPTHISOUT	-- remove some obsolete code

Alternate library routines  for misfeature workarounds  (these have  the
same names as standard library routines, but upper-cased):
	EXIT
	FOPEN
	FSEEK
	FTELL
	GETENV
	MALLOC(n)
	READ
	REWIND(fp)
	UNGETC

C Implementations:
	ATT		-- AT&T Unix (System III, V)
	BSD41		-- Berkeley 4.1BSD
	BSD42		-- Berkeley 4.2BSD
	HPUX		-- HP 9000 series Unix (System V based)
	IBM_PC_LATTICE	-- IBM PC Lattice C compiler
	IBM_PC_MICROSOFT-- IBM PC Microsoft Version 3.x or later C compiler
	IBM_PC_WIZARD	-- IBM PC Wizard C compiler
	KCC_20		-- SRI's KCC Compiler on TOPS-20
	PCC_20		-- Portable C Compiler on TOPS-20

Operating systems:
	OS_ATARI	-- Atari 520ST+ TOS (similar to MS DOS)
	OS_PCDOS	-- IBM (and clones) PC DOS and MS DOS
	OS_TOPS20	-- DEC-20 TOPS-20
	OS_UNIX		-- Unix (almost any variant)
	OS_VAXVMS	-- VAX VMS

Device names (defined in each DVIxxx.C file):
	APPLEIMAGEWRITER -- Apple ImageWriter printer
	BBNBITGRAPH	-- BBN BitGraph terminal
	CANON_A2	-- Canon LBP-8 A2 laser printer
	DECLA75		-- DEC LA75 printer
	DECLN03PLUS	-- DEC LN03-PLUS laser printer
	EPSON		-- Epson 9-pin family dot-matrix printer
	GOLDENDAWNGL100	-- Golden Dawn GL100 laser printer
	HPJETPLUS	-- Hewlett-Packard Laser Jet Plus (downloaded fonts)
	HPLASERJET	-- Hewlett-Packard Laser Jet (bitmap display)
	IMPRESS		-- imPRESS (IMAGEN laser printer)
	MPISPRINTER	-- MPI Sprinter printer
	OKIDATA2410	-- OKIData 2410 printer
	POSTSCRIPT	-- Adobe PostScript (Apple LaserWriter laser printer)
	PRINTRONIX	-- Printronix (DEC LXY-11, C-Itoh) printer
	TOSHIBAP1351	-- Toshiba P-1351 dot matrix printer

***********************************************************************/

/**********************************************************************
Define all symbols for devices, operating systems, and implementations
to be explicitly 0, unless it is expected that they might be set at
compile time.
***********************************************************************/

#define ALLOW_INTERRUPT	0
#define ANSI_PROTOTYPES	0

#ifndef ANSI_LIBRARY		/* may be specified at compile time */
#define ANSI_LIBRARY	0
#endif

#if    ANSI_LIBRARY
#undef ANSI_PROTOTYPES
#define ANSI_PROTOTYPES	1	/* If library conforms, declarations do too */
#endif

#define ARITHRSHIFT	1 /* most C compilers use arithmetic right shift */
#define DISKFULL(fp)	(ferror(fp) && (errno == ENOSPC))
#define DVIEXT		".dvi"
#define DVIPREFIX	"dvi-"
#define EXIT		exit
#define FASTZERO	0

/* The following definitions work for at least PCC-20, BSD 4.2 and  4.3,
and HPUX;  VAX  VMS  has  an extra  level  of  indirection.   Check  the
definition of fileno(fp) in stdio.h; on PCC-20, it is
	#define fileno(p) ((p)->_file)
*/
#define FILE_CNT(fp)	(fp)->_cnt
#define FILE_BASE(fp)	(fp)->_base
#define FILE_PTR(fp)	(fp)->_ptr

/* #define FONTLIST	0 -- can be set at compile time */
/* #define FONTPATH	0 -- can be set at compile time */

#define FOPEN		fopen
#define FSEEK		fseek
#define FTELL		ftell
#define GETENV		getenv
#define HIRES		0
#define MALLOC(n)	malloc(n)
#define MAXDRIFT	2	/* we insist that
				abs|(hh-pixel_round(h))<=MAXDRIFT| */

/* MAXOPEN  should  be 6  less  than the  system  limit on  open  files,
allowing for  files  open  on stdin,  stdout,  stderr,  .dvi,  .dvi-log,
.dvi-xxx, plus MAXOPEN font  files.  It may  be additionally limited  by
the amount of memory available for buffers (e.g. IBM PC). */
#define MAXOPEN		14

/* #define PS_MAXWIDTH  72 -- can be set at compile time */

/* #define PS_SHORTLINES 0 -- can be set at compile time */

#define PS_XONXOFFBUG	0
#define PXLID		0
#define RB_OPEN		"r"

/* For virtual font caching to succeed, read() must return the requested
number of bytes, and  preferably do this  with one system   call  and no
double buffering. */

#define READ		read

/* In  many  implementations, rewind(fp)  is  defined as  equivalent  to
fseek(fp,0L,0).  In  some, however  (e.g.  PCC-20,  and probably  others
based on PCC), it additionally discards input buffer contents, which may
cause unnecessary I/O, and in the case of virtual font caching,   clears
the cache.  Defining  it in  terms of fseek()  should be  okay, but  the
implementation of fseek() should be checked. */

#define REWIND(fp)	FSEEK(fp,0L,0)

#define SEGMEM		0	/* may be reset by dvixxx for big bitmaps */
#define STDRES		0

/* #define SUBPATH	0 -- can be set at compile time */

#define SUBEXT		".sub"
#define SUBNAME		"texfonts"

/* #define TEXFONTS	0 -- can be set at compile time */
/* #define TEXINPUTS	0 -- can be set at compile time */

#define UNGETC		ungetc
#define USEGLOBALMAG	0
#define WB_OPEN		"w"
#define ZAPTHISOUT	0


/**********************************************************************/
/* Clear all implementation/operating-system flags--reset later */

#define ATT		0	/* define zero or one of these */
#define BSD41		0
#define BSD42		0
#define HPUX		0
#define IBM_PC_LATTICE	0
#define IBM_PC_MICROSOFT	0
#define IBM_PC_WIZARD	0
#define KCC_20		0
#define PCC_20		0

#define OS_ATARI	0	/* define one of these */
#define OS_PCDOS	0
#define OS_TOPS20	0
#define OS_UNIX		0
#define OS_VAXVMS	0

#define APPLEIMAGEWRITER	0	/* one will be defined by DVIxxx */
#define BBNBITGRAPH	0
#define CANON_A2	0
#define DECLA75		0
#define DECLN03PLUS	0
#define EPSON		0
#define GOLDENDAWNGL100	0
#define HPJETPLUS	0
#define HPLASERJET	0
#define IMPRESS		0
#define MPISPRINTER	0
#define OKIDATA2410	0
#define POSTSCRIPT	0
#define PRINTRONIX	0
#define TOSHIBAP1351	0
#define VIRTUAL_FONTS	0


/***********************************************************************
Define operating system and implementation  here.  Since these have  all
been explicitly set  to 0  above, we  issue #undef's  to avoid  compiler
macro redefinition warning messages.
***********************************************************************/


/*====================
#undef PCC_20
#undef OS_TOPS20
#define PCC_20		1
#define OS_TOPS20	1
====================*/

/*====================
#undef KCC_20
#undef OS_TOPS20
#define KCC_20		1
#define OS_TOPS20	1
====================*/

/*====================
#undef  OS_ATARI
#define OS_ATARI	1
====================*/

/*====================
#undef  IBM_PC_LATTICE
#undef  OS_PCDOS
#define IBM_PC_LATTICE	1
#define OS_PCDOS	1
====================*/

/*====================
#undef  IBM_PC_MICROSOFT
#undef  OS_PCDOS
#define IBM_PC_MICROSOFT	1
#define OS_PCDOS	1
====================*/

/*====================
#undef  IBM_PC_WIZARD
#undef  OS_PCDOS
#define IBM_PC_WIZARD	1
#define OS_PCDOS	1
====================*/

/*====================
#undef  OS_VAXVMS
#define OS_VAXVMS	1
====================*/

#if    (OS_ATARI | OS_PCDOS | OS_TOPS20 | OS_UNIX | OS_VAXVMS)
#else
#undef  OS_UNIX
#define OS_UNIX		1		/* provide default operating system */
#endif


/**********************************************************************/

#if    OS_ATARI

#undef  BSD42
#define BSD42		1

#undef  DISKFULL
#define DISKFULL(fp)	ferror(fp)

#define DVIHELP 	"type e:\\tex\\dvi.hlp"

#ifdef  FONTLIST 	/* can be set at compile time */
#else
#define FONTLIST	"PK-GF-PXL"	/* preferred search order */
#endif /* FONTLIST */

#ifdef FONTPATH 		/* can be set at compile time */
#else
#define FONTPATH	"e:\\tex\\fonts\\"
#endif

#define HOST_WORD_SIZE	32	/* must be 32 or larger -- used in */
				/* signex to pack 8-bit bytes back */
				/* into integer values, and in dispchar */
				/* and fillrect for managing character */
				/* raster storage. */
#define MAXFNAME	64	/* longest host complete filename */

#ifndef PS_MAXWIDTH
#define PS_MAXWIDTH	72
#endif

#ifndef PS_SHORTLINES
#define PS_SHORTLINES	1
#endif

#ifdef SUBPATH			/* can be set at compile time */
#else
#define SUBPATH 	"e:\\tex\\inputs\\"
#endif

#ifdef TEXINPUTS		/* can be set at compile time */
#else
#define TEXINPUTS	"TEXINPUTS"
#endif

#define TEXFONTS	"TEXFONTS"

#endif /* OS_ATARI */


/**********************************************************************/

#if    OS_PCDOS

#if    IBM_PC_MICROSOFT
#undef ANSI_PROTOTYPES
#define ANSI_PROTOTYPES	1

/*
Argument type checking in MSC Version 4.0 is selected by LINT_ARGS.  MSC
Version 5.0 has it selected  by default.  For Version 5.0,  ANSI_LIBRARY
should be defined at compile time  so as to get ANSI-conformant  library
function declarations.  Treating float as double eliminates lots of data
conversion warnings with both Versions 4.0 and 5.0.
*/
#define float double	
#define LINT_ARGS	1

#undef MALLOC
#define MALLOC(n)	calloc(n,1)
#endif /* IBM_PC_MICROSOFT */

#define DVIHELP		"type d:\\tex\\dvi.hlp"

#undef DVIPREFIX
#define DVIPREFIX	""

#ifdef FONTLIST		/* can be set at compile time */
#else
#define FONTLIST	"PK-GF-PXL"	/* preferred search order */
#endif /* FONTLIST */

#ifdef FONTPATH			/* can be set at compile time */
#else
#define FONTPATH	"d:\\tex\\fonts\\"
#endif /* FONTPATH */

#define HOST_WORD_SIZE	32	/* must be 32 or larger -- used in */
				/* signex to pack 8-bit bytes back */
				/* into integer values, and in dispchar */
				/* and fillrect for managing character */
				/* raster storage. */
#define MAXFNAME	64	/* longest host complete filename */

#undef MAXOPEN
#define MAXOPEN		5	/* limit on number of open font files */

#ifndef PS_MAXWIDTH
#define PS_MAXWIDTH	72
#endif

#ifndef PS_SHORTLINES
#define PS_SHORTLINES	1
#endif

#undef  RB_OPEN
#define RB_OPEN		"rb"

#ifdef SUBPATH			/* can be set at compile time */
#else
#define SUBPATH		"d:\\tex\\inputs\\"
#endif

#if    TEXINPUTS		/* can be set at compile time */
#else
#define TEXINPUTS	"TEXINPUTS"
#endif

#define TEXFONTS	"TEXFONTS"

#if    IBM_PC_MICROSOFT
#undef VIRTUAL_FONTS
#define VIRTUAL_FONTS	1
#endif

#undef  WB_OPEN
#define WB_OPEN		"wb"

#endif /* OS_PCDOS */


/***********************************************************************/
#if    OS_TOPS20

/************************************************************************
**
**  Adapted for the DEC-20 TOPS-20  operating system with Jay  Lepreau's
**  PCC-20  by  Nelson  H.F.	Beebe,  College  of  Science   Computer,
**  University of Utah, Salt Lake City, UT 84112, Tel: (801) 581-5254.
**
**  The PCC_20 switch is  used to get around  variations on the  DEC-20.
**  The major one is  that text files have  7-bit bytes, while the  .DVI
**  file and the font files have 8-bit bytes.  For the latter, we use  a
**  routine f20open which provides  the necessary interface for  opening
**  with a ddifferent byte size.  PCC-20 follows many other C  compilers
**  in that only the first 8 characters of identifiers are looked at, so
**  massive substitutions  were  necessary  in the  file  commands.h  to
**  shorten the long names there.
**
**  The PCC_20 switch is also used  to get variant font directory  names
**  and to select TOPS-20 jsys  code.  TOPS-20 is a wonderous  operating
**  system with  capabilities far  beyond  most of  its  contemporaries.
**  Like Topsy, it  just grew, and  consequently, its many  capabilities
**  are not  well  integrated.	 The terminal  control  jsys'es  (MTOPR,
**  RFMOD, SFMOD, STPAR, RFCOC, SFCOC and TLINK) are particularly poorly
**  done -- RFMOD returns  the JFN mode word,  particular bits of  which
**  must be set by SFMOD,  STPAR, and TLINK.  Why  could there not be  a
**  "return  the  terminal  state"  and  "restore  the  terminal  state"
**  jsys'es?  Some of this  may in fact be  already integrated into  the
**  PCC-20 C run-time library, but since it is totally undocumented  (an
**  all-too common problem with C),  it is essentially unusable in  that
**  form.
**
**  The OS_TOPS20 switch is used in one place to get ioctl.h included at
**  the right point, and in several places to get error messages  output
**  with Tops-20 conventions in  column 1: query  (?)  causes batch  job
**  abort, percent (%) flags a warning.
**
***********************************************************************/

#if    KCC_20
/* KCC wants all #if symbols defined before use. */
#ifndef FONTLIST
#define FONTLIST "PK-GF-PXL"
#endif

#ifndef FONTPATH
#define FONTPATH "TEXFONTS:"
#endif

#ifndef SUBPATH
#define SUBPATH "TEXINPUTS:"
#endif

#ifndef TEXFONTS
#define TEXFONTS "TEXFONTS:"
#endif

#ifndef TEXINPUTS
#define TEXINPUTS "TEXINPUTS:"
#endif

#undef VIRTUAL_FONTS
#define VIRTUAL_FONTS 0		/* cannot support this yet */

#endif

#if    KCC_20
#include <local-jsys.h>
/* #include <jsys.h> */
/* KCC-20 and PCC-20  have similar enough JSYS  interfaces that we  just
define values for KCC-20 using PCC-20 names. */
#define JSchfdb	CHFDB
#define JSmtopr	MTOPR
#define JSrfcoc	RFCOC
#define JSrfmod	RFMOD
#define JSsfcoc	SFCOC
#define JSsfmod	SFMOD
#define JSsti	STI
#define JSstpar	STPAR
#define JStlink	TLINK
#define Getmask(name) 	 ( 1?name )
#define Getshift(name)	 ( 0?name )
#define Absmask(name) ( (1?name) << (0?name) )    /* maybe use this one */
#define Value(name)   ( (1?name) << (0?name) )    /* maybe use this one */
#define makefield(name, value)	( ((value) & Getmask(name)) << Getshift(name) )
#define getfield(var, name)	( (var) >> Getshift(name) & Getmask(name) )
#define setfield(var, name, value) ( (var) = ((var) & ~Absmask(name)) |\
	makefield(name, value) )
#endif

#if    PCC_20
#undef  ARITHRSHIFT
#define ARITHRSHIFT	0	/* PCC-20 uses logical right shift */
#undef  DISKFULL
#define DISKFULL(fp)	ferror(fp)	/* PCC-20 does not always set errno */
#endif

#define DVIHELP	"help dvi\nor\ntype hlp:dvi.hlp\nor\nxinfo local clsc dvi"

#if    PCC_20
#undef FASTZERO
#define FASTZERO	1	/* for fast assembly language memory zeroing */
#endif

#ifdef FONTLIST		/* can be set at compile time */
#else
#define FONTLIST	"PK-GF-PXL"	/* preferred search order */
#endif /* FONTLIST */

#ifdef FONTPATH			/* can be set at compile time */
#else
#define FONTPATH	"/texfonts/"
#endif

#undef FOPEN
#define FOPEN		f20open	/* private version for 8-bit binary */

#define HOST_WORD_SIZE	36

#undef MAXFNAME
#define MAXFNAME	256	/* longest host complete filename */

#if    KCC_20
#undef MAXOPEN
#define MAXOPEN		26
#endif

#if    PCC_20
#undef MAXOPEN
#define MAXOPEN		14
#endif

#ifndef PS_MAXWIDTH
#define PS_MAXWIDTH	72
#endif

#ifndef PS_SHORTLINES
#define PS_SHORTLINES	1
#endif

#undef  RB_OPEN
#define RB_OPEN		"rb"

#if    PCC_20
#undef READ
#define READ		_read	/* fast version with one system call */
#endif
				/* and single buffering */
#ifdef SUBPATH
#else
#define SUBPATH		"/texinputs/"
#endif

#ifdef TEXFONTS			/* can be set at compile time */
#else
#define TEXFONTS	"TEXFONTS"
#endif

#ifdef TEXINPUTS		/* can be set at compile time */
#else
#define TEXINPUTS	"TEXINPUTS"
#endif

#if    PCC_20
#undef VIRTUAL_FONTS
#define VIRTUAL_FONTS	1
#endif

#undef  WB_OPEN
#define WB_OPEN		"wb"

/**********************************************************************/
/* The following definitions (down to the endif) are taken from */
/* monsym.h.   It is too big for CPP to handle, so this kludge is */
/* necessary until CPP's tables can be enlarged. */

/* selected fields for CHFDB% */
#define CF_nud		01:35-0		/* no update directory */
#define CF_dsp		0777:35-17	/* fdb displacement */
#define CF_jfn		0777777:35-35	/* jfn */

#define FBbyv		011		/* retention+bytesize+mode,,# of pages*/
#define FB_ret		077:35-5	/* retention count */


/* tty mode definitions */

#define MOrlw		030		/* read width */
#define MOslw		031		/* set width */
#define MOrll		032		/* read length */
#define MOsll		033		/* set length */

#define MOsnt		034		/* set tty non-terminal status */
#define MOsmn		01		/* no system messages(i.e. suppress) */
#define MOsmy		00		/* yes system messages(default) */
#define MOrnt		035		/* read tty non-terminal status */

/* fields of jfn mode word */

#define TT_osp		01:35-0		/* output suppress */
#define TT_mff		01:35-1		/* mechanical formfeed present */
#define TT_tab		01:35-2		/* mechanical tab present */
#define TT_lca		01:35-3		/* lower case capabilities present */
#define TT_len		0177:35-10	/* page length */
#define TT_wid		0177:35-17	/* page width */
#define TT_wak		017:35-23	/* wakeup field */
#define TT_wk0		01:35-18	/* wakeup class 0 (unused) */
#define TT_ign		01:35-19	/* ignore tt_wak on sfmod */
#define TT_wkf		01:35-20	/* wakeup on formating control chars */
#define TT_wkn		01:35-21	/* wakeup on non-formatting controls */
#define TT_wkp		01:35-22	/* wakeup on punctuation */
#define TT_wka		01:35-23	/* wakeup on alphanumerics */
#define TT_eco		01:35-24	/* echos on */
#define TT_ecm		01:35-25	/* echo mode */
#define TT_alk		01:35-26	/* allow links */
#define TT_aad		01:35-27	/* allow advice (not implemented) */
#define TT_dam		03:35-29	/* data mode */
#define TTbin		00		/* binary */
#define TTasc		01		/* ascii */
#define TTato		02		/* ascii and translate output only */
#define TTate		03		/* ascii and translate echos only */
#define TT_uoc		01:35-30	/* upper case output control */
#define TT_lic		01:35-31	/* lower case input control */
#define TT_dum		03:35-33	/* duplex mode */
#define TTfdx		00		/* full duplex */
#define TT0dx		01		/* not used, reserved */
#define TThdx		02		/* half duplex (character) */
#define TTldx		03		/* line half duplex */
#define TT_pgm		01:35-34	/* page mode */
#define TT_car		01:35-35	/* carrier state */

/* tlink */

#define TL_cro		01:35-0		/* clear remote to object link */
#define TL_cor		01:35-1		/* clear object to remote link */
#define TL_eor		01:35-2		/* establist object to remote link */
#define TL_ero		01:35-3		/* establish remote to object link */
#define TL_sab		01:35-4		/* set accept bit for object */
#define TL_abs		01:35-5		/* accept bit state */
#define TL_sta		01:35-6		/* set or clear advice */
#define TL_aad		01:35-7		/* accept advice */
#define TL_obj		0777777:35-35	/* object designator */

#endif /* OS_TOPS20 */

/**********************************************************************/

#if    OS_UNIX

#ifdef bsdi
#define BSD44 1
#endif

#ifdef __STDC__
#undef ANSI_PROTOTYPES
#define ANSI_PROTOTYPES 1
#undef ANSI_LIBRARY
#define ANSI_LIBRARY 1
#endif

#undef BSD42
#define BSD42		1		/* want DVISPOOL code in dviterm.h */

#ifdef hpux
#undef BSD42
#undef HPUX
#define HPUX		1	
#undef DVIPREFIX		/* HP-UX has 14-character name limit */
#define DVIPREFIX	""
#endif

#define DVIHELP		"man dvi\nor\napropos dvi"

#ifdef FONTLIST		/* can be set at compile time */
#else
#define FONTLIST	"PK-GF-PXL"	/* preferred search order */
#endif /* FONTLIST */

#ifdef FONTPATH			/* can be set at compile time */
#else
#define FONTPATH	"/usr/lib/tex/fonts/"
#endif

#undef MAXOPEN

#if    HPUX
#define MAXFNAME	1024	/* longest host complete filename */
#define MAXOPEN		50
#else  /* NOT HPUX */
#define MAXFNAME	256	/* longest host complete filename */
#define MAXOPEN		14
#endif /* HPUX */

#ifndef PS_MAXWIDTH
#define PS_MAXWIDTH	72
#endif

#ifndef PS_SHORTLINES
#define PS_SHORTLINES	1       /* some Unix utilities fail with long lines */
#endif

#ifdef SUBPATH			/* can be set at compile time */
#else
#define SUBPATH		"/usr/lib/tex/macros/"
#endif

#if    TEXINPUTS		/* can be set at compile time */
#else
#define TEXINPUTS	"TEXINPUTS"
#endif

#if    TEXFONTS			/* can be set at compile time */
#else
#define TEXFONTS	"TEXFONTS"
#endif

#define HOST_WORD_SIZE	32	/* must be 32 or larger -- used in */
				/* signex to pack 8-bit bytes back */
				/* into integer values, and in dispchar */
				/* and fillrect for managing character */
				/* raster storage. */
#endif /* OS_UNIX */


/**********************************************************************/

#if    OS_VAXVMS

/***********************************************************************
** Several standard Unix library functions do not work properly with VMS
** C, or are not implemented:
**
**	exit()		-- wrong conventions for return code
**	fseek()		-- fails on record-oriented files
**	ftell()		-- fails on record-oriented files
**	getchar()	-- waits for <CR> to be typed
**	getenv()	-- colon- and case-sensitive
**	getlogin()	-- not implemented
**	qsort()		-- not implemented
**	tell()		-- not implemented
**	ungetc()	-- fails for any character with high-order bit set
**	unlink()	-- not implemented (equivalent available)
**
** The  file  VAXVMS.C  contains   workarounds;  it  must  be   compiled
** separately and loaded with each of the DVI drivers.
***********************************************************************/

#include <jpidef.h>		/* need for getjpi() in openfont() */

#define DVIHELP		"help dvi\nor\ntype tex_inputs:dvi.hlp"
#define EXIT		vms_exit

#define FILE_CNT(fp)	(*fp)->_cnt
#define FILE_BASE(fp)	(*fp)->_base
#define FILE_PTR(fp)	(*fp)->_ptr

#ifndef FONTLIST		/* can be set at compile time */
#define FONTLIST	"PK-GF-PXL"	/* preferred search order */
#endif /* FONTLIST */

#ifndef FONTPATH		/* can be set at compile time */
#define FONTPATH	"TEX_FONTS:" /* Kellerman & Smith VMS TeX */
#endif /* FONTPATH */

#define FSEEK		vms_fseek
#define FTELL		vms_ftell
#define GETENV		vms_getenv
#define HOST_WORD_SIZE	32	/* must be 32 or larger -- used in */
				/* signex to pack 8-bit bytes back */
				/* into integer values, and in dispchar */
				/* and fillrect for managing character */
				/* raster storage. */

#define MAXFNAME	256	/* longest host complete filename */

#undef MAXOPEN
#define MAXOPEN		14

#undef  RB_OPEN
#define RB_OPEN		"rb"

#ifndef PS_MAXWIDTH
#define PS_MAXWIDTH	72
#endif

#ifndef  PS_SHORTLINES
#define PS_SHORTLINES	1       /* VMS has trouble with long lines */
#endif

#define READ		vms_read /* ordinary read() returns only one disk */
				/* at each call */

#ifndef SUBPATH			/* can be set at compile time */
#define SUBPATH		"TEX_INPUTS:" /* Kellerman & Smith VMS TeX */
#endif

#ifndef TEXINPUTS		/* can be set at compile time */
#define TEXINPUTS	"TEX_INPUTS:"
#endif

#ifndef TEXFONTS		/* can be set at compile time */
#define TEXFONTS	"TEX_FONTS:"
#endif

#define UNGETC		vms_ungetc

/* VIRTUAL_FONTS cannot be implemented  yet.  The code  works, but the
calls to FSEEK() (vms_seek) result in _filbuf() being called to refill
the buffer, obviating the pre-buffering.  Additional code in case 0 of
vms_seek() can probably be developed to avoid this, but I have run out
of time for now. */

#undef  WB_OPEN
#define WB_OPEN		"wb"

#endif /* OS_VAXVMS */

/**********************************************************************/
