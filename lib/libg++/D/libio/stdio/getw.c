#include "libioP.h"
#include "stdio.h"

int
getw(fp)
     FILE *fp;
{
  int w;
  _IO_size_t bytes_read;
  COERCE_FILE(fp);
  bytes_read = _IO_sgetn (fp, (char*)&w, sizeof(w));
  return sizeof(w) == bytes_read ? w : EOF;
}
