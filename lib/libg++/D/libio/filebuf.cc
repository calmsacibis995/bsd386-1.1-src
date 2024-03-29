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
the executable file might be covered by the GNU General Public License.

Written by Per Bothner (bothner@cygnus.com). */

#include "iostreamP.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "builtinbuf.h"

#define CLOSED_FILEBUF_FLAGS \
  (_IO_IS_FILEBUF+_IO_NO_READS+_IO_NO_WRITES+_IO_TIED_PUT_GET)

void filebuf::init()
{
  _IO_file_init(this);
}

filebuf::filebuf() : streambuf(CLOSED_FILEBUF_FLAGS)
{
  _IO_file_init(this);
}

/* This is like "new filebuf()", but it uses the _IO_file_jump jumptable,
   for eficiency. */

filebuf* filebuf::__new()
{
  filebuf *fb = new filebuf;
  fb->_jumps = &_IO_file_jumps;
  fb->_vtable() = NULL;
  return fb;
}

filebuf::filebuf(int fd) : streambuf(CLOSED_FILEBUF_FLAGS)
{
  _IO_file_init(this);
  _IO_file_attach(this, fd);
}

filebuf::filebuf(int fd, char* p, int len) : streambuf(CLOSED_FILEBUF_FLAGS)
{
  _IO_file_init(this);
  _IO_file_attach(this, fd);
  setbuf(p, len);
}

filebuf::~filebuf()
{
    if (!(xflags() & _IO_DELETE_DONT_CLOSE))
	close();

    _un_link();
}

filebuf* filebuf::open(const char *filename, ios::openmode mode, int prot)
{
  if (_IO_file_is_open (this))
    return NULL;
  int posix_mode;
  int read_write;
  if (mode & ios::app)
    mode |= ios::out;
  if ((mode & (ios::in|ios::out)) == (ios::in|ios::out)) {
    posix_mode = O_RDWR;
    read_write = 0;
  }
  else if (mode & ios::out)
    posix_mode = O_WRONLY, read_write = _IO_NO_READS;
  else if (mode & (int)ios::in)
    posix_mode = O_RDONLY, read_write = _IO_NO_WRITES;
  else
    posix_mode = 0, read_write = _IO_NO_READS+_IO_NO_WRITES;
  if ((mode & (int)ios::trunc) || mode == (int)ios::out)
    posix_mode |= O_TRUNC;
  if (mode & ios::app)
    posix_mode |= O_APPEND, read_write |= _IO_IS_APPENDING;
  if (!(mode & (int)ios::nocreate) && mode != ios::in)
    posix_mode |= O_CREAT;
  if (mode & (int)ios::noreplace)
    posix_mode |= O_EXCL;
  int fd = ::open(filename, posix_mode, prot);
  if (fd < 0)
    return NULL;
  _fileno = fd;
  xsetflags(read_write, _IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);
  if (mode & (ios::ate|ios::app)) {
    if (sseekoff(0, ios::end) == EOF)
      return NULL;
  }
  _IO_link_in(this);
  return this;
}

filebuf* filebuf::open(const char *filename, const char *mode)
{
  return (filebuf*)_IO_file_fopen(this, filename, mode);
}

filebuf* filebuf::attach(int fd)
{
  return (filebuf*)_IO_file_attach(this, fd);
}

streambuf* filebuf::setbuf(char* p, int len)
{
  return (streambuf*)_IO_file_setbuf(this, p, len);
}

int filebuf::doallocate() { return _IO_file_doallocate(this); }

int filebuf::overflow(int c)
{
  return _IO_file_overflow(this, c);
}

int filebuf::underflow()
{
  return _IO_file_underflow(this);
}

int filebuf::do_write(const char *data, int to_do)
{
  return _IO_do_write(this, data, to_do);
}

int filebuf::sync()
{
  return _IO_file_sync(this);
}

streampos filebuf::seekoff(streamoff offset, _seek_dir dir, int mode)
{
  return _IO_file_seekoff (this, offset, convert_to_seekflags(dir, mode));
}

filebuf* filebuf::close()
{
  return (_IO_file_close_it(this) ? NULL : this);
}

streamsize filebuf::sys_read(char* buf, streamsize size)
{
  return _IO_file_read(this, buf, size);
}

streampos filebuf::sys_seek(streamoff offset, _seek_dir dir)
{
  return _IO_file_seek(this, offset, dir);
}

streamsize filebuf::sys_write(const char *buf, streamsize n)
{
  return _IO_file_write (this, buf, n);
}

int filebuf::sys_stat(void* st)
{
  return _IO_file_stat (this, st);
}

int filebuf::sys_close()
{
  return _IO_file_close (this);
}

streamsize filebuf::xsputn(const char *s, streamsize n)
{
  return _IO_file_xsputn(this, s, n);
}

streamsize filebuf::xsgetn(char *s, streamsize n)
{
    // FIXME: OPTIMIZE THIS (specifically, when unbuffered()).
    return streambuf::xsgetn(s, n);
}

// Non-ANSI AT&T-ism:  Default open protection.
const int filebuf::openprot = 0644;
