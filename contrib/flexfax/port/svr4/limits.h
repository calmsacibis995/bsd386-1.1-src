#ifdef __cplusplus
extern "C" {
#endif
#ifdef __GNUC__
#include_next <limits.h>
#else
#include "/usr/include/limits.h"
#endif
#ifdef __cplusplus
}
#endif

#ifndef _POSIX_OPEN_MAX
# include <ulimit.h>
# define _POSIX_OPEN_MAX ulimit(UL_GDESLIM, 0)
#endif

