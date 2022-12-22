#include "libioP.h"
#include "stdio.h"

int
ferror(fp)
     FILE* fp;
{
  COERCE_FILE(fp);
  return _IO_ferror(fp);
}
