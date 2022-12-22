#include "libioP.h"
#include "stdio.h"

int
fileno(fp)
     _IO_FILE* fp;
{
  COERCE_FILE(fp);
  if (!(fp->_flags & _IO_IS_FILEBUF))
    return EOF;
  return _IO_fileno(fp);
}
