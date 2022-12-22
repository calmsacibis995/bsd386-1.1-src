/* -*-C-*- fndfil.c */
/*-->findfile*/
/**********************************************************************/
/****************************** findfile ******************************/
/**********************************************************************/

/***********************************************************************
NB:  If  the macro DVI  is defined  when this  file  is compiled, then
additional code will be executed  in isfile() to  optionally trace the
lookup attempts.

If the macro TEST is defined at compile-time, a test main program will
be compiled  which takes paths and  names from stdin,  and echos their
existence and expansion to stdout.
***********************************************************************/

/***********************************************************************

Search for a file  in a list of  directories specified in the  pathlist,
and if found,  return a pointer  to an internal  buffer (overwritten  on
subsequent  calls)  containing  a   full  filename;  otherwise,   return
(char*)NULL.

The pathlist is  expected to  contain directories separated  by one  (or
more) of the  characters in SEPCHAR.   The search is  restricted to  the
directories in the path list; unlike PC DOS's PATH variable, the current
directory is NOT searched unless it is included in the path list.

For example,

	findfile(".:/usr/local/lib:/tmp", "foo.bar")

will check for the files

	./foo.bar
	/usr/local/lib/foo.bar
	/tmp/foo.bar

in that order.  The character which normally separates a directory  from
a file name will  be supplied automatically  if it is  not given in  the
path list.  Thus, the example above could have been written

	findfile("./:/usr/local/lib/:/tmp/","foo.bar")

Since multiple path separators are ignored, the following are also
acceptable:

	findfile("./ : /usr/local/lib/ : /tmp/","foo.bar")
	findfile("./   /usr/local/lib/   /tmp/","foo.bar")
	findfile("./:::::/usr/local/lib/:::::/tmp/:::::","foo.bar")

For VAX VMS, additional support is  provided for rooted logical names,
and names  that   look like  rooted  names, but  are not.  Normally, a
logical name  in a path  will look name  "LOGNAME:",  and a  file like
"FILE.EXT"; combining them gives "LOGNAME:FILE.EXT".  A rooted logical
name is created by a DCL command like

$ define /translation=(concealed,terminal) LOGNAME DISK:[DIR.]

In such a case, VMS requires that what  follows the logical name begin
with a directory, e.g.  the file is "[SUB]FILE.EXT".  Concatenation of
path with name gives "LOGNAME:[SUB]FILE.EXT", which is acceptable.  To
refer   to  a   file in   the   directory   DISK:[DIR],  one    writes
"LOGNAME:[000000]FILE.EXT", where 000000  is the magic  name of a root
directory.

If, however, the logical  name looks  like a  rooted name,  but wasn't
defined  with  "/translation=(concealed,terminal)", then VMS  will not
recognize it.   [That  is an easy mistake to  make, and  issuing a DCL
"show logical LOGNAME" command will NOT reveal the error.]  Similarly,
if the logical name  looks like a  directory name, and the file begins
with a directory name, VMS will not recognize that either, even though
there is no ambiquity about what is meant.

We therefore make the  following reductions after  an  initial attempt
fails.  First, expand the logical name, skipping further processing if
that fails.  Then, reduce the concatenation as follows:

	[DIR.]FILE.EXT          -->     [DIR]FILE.EXT
	[DIR.][SUB]FILE.EXT     -->     [DIR.SUB]FILE.EXT
	[DIR][SUB]FILE.EXT      -->     [DIR.SUB]FILE.EXT

If  the logical name  points to a  chain of names,  the standard VMS C
run-time library getenv()  will return  only the first  in  the chain.
Chained logical names were added in VMS version  4.0 without operating
system support  to retrieve all members  of the  chain.   The only way
I've been able to  find the subsequent  members is to spawn a  process
that runs a VMS DCL SHOW LOGICAL command, and then parse its output!

Under ATARI TOS, PC DOS, and UNIX,  environment variable  substitution
is supported as follows.  If the initial lookup fails, the filename is
checked for the presence of an initial tilde.   If one is  found, then
if   the  next  character  is   not  alphanumeric or  underscore (e.g.
~/foo.bar),  the  tilde  is  replaced  by ${HOME};     otherwise (e.g.
~user/foo.bar), the tilde is stripped and the following name is looked
up using getpwnam() to find the login directory of that user name, and
that  string is substituted  for the ~name.   Then,  the  filename  is
scanned for  $NAME  or ${NAME} sequences,  where NAME conforms  to the
regular expression  [A-Za-z_]+[A-Za-z0-9_]*, and environment  variable
substitution  is performed for  NAME.  Finally,  the   lookup is tried
again.  If it  is successful,  the  name[] string  is replaced by  the
substituted name, so it can be later used to open the file.

***********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "machdefs.h"

#define MAXPATHLEN		(4*MAXFNAME) /* allow space for long paths */

