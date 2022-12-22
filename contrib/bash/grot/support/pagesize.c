/*
 * getpagesize - print the system pagesize
 *
 * Chet Ramey
 * chet@ins.cwru.edu
 */

#include <stdio.h>

/*
 * I know these systems have getpagesize(2)
 */

#if defined (Bsd) || defined (Ultrix) || defined (sun)
#  define HAVE_GETPAGESIZE
#endif

#if !defined (HAVE_GETPAGESIZE)

#include <sys/param.h>

#if defined (EXEC_PAGESIZE)
#  define getpagesize() EXEC_PAGESIZE
#else
#  if defined (PAGESIZE)
#    define getpagesize() PAGESIZE
#  else
#    if defined (NBPG)
#      define getpagesize() NBPG * CLSIZE
#      ifndef CLSIZE
#        define CLSIZE 1
#      endif /* no CLSIZE */
#    else /* no NBPG */
#      define getpagesize() NBPC
#    endif /* no NBPG */
#  endif /* no PAGESIZE */
#endif /* no EXEC_PAGESIZE */

#endif /* not HAVE_GETPAGESIZE */

main()
{
#if defined (HAVE_GETPAGESIZE) || defined (getpagesize)
  printf ("%ld\n", getpagesize ());
#else
  puts ("1024");
#endif
}
