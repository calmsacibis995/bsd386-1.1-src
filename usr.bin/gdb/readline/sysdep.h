/* System-dependent stuff, for ``normal'' systems */

#ifdef __GNUC__
#define alloca __builtin_alloca
#else
#if defined (sparc) && defined (sun)
#include <alloca.h>
#endif
extern char *alloca ();
#endif

#include <sys/types.h>			/* Needed by dirent.h */

#include <dirent.h>
typedef struct dirent dirent;

#define	HAVE_UNISTD_H			1
