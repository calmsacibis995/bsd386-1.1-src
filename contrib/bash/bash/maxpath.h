/* maxpath.h - Find out what this system thinks MAXPATHLEN is. */

#if !defined (_MAXPATH_H)
#define _MAXPATH_H

#if !defined (MAXPATHLEN) && defined (HAVE_LIMITS_H)
#  if !defined (BUILDING_MAKEFILE)
#    include <limits.h>
#  endif /* BUILDING_MAKEFILE */
#endif /* !MAXPATHLEN && HAVE_LIMITS_H */

#if !defined (MAXPATHLEN)
#  if defined (bsdi) || defined (OSF1) || defined (Solaris) || defined (Dynix)
#    include <sys/param.h>
#  endif /* bsdi || OSF1 || Solaris || Dynix */
#endif /* !MAXPATHLEN */

#if !defined (MAXPATHLEN) && defined (PATH_MAX)
#  define MAXPATHLEN PATH_MAX
#endif /* !MAXPATHLEN && PATH_MAX */

/* Yecch!  Who cares about this gross concept in the first place? */
#if !defined (MAXPATHLEN)
#  define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

#endif /* _MAXPATH_H */
