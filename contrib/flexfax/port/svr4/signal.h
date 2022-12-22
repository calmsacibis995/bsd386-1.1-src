
#ifdef __GNUC__
#include_next <signal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC__
#include "/usr/include/signal.h"
#endif

	extern void (*sigset(int, void (*)(int)))(int);

#ifdef __cplusplus
}
#endif

#define signal sigset

