/* This is part of libio/iostream, providing -*- C++ -*- input/output.
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
#include "iostream.h"
#include <string.h>

istream& istream::get(streambuf& sb, char delim /* = '\n' */)
{
  _gcount = 0;
  if (ipfx1())
    {
      register streambuf* isb = rdbuf();
      for (;;)
	{
	  streamsize len = isb->_IO_read_end - isb->_IO_read_ptr;
	  if (len <= 0)
	    if (__underflow(isb) == EOF)
	      break;
	    else
	      len = isb->_IO_read_end - isb->_IO_read_ptr;
	  char *delimp = (char*)memchr((void*)isb->_IO_read_ptr, delim, len);
	  if (delimp != NULL)
	    len = delimp - isb->_IO_read_ptr;
	  int written = sb.sputn(isb->_IO_read_ptr, len);
	  isb->_IO_read_ptr += written;
	  _gcount += written;
	  if (written != len)
	    {
	      set(ios::failbit);
	      break;
	    }
	  if (delimp != NULL)
	    break;
	}
    }
  return *this;
}
