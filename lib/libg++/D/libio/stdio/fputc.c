#include "libioP.h"
#include "stdio.h"

int
fputc(c, fp)
     int c;
     FILE *fp;
{
  COERCE_FILE(fp);
  return _IO_putc(c, fp);
}