#ifdef VMS

/***********************************************************************
Under VAX VMS 3.x  and  4.x, exit(n) and return(n)  treat n as a VAX VMS
status code, where odd n means success, and even n means failure.  Under
VMS 5.x, return(n)  still acts this way, but  exit(n) means success if n
== 0,  and failure  if n !=  0,  bringing it into  agreement  with other
exit() implementations.  Prior to VMS 5.x, EXIT_FAILURE and EXIT_SUCCESS
were not defined in stdlib.h; they  are in VMS 5.x.   The code here will
therefore leave the new definitions unchanged,  and supply them on older
VMS systems.
***********************************************************************/

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	((1 << 28) + 2) /* (suppresses %NONAME-E-NOMSG) */
#endif /* EXIT_FAILURE */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	(1)
#endif /* EXIT_SUCCESS */

#else  /* NOT VMS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	(1)
#endif /* EXIT_FAILURE */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	(0)
#endif /* EXIT_SUCCESS */

#endif /* VMS */


#if    OS_PCDOS
#include <io.h>				/* for more function declarations */
#endif /* OS_PCDOS */

#if    defined(__STDC__) || defined(_AIX)
#define ARGS(plist)	plist
#else
#define ARGS(plist)	()
#endif

#if    OS_UNIX
#include <pwd.h>			/* and ~name lookup in envsub() */
#else
int		access ARGS((const char *file_spec, int mode));
#endif /* OS_UNIX */

#ifdef DVI
#include "typedefs.h"
extern UNSIGN16		debug_code;
extern BOOLEAN		g_dolog;	/* allow log file creation */
extern FILE		*g_logfp;	/* log file pointer (for errors) */
extern FILE		*stddbg;	/* debug output file pointer */
extern char		g_logname[];	 /* name of log file, if created */
#define  DBGOPT(flag)	(debug_code & (flag))
#define  DBG_OPEN_OKAY	0x0008
#define  DBG_OPEN_FAIL	0x0010
static void	log_lookup ARGS((FILE *fp, const char *filename,
				const char *status));
#if    (OS_ATARI | OS_PCDOS | OS_RMX | OS_TOPS20)
#define NEWLINE(fp) {(void)putc((char)'\r',fp);(void)putc((char)'\n',fp);}
					/* want <CR><LF> for these systems */
#else  /* NOT (OS_ATARI | OS_PCDOS | OS_RMX | OS_TOPS20) */
#define NEWLINE(fp) (void)putc((char)'\n',fp)	/* want bare <LF> */
#endif /* (OS_ATARI | OS_PCDOS | OS_RMX | OS_TOPS20) */
#endif /* DVI */

#define FILE_EXISTS(n) (access((char*)n,0) == 0)

#define ISNAMEPREFIX(c) (isalpha(c) || ((c) == '_'))
#define ISNAMESUFFIX(c) (isalnum(c) || ((c) == '_'))

#ifdef MIN
#undef MIN
#endif /* MIN */

#define	MIN(a,b)	((a) < (b) ? (a) : (b))

#if    OS_VAXVMS
#define GETENV (char*)vms_getenv
#endif /* OS_VAXVMS */

/* VMS 4.4 string.h incorrectly declares strspn as char* instead of
size_t, but the return value is (correctly) an integer.  VMS 5.5 string.h
declares it correctly. */
#define STRSPN(s,t) (size_t)strspn(s,t)

static const char	*copyname ARGS((char *target, const char *source));
static char		*envsub ARGS((const char *filename));
char			*findfile ARGS((const char *pathlist,
				const char *name));
static int		isfile ARGS((char *filename));

#ifdef TEST
int		main ARGS((void));
#endif /* TEST */

#if    OS_ATARI
#define DIRSEP "\\"
#define SEPARATORS " ;,|"
#endif /* OS_ATARI */

