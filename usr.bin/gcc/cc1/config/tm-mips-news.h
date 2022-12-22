/* like pmax except BIG ENDIAN instead of LITTLE ENDIAN  */

#include "tm-mips.h"

#undef CPP_SPEC
				/* default RISC NEWS environment */
#define CPP_SPEC "-Dr3000 -Dnews7300 -DLANGUAGE_C -DMIPSEB -DSYSTYPE_BSD \
 -Dsony_news -Dsony -Dunix -Dmips -Dhost_mips -I/usr/include2.11"

#undef MACHINE_TYPE
#define MACHINE_TYPE "Sony NEWS (RISC NEWS)"
