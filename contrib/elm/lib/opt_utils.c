
static char rcsid[] = "@(#)Id: opt_utils.c,v 5.8 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: opt_utils.c,v $
 * Revision 1.2  1994/01/14  00:53:36  cp
 * 2.4.23
 *
 * Revision 5.8  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.7  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.6  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.5  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.4  1992/12/12  01:29:26  syd
 * Fix double inclusion of sys/types.h
 * From: Tom Moore <tmoore@wnas.DaytonOH.NCR.COM>
 *
 * Revision 5.3  1992/10/30  21:49:38  syd
 * Add init of buf
 * From: Syd via request from hessmann@unipas.fmi.uni-passau.de (Georg Hessmann)
 *
 * Revision 5.2  1992/10/24  13:06:23  syd
 * make getdomainname optional
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This file contains routines that might be needed for the various
     machines that the mailer can run on.  Please check the Makefile
     for more help and/or information. 

**/

#include "headers.h"
#include "s_error.h"

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#ifndef GETHOSTNAME
# ifdef DOUNAME
#  include <sys/utsname.h>
# endif
#endif

#ifndef GETHOSTNAME

gethostname(cur_hostname,size) /* get name of current host */
char *cur_hostname;
int size;
{
	/** Return the name of the current host machine. **/

#if (defined(XENIX) || defined(M_UNIX)) & !defined(DOUNAME)
	char    buf[32];
	FILE    *fp;
	char    *p;

	buf[0] = '\0';

	if ((fp = fopen("/etc/systemid", "r")) != 0) {
	  fgets(buf, sizeof(buf) - 1, fp);
	  fclose(fp);
	  if ((p = index(buf, '\n')) != NULL)
	    *p = '\0';
	  (void) strfcpy(cur_hostname, buf);
	  return 0;
	}

#else   /* neither XENIX nor SCO UNIX */

#ifdef DOUNAME
	/** This routine compliments of Scott McGregor at the HP
	    Corporate Computing Center **/
     
	int uname();
	struct utsname name;

	(void) uname(&name);
	(void) strfcpy(cur_hostname, name.nodename, size);
#else
	(void) strfcpy(cur_hostname, HOSTNAME, size-1);
#endif	/* DOUNAME */

	return 0;

#endif  /* XENIX or SCO UNIX */
}

#endif  /* GETHOSTNAME */


gethostdomain(hostdom, size)    /* get domain of current host */
char *hostdom;
int size;
{
	char    buf[64];
	FILE    *fp;
	char    *p;

	if (size < 2)
	  return -1;

	buf[0] = '\0';

	if ((fp = fopen(hostdomfile, "r")) != 0) {
	  fgets(buf, sizeof(buf) - 1, fp);
	  fclose(fp);
	  if ((p = index(buf, '\n')) != NULL)
	    *p = '\0';
	}
	else {
#ifdef USEGETDOMAINNAME
	  if (getdomainname(buf, sizeof(buf)) != 0)
#endif
	    strfcpy(buf, DEFAULT_DOMAIN, sizeof(buf));
	}
	if (buf[0] != '\0' && buf[0] != '.') {
	  *hostdom++ = '.';
	  --size;
	}
	(void) strfcpy(hostdom, buf, size);

	return 0;
}


#ifndef HAS_CUSERID

char *cuserid(uname)
     char *uname;
{
	/** Added for compatibility with Bell systems, this is the last-ditch
	    attempt to get the users login name, after getlogin() fails.  It
	    instantiates "uname" to the name of the user...(it also tries
	    to use "getlogin" again, just for luck)
	**/
	/** This wasn't really compatible.  According to our man page, 
	 ** It was inconsistent.  If the parameter is NULL then you return
	 ** the name in a static area.  Else the ptr is supposed to be a
	 ** pointer to l_cuserid bytes of memory [probally 9 bytes]...
	 ** It's not mention what it should return if you copy the name
	 ** into the array, so I chose NULL.
	 ** 					Sept 20, 1988
	 **					**WJL**
	 **/

  struct passwd *password_entry;
#ifndef _POSIX_SOURCE
  struct passwd *getpwuid();
#endif
  char   *name, *getlogin();
  static char buf[10];
  register returnonly = 0;
  
  if (uname == NULL) ++returnonly;
  
  if ((name = getlogin()) != NULL) {
    if (returnonly) {
      return(name);
    } else {
      strcpy(uname, name);
      return name;
    }
  } 
  else 
    if (( password_entry = getpwuid(getuid())) != NULL) 
      {
	if (returnonly) 
	  {
	    return(password_entry->pw_name);
	  }
	else 
	  {
	    strcpy(uname, password_entry->pw_name);
	    return name;
	  }
      } 
    else 
      {
	return NULL;
      }
}