#if    OS_PCDOS
#define DIRSEP "\\"
#define SEPARATORS " ;,|"
#endif /* OS_PCDOS */

#if    OS_PRIMOS
#define SEPARATORS " :"
#define DIRSEP ">"
#endif /* OS_PRIMOS */

#if    OS_RMX
#define DIRSEP "/"
#define SEPARATORS " ;,|"
#endif /* OS_RMX */

#if    OS_TOPS20
#define DIRSEP ":>"			/* first char is what we default to */
#define SEPARATORS " ;,|"
#endif /* OS_TOPS20 */

#if    OS_UNIX
#define DIRSEP "/"
#define SEPARATORS " ;:,|"
#endif /* OS_UNIX */

#if    OS_VAXVMS
#define DIRSEP ":]"			/* first char is what we default to */
#define SEPARATORS " ;,|"
#endif /* OS_VAXVMS */

/***********************************************************************
Copy environment variable or usename, and return updated pointer past
end of copied name in source[].
***********************************************************************/

static const char*
copyname(target,source)
register char			*target; /* destination string */
register const char		*source; /* source string */
{
    if (ISNAMEPREFIX(*source))
    {
	for (*target++ = *source++; ISNAMESUFFIX(*source); )
	    *target++ = *source++;	/* copy name */
	*target = '\0';			/* terminate name */
    }
    return (source);
}


/***********************************************************************
Do  tilde and environment  variable  in  a private copy   of filename,
return  (char*)NULL if no  changes were made,  and otherwise, return a
pointer to the internal copy of the expanded filename.
***********************************************************************/

static char*
envsub(filename)
const char			*filename;
{

#if    (OS_ATARI | OS_PCDOS | OS_UNIX)
    static char			altname[MAXPATHLEN+1]; /* result storage */
    register const char		*p;
    register char		*r;
    register const char		*s;
    char			tmpfname[MAXPATHLEN+1];

#if    OS_UNIX
    struct passwd		*pw;	/* handle tilde processing */

    tmpfname[0] = '\0';			/* initialize tmpfname[] */
    p = strchr(filename,'~');
    if (p != (char*)NULL)		/* handle tilde (once per filename) */
    {
	(void)strncpy(tmpfname,filename,(size_t)(p - filename));
	tmpfname[(int)(p - filename)] = '\0'; /* terminate copied string */
	r = strchr(tmpfname,'\0');	/* remember start of name */
	++p;				/* point past tilde */
	if (ISNAMEPREFIX(*p))		/* expect ~name */
	{
	    p = copyname(r,p);		/* p now points past ~name */
	    pw = getpwnam(r);		/* get password structure */
	    (void)strcpy(r,pw->pw_dir);	/* replace name by login directory */
	}
	else				/* expect ~/name */
	    (void)strcat(tmpfname,"${HOME}"); /* change to ${HOME}/name */
	(void)strcat(tmpfname,p);	/* copy rest of filename */
    }
    else
	(void)strcpy(tmpfname,filename);	/* copy of filename */
#else /* NOT OS_UNIX */
    (void)strcpy(tmpfname,filename);	/* copy of filename */
#endif /* OS_UNIX */

    for (r = altname, *r = '\0', p = tmpfname; *p; )
    {					/* handle environment variables */
	if (*p == '$')			/* expect $NAME or ${NAME} */
	{
	    p++;			/* point past $ */
	    if (*p == '{')		/* have ${NAME} */
	    {
		p = copyname(r,p+1);
		if (*p++ != '}')
		    return ((char*)NULL); /* bail out -- no closing brace */
		for (s = (const char *)getenv(r); (s != (char*)NULL) && *s;)
		    *r++ = *s++;	/* copy environment variable value */
		*r = '\0';		/* terminate altname[] */
	    }
	    else if (ISNAMEPREFIX(*p))	/* have $NAME */
	    {
		p = copyname(r,p);
		for (s = (const char *)getenv(r); (s != (char*)NULL) && *s;)
		    *r++ = *s++;	/* copy environment variable value */
		*r = '\0';		/* terminate altname[] */
	    }
	    else			/* invalid $NAME */
		*r++ = '$';		/* so just copy bare $ */
	}
	else				/* just copy character */
	    *r++ = *p++;
    }
    *r = '\0';				/* terminate altname[] */

#ifdef TEST
    printf("envsub: [");
    for (p = filename; *p; ++p)
    {
	putchar(*p);
	if (strchr(SEPARATORS,*p) != (char*)NULL)
	    printf("\n\t");
    }
    printf("] ->\n\t[");
    for (p = altname; *p; ++p)
    {
	putchar(*p);
	if (strchr(SEPARATORS,*p) != (char*)NULL)
	    printf("\n\t");
    }
    printf("]\n");
#endif /* TEST */

    return (altname[0] ? altname : (char*)NULL);
#else  /* NOT (OS_ATARI | OS_PCDOS | OS_UNIX) */
    return ((char*)NULL);		/* no processing to be done */
#endif /* (OS_ATARI | OS_PCDOS | OS_UNIX) */

}


