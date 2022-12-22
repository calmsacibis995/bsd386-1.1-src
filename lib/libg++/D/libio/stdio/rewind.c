#include "stdio.h"
#include "libioP.h"

void
rewind(fp)
     _IO_FILE* fp;
{
  COERCE_FILE(fp);
  _IO_rewind(fp);
}
