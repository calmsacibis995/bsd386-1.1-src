/* 
Copyright (C) 1993 Free Software Foundation

This file is part of the GNU IO Library.  This library is free
software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option)
any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

As a special exception, if you link this library with files
compiled with a GNU compiler to produce an executable, this does not cause
the resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why
the executable file might be covered by the GNU General Public License. */

#include "libioP.h"

#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

int
_IO_setvbuf(fp, buf, mode, size)
     _IO_FILE* fp;
     char* buf;
     int mode;
     _IO_size_t size;
{
  COERCE_FILE(fp);
  switch (mode)
    {
    case _IOFBF:
      fp->_IO_file_flags &= ~_IO_LINE_BUF;
      if (buf == NULL)
	return 0;
      return fp->_jumps->__setbuf(fp, buf, size);
    case _IOLBF:
      fp->_IO_file_flags |= _IO_LINE_BUF;
      if (buf == NULL)
	return 0;
      return fp->_jumps->__setbuf(fp, buf, size);
    case _IONBF:
      return fp->_jumps->__setbuf(fp, NULL, 0);
    default:
      return EOF;
    }
}
