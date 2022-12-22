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

_IO_FILE *
_IO_fdopen (fd, mode)
     int fd;
     const char *mode;
{
  int read_write;
  struct _IO_FILE_plus *fp;

  switch (*mode++)
    {
    case 'r':
      read_write = _IO_NO_WRITES;
      break;
    case 'w':
      read_write = _IO_NO_READS;
      break;
    case 'a':
      read_write = _IO_NO_READS|_IO_IS_APPENDING;
      break;
    default:
#ifdef EINVAL
      errno = EINVAL;
#endif
      return NULL;
  }
  if (mode[0] == '+' || (mode[0] == 'b' && mode[1] == '+'))
    read_write &= _IO_IS_APPENDING;

  fp = (struct _IO_FILE_plus*)malloc(sizeof(struct _IO_FILE_plus));
  if (fp == NULL)
    return NULL;
  _IO_init(&fp->_file, 0);
  fp->_file._jumps = &_IO_file_jumps;
  _IO_file_init(&fp->_file);
  fp->_vtable = NULL;
  if (_IO_file_attach(&fp->_file, fd) == NULL)
    {
      _IO_un_link(&fp->_file);
      free (fp);
      return NULL;
    }
  fp->_file._flags &= ~_IO_DELETE_DONT_CLOSE;

  fp->_file._IO_file_flags = 
    _IO_mask_flags(&fp->_file, read_write,
		   _IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);

  return (_IO_FILE*)fp;
}
