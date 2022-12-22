#include "libioP.h"
#include "stdio.h"

int
fgetc(fp)
     FILE *fp;
{
  COERCE_FILE(fp);
  return _IO_getc(fp);
}
