#include "libioP.h"
#include "stdio.h"

void
clearerr(fp)
     FILE* fp;
{
  COERCE_FILE(fp);
  _IO_clearerr(fp);
}
