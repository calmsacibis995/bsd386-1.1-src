#include "tm-vax.h"

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dvax -Dunix -Dultrix -Dbsd4_2 -D__vax -D__unix -D__ultrix -D__bsd4_2"

/* By default, allow $ to be part of an identifier.  */
#define DOLLARS_IN_IDENTIFIERS 1