/***********************************************************************
Given a directory search path in pathlist[] and a file name in name[],
search the path  for an  existing file.  If  one  is  found, return  a
pointer  to  an  internal copy   of its  full  name; otherwise, return
(char*)NULL.
***********************************************************************/

char*
findfile(pathlist,name)
const char  *pathlist;
const char  *name;
{
    static char			fullname[MAXPATHLEN+1];
    static char			fullpath[MAXPATHLEN+1];
    int				k;
    int				len;
    register const char		*p;

    /*******************************************************************
      For user convenience, allow path lists to contain nested
      references to environment variables.  We repeatedly expand the
      path list until there are no more changes in it, or a reasonable
      limit is reached (to prevent infinite recursion when a path
      mistakenly contains itself).
    *******************************************************************/

    p = pathlist;
    if (pathlist != (char*)NULL)
    {
	(void)strcpy(fullpath,pathlist);
	for (k = 0; k < 10; ++k)
	{
	    p = envsub(fullpath);
	    if ( (p == (char*)NULL) || (strcmp(p,fullpath) == 0) )
		break;			/* no new substitutions were made */
	    else			/* save new expansion */
		(void)strcpy(fullpath,p);
	}
	p = fullpath;			/* remember last expansion */
    }

    if ((p == (char*)NULL) || (*p == '\0')) /* no path, try bare filename */
    {
	(void)strncpy(fullname,name,MAXPATHLEN);
	fullname[MAXPATHLEN] = '\0';	/* in case strncpy() doesn't do it */
	return (isfile(fullname) ? (char*)&fullname[0] : (char*)NULL);
    }
    while (*p)				/* process pathlist */
    {
	len = strcspn(p,SEPARATORS);	/* get length before separator */
	len = MIN(MAXPATHLEN-1,len);	/* leave space for possible DIRSEP */
	(void)strncpy(fullname,p,len);	/* set directory name */
	k = len;
	if (strchr(DIRSEP,fullname[k-1]) == (char*)NULL)
	    fullname[k++] = DIRSEP[0];	/* supply directory separator */
	(void)strncpy(&fullname[k],name,MAXPATHLEN-k); /* append name */
	fullname[MAXPATHLEN] = '\0';	/* strncpy may not supply this */
	if (isfile(fullname))
	    return ((char*)&fullname[0]);

#if    OS_VAXVMS
	do				/* single trip loop */
	{
	    char		*logname;
	    int			n;

	    if (fullname[k-1] == ']')	/* then not logical name */
		break;			/* here's a loop exit */
	    fullname[k] = '\0';		/* terminate logical name */
	    logname = GETENV(fullname);
	    if (logname == (char*)NULL)	/* then unknown logical name */
		break;			/* here's a loop exit */
	    (void)strcpy(fullname,logname);
	    n = strlen(fullname);
	    if ( (n >= 2) && (fullname[n-2] == '.') && (fullname[n-1] == ']') )
	    {				/* have rooted name [dir.] */
		if (name[0] == '[')	/* have [dir.][sub]name */
		    (void)strcpy(&fullname[n-1],&name[1]); /* [dir.sub]name */
		else			/* have [dir.]name */
		{
		    fullname[n-2] = ']';
		    (void)strcpy(&fullname[n-1],&name[0]); /* [dir]name */
		}
	    }
	    else if (fullname[n-1] == ']')
	    {				/* have normal name [dir] */
		if (name[0] == '[')	/* have [dir][sub]name */
		{
		    fullname[n-1] = '.';
		    (void)strcpy(&fullname[n],&name[1]); /* [dir.sub]name */
		}
		else			/* have [dir]name */
		    (void)strcpy(&fullname[n],&name[0]); /* [dir]name */
	    }
	    else			/* must be logical name component */
	    {
		if (fullname[n-1] != ':')
		    fullname[n++] = ':';
		(void)strcpy(&fullname[n],&name[0]); /* logname:name */
	    }
	    if (isfile(fullname))
		return ((char*)&fullname[0]);
	}
	while (0);
#endif /* OS_VAXVMS */

	p += len;			/* point past first path */
	if (*p)				/* then not at end of path list */
	{
	    p++;			/* point past separator */
	    p += STRSPN(p,SEPARATORS);	/* skip over any extra separators */
	}
    }
    return ((char*)NULL);		/* no file found */
}


