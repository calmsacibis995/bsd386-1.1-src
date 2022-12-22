#include_next <ctype.h>

#ifdef toupper
#undef toupper
int toupper __P((int));
#endif

#ifdef tolower
#undef tolower
int tolower __P((int));
#endif
