/* Generated from paths.h.in (Sat Aug 22 16:54:43 PDT 1992).  */
/* Paths.  */

/* The meanings of these paths are described in the documentation.  They are
   exactly the same as those in the TeX distribution.  */

/* The directories listed in these paths are searched for the various
   font files.  The current directory is always searched first.  */
#ifndef DEFAULT_TFM_PATH
#define DEFAULT_TFM_PATH "/usr/contrib/lib/tex/fonts//:."
#endif

#ifndef DEFAULT_PK_PATH
#define DEFAULT_PK_PATH "/usr/contrib/lib/tex/fonts//:."
#endif

#ifndef DEFAULT_VF_PATH
#define DEFAULT_VF_PATH "/usr/contrib/lib/tex/fonts//:."
#endif

#ifndef DEFAULT_CONFIG_PATH
#define DEFAULT_CONFIG_PATH ".:/usr/contrib/lib/dvips"
#endif

#ifndef DEFAULT_HEADER_PATH
#define DEFAULT_HEADER_PATH ".:/usr/contrib/lib/dvips:/usr/contrib/lib/tex/fonts//"
#endif

#ifndef DEFAULT_FIG_PATH
#define DEFAULT_FIG_PATH ".:/usr/contrib/lib/tex/macros//"
#endif

#ifndef DEFAULT_PICT_PATH
#define DEFAULT_PICT_PATH ".:/usr/contrib/lib/tex/macros//"
#endif



/* The following are not set from the Makefile.  */

/*
 *   OUTPATH is where to send the output.  If you want a .ps file to
 *   be created by default, set this to "".  If you want to automatically
 *   invoke a pipe (as in lpr), make the first character an exclamation
 *   point or a vertical bar, and the remainder the command line to
 *   execute.
 *
 *   Actually, OUTPATH should be overridden by an `o' line in config.ps.
 */
#define OUTPATH ""

/*
 *   Names of config and prologue files:
 */
#ifdef MSDOS
#define DVIPSRC "dvips.ini"
#else
#ifdef VMCMS  /* IBM: VM/CMS */
#define DVIPSRC "dvips.profile"
#else
#define DVIPSRC ".dvipsrc"
#endif  /* IBM: VM/CMS */
#endif

#define HEADERFILE "tex.pro"
#define CHEADERFILE "texc.pro"
#define PSFONTHEADER "texps.pro"
#define IFONTHEADER "finclude.pro"
#define SPECIALHEADER "special.pro"
#define COLORHEADER "color.pro"  /* IBM: color */
#define CROPHEADER "crop.pro"
#define PSMAPFILE "psfonts.map"
#ifndef CONFIGFILE
#define CONFIGFILE "config.ps"
#endif
