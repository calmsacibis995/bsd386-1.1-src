/* c-auto.h.  Generated automatically by configure.  */
/* c-auto.h.in: template for c-auto.h.  */

/* Define if your system has <dirent.h>.  */
#define DIRENT 1

/* Define if your system has <sys/ndir.h>.  */
#undef SYSNDIR

/* Define if the directory library header declares closedir as void.  */
#undef VOID_CLOSEDIR

/* Define if your system lacks <limits.h>.  */
#undef LIMITS_H_MISSING

/* Likewise, for <float.h>.  */
#undef FLOAT_H_MISSING

/* Define if <string.h> does not declare memcmp etc., and <memory.h> does.  */
#undef NEED_MEMORY_H

/* Define if your system has ANSI C header files: <stdlib.h>, etc.  */
#define STDC_HEADERS 1

/* Define if your system has <unistd.h>.  */
#define HAVE_UNISTD_H 1

/* Define if your system lacks rindex, bzero, etc.  */
#undef USG

/* Define if `int' is smaller than `long'.  */
#undef INT_16_BITS
#ifdef INT_16_BITS
#define SHORTINT
#endif
