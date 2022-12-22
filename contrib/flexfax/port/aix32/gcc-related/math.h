extern "C" {
#include </usr/include/standards.h>

#ifdef _ANSI_C_SOURCE
extern  double fmod(double x, double y);
#endif /*_ANSI_C_SOURCE */

#ifndef	_BSD
#include <stdlib.h>
#else	/* _BSD */
extern double   atof(const char *nptr);
#endif	/* _BSD */
}