#endif


#ifndef STRTOK

char *strtok(source, keys)
char *source, *keys;
{
	/** This function returns a pointer to the next word in source
	    with the string considered broken up at the characters 
	    contained in 'keys'.  Source should be a character pointer
	    when this routine is first called, then NULL subsequently.
	    When strtok has exhausted the source string, it will 
	    return NULL as the next word. 

	    WARNING: This routine will DESTROY the string pointed to
	    by 'source' when first invoked.  If you want to keep the
	    string, make a copy before using this routine!!
	 **/

	register int  last_ch;
	static   char *sourceptr;
		 char *return_value;

	if (source != NULL)
	  sourceptr = source;
	
	if (*sourceptr == '\0') 
	  return(NULL);		/* we hit end-of-string last time!? */

	sourceptr += strspn(sourceptr, keys);	/* skip leading crap */
	
	if (*sourceptr == '\0') 
	  return(NULL);		/* we've hit end-of-string */

	last_ch = strcspn(sourceptr, keys);	/* end of good stuff */

	return_value = sourceptr;		/* and get the ret   */

	sourceptr += last_ch;			/* ...value 	     */

	if (*sourceptr != '\0')		/* don't forget if we're at END! */
	  sourceptr++;			   /* and skipping for next time */

	return_value[last_ch] = '\0';		/* ..ending right    */
	
	return((char *) return_value);		/* and we're outta here! */
}

#endif


#ifndef STRPBRK

char *strpbrk(source, keys)
char *source, *keys;
{
	/** Returns a pointer to the first character of source that is any
	    of the specified keys, or NULL if none of the keys are present
	    in the source string. 
	**/

	register char *s, *k;

	for (s = source; *s != '\0'; s++) {
	  for (k = keys; *k; k++)
	    if (*k == *s)
	      return(s);
	}
	
	return(NULL);
}

#endif

#ifndef STRSPN

strspn(source, keys)
char *source, *keys;
{
	/** This function returns the length of the substring of
	    'source' (starting at zero) that consists ENTIRELY of
	    characters from 'keys'.  This is used to skip over a
	    defined set of characters with parsing, usually. 
	**/

	register int loc = 0, key_index = 0;

	while (source[loc] != '\0') {
	  key_index = 0;
	  while (keys[key_index] != source[loc])
	    if (keys[key_index++] == '\0')
	      return(loc);
	  loc++;
	}

	return(loc);
}

#endif

#ifndef STRCSPN

strcspn(source, keys)
char *source, *keys;
{
	/** This function returns the length of the substring of
	    'source' (starting at zero) that consists entirely of
	    characters NOT from 'keys'.  This is used to skip to a
	    defined set of characters with parsing, usually. 
	    NOTE that this is the opposite of strspn() above
	**/

	register int loc = 0, key_index = 0;

	while (source[loc] != '\0') {
	  key_index = 0;
	  while (keys[key_index] != '\0')
	    if (keys[key_index++] == source[loc])
	      return(loc);
	  loc++;
	}

	return(loc);
}

#endif

#ifndef TEMPNAM
/* and a tempnam for temporary files */
static int cnt = 0;

char *tempnam( dir, pfx)
 char *dir, *pfx;
{
	char space[SLEN];
	char *newspace;

	if (dir == NULL) {
		dir = "/usr/tmp";
	} else if (*dir == '\0') {
		dir = "/usr/tmp";
	}
	
	if (pfx == NULL) {
		pfx = "";
	}

	sprintf(space, "%s%s%d.%d", dir, pfx, getpid(), cnt);
	cnt++;
	
	newspace = malloc(strlen(space) + 1);
	if (newspace != NULL) {
		strcpy(newspace, space);
	}
	return newspace;
}

#endif

#ifndef GETOPT

/*LINTLIBRARY*/
#define NULL	0
#define EOF	(-1)
#define ERR(s, c)	if(opterr){\
	extern int strlen(), write();\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}

extern int strcmp();

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int
getopt(argc, argv, opts)
int	argc;
char	**argv, *opts;
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == NULL) {
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=index(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			cp = catgets(elm_msg_cat, ErrorSet, ErrorGetoptReq,
				": option requires an argument -- ");
			ERR(cp, c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

#endif

#ifndef RENAME
int rename(tmpfname, fname)
#ifdef ANSI_C
const
#endif
   char *tmpfname, *fname;
{
	int status;

	(void) unlink(fname);
	if ((status = link(tmpfname, fname)) != 0)
		return(status);
	(void) unlink(tmpfname);
	return(0);
}
#endif
