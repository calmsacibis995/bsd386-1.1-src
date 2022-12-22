#include "libioP.h"
#include "stdio.h"

#undef putw

int
putw(w, fp)
     int w;
     FILE *fp;
{
  _IO_size_t written;
  COERCE_FILE(fp);
  written = _IO_sputn(fp, (const char *)&w, sizeof(w));
  return written == sizeof(w) ? 0 : EOF;
}
