
#ifdef __GNUC__
#include_next <sys/select.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC__
#include "/usr/include/sys/select.h"
#endif

	struct timeval;
	extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef __cplusplus
}
#endif
