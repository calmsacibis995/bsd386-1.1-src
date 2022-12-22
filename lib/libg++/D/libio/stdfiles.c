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


/* check following! */
#define FILEBUF_LITERAL(CHAIN, FLAGS, FD) \
       { _IO_MAGIC+_IO_LINKED+_IO_IS_FILEBUF+FLAGS, \
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CHAIN, &_IO_file_jumps, FD}

#define DEF_STDFILE(NAME, FD, CHAIN, FLAGS) \
  struct _IO_FILE_plus NAME = {FILEBUF_LITERAL(CHAIN, FLAGS, FD), 0}

DEF_STDFILE(_IO_stdin_, 0, 0, _IO_NO_WRITES);
DEF_STDFILE(_IO_stdout_, 1, &_IO_stdin_._file, _IO_NO_READS);
DEF_STDFILE(_IO_stderr_, 2, &_IO_stdout_._file,
            _IO_NO_READS+_IO_UNBUFFERED);

_IO_FILE *_IO_list_all = &_IO_stderr_._file;
