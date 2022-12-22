/* bashansi.h -- Typically included information required by picky compilers. */

#if !defined (_BASHANSI_H_)
#define _BASHANSI_H_

#if defined (HAVE_STRING_H)
#  include <string.h>
#else
#  include <strings.h>
#endif /* !HAVE_STRING_H */

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#endif /* !_BASHANSI_H_ */
