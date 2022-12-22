#include "libioP.h"
#if _G_HAVE_ATEXIT
#include <stdlib.h>

typedef void (*voidfunc) _PARAMS((void));

static void _IO_register_cleanup ()
{
  atexit ((voidfunc)_IO_flush_all);
  _IO_cleanup_registration_needed = 0;
}

void (*_IO_cleanup_registration_needed)() = _IO_register_cleanup;
#endif /* _G_HAVE_ATEXIT */