/***********************************************************************
Return  a non-zero   result if a  file  named   filename  exists,  and
otherwise, return zero.  If the first existence check  fails,  then we
try tilde and  environment  variable substitutions, and  if there were
any, a second existence check  is  made.  If this second one succeeds,
we  replace filename with the  substituted  name,  since  that will be
needed later to open the file.
***********************************************************************/

static int
isfile(filename)
char				*filename;
{
    int				exists;
    char			*p;

    exists = FILE_EXISTS(filename);

    if (!exists)		/* try environment variable substitution */
    {
	p = envsub(filename);
	if (p != (char*)NULL)
	{
	    exists = FILE_EXISTS(p);
	    if (exists)
		(void)strcpy(filename,p);
	}
    }

#ifdef DVI
    {
	FILE	*debug_fp = (stddbg == (FILE*)NULL) ? (FILE*)stderr : stddbg;

	if (DBGOPT(DBG_OPEN_OKAY) && exists)
	{
	    log_lookup(debug_fp,filename,"OK    ");
	    if (g_dolog && (g_logfp != (FILE *)NULL) && g_logname[0])
		log_lookup(g_logfp,filename,"OK    ");
	}
	else if (DBGOPT(DBG_OPEN_FAIL) && !exists)
	{
	    log_lookup(debug_fp,filename,"FAILED");
	    if (g_dolog && (g_logfp != (FILE *)NULL) && g_logname[0])
		log_lookup(g_logfp,filename,"FAILED");
	}
    }
#endif /* DVI */

    return (exists);
}

#ifdef DVI
static void
log_lookup(fp,filename,status)
FILE		*fp;
const char	*filename;
const char	*status;
{
    (void)fprintf(fp,"%%Lookup %s [%s]",status,filename);
    NEWLINE(fp);
    (void)fflush(fp);
}
#endif /* DVI */

#ifdef TEST

#define  MAXSTR		257	/* DVI file text string size */

UNSIGN16 debug_code;		/* 0 for no debug output */
BOOLEAN g_dolog = TRUE;		/* allow log file creation */
FILE *g_logfp = (FILE*)NULL;	/* log file pointer (for errors) */
char g_logname[MAXSTR];		/* name of log file, if created */
FILE *stddbg = stderr;

/***********************************************************************
Simple test program for findfile().  It prompts for a single directory
search path, then  loops reading test filenames,  uses findfile to see
if they exist in the search path, and  prints the name of the matching
file,  if  any.  If tilde  or environment  variable  substitutions are
made, they are printed as well.
***********************************************************************/

int
main()
{
    char			pathlist[MAXPATHLEN+1];
    char			name[MAXPATHLEN+1];
    char			*p;

    (void)printf("Enter file search path: ");
    (void)fgets(pathlist,MAXPATHLEN,stdin);

    p = strchr(pathlist,'\n');
    if (p != (char*)NULL)
	*p = '\0';			/* kill terminal newline */

    for (;;)
    {
	(void)printf("Enter file name: ");
	if (fgets(name,MAXPATHLEN,stdin) == (char*)NULL)
	    break;			/* here's the loop exit */
	p = strchr(name,'\n');
	if (p != (char*)NULL)
	    *p = '\0';			/* kill terminal newline */
	p = findfile(pathlist,name);
	if (p == (char*)NULL)
	    (void)printf("\tNo such file\n");
	else
	    (void)printf("\tFile is [%s]\n",p);
    }
    exit(EXIT_SUCCESS);
    return (0);
}
#endif /* TEST */
