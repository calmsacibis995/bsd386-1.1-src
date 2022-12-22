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

/* Written by Per Bothner (bothner@cygnus.com). */

#ifndef _STDIOSTREAM_H
#define _STDIOSTREAM_H

#ifdef __GNUG__
#pragma interface
#endif

#include <streambuf.h>
#include <stdio.h>

class stdiobuf : public filebuf {
  protected:
    FILE *_file;
  public:
    FILE* stdiofile() const { return _file; }
    stdiobuf(FILE *f);
    virtual _IO_ssize_t sys_read(char* buf, _IO_size_t size);
    virtual _IO_fpos_t sys_seek(_IO_fpos_t, _seek_dir);
    virtual _IO_ssize_t sys_write(const void*, _IO_size_t);
    virtual int sys_close();
    virtual int sync();
    virtual int overflow(int c = EOF);
    virtual int xsputn(const char* s, int n);
};

#endif /* !_STDIOSTREAM_H */
