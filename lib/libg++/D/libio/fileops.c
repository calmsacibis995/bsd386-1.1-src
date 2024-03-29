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

/*  written by Per Bothner (bothner@cygnus.com) */

#define _POSIX_SOURCE
#include "libioP.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

/* An fstream can be in at most one of put mode, get mode, or putback mode.
   Putback mode is a variant of get mode.

   In a filebuf, there is only one current position, instead of two
   separate get and put pointers.  In get mode, the current posistion
   is that of gptr(); in put mode that of pptr().

   The position in the buffer that corresponds to the position
   in external file system is file_ptr().
   This is normally egptr(), except in putback mode, when it is _save_egptr.
   If the field _fb._offset is >= 0, it gives the offset in
   the file as a whole corresponding to eGptr(). (???)

   PUT MODE:
   If a filebuf is in put mode, pbase() is non-NULL and equal to base().
   Also, epptr() == ebuf().
   Also, eback() == gptr() && gptr() == egptr().
   The un-flushed character are those between pbase() and pptr().
   GET MODE:
   If a filebuf is in get or putback mode, eback() != egptr().
   In get mode, the unread characters are between gptr() and egptr().
   The OS file position corresponds to that of egptr().
   PUTBACK MODE:
   Putback mode is used to remember "excess" characters that have
   been sputbackc'd in a separate putback buffer.
   In putback mode, the get buffer points to the special putback buffer.
   The unread characters are the characters between gptr() and egptr()
   in the putback buffer, as well as the area between save_gptr()
   and save_egptr(), which point into the original reserve buffer.
   (The pointers save_gptr() and save_egptr() are the values
   of gptr() and egptr() at the time putback mode was entered.)
   The OS position corresponds to that of save_egptr().
   
   LINE BUFFERED OUTPUT:
   During line buffered output, pbase()==base() && epptr()==base().
   However, ptr() may be anywhere between base() and ebuf().
   This forces a call to filebuf::overflow(int C) on every put.
   If there is more space in the buffer, and C is not a '\n',
   then C is inserted, and pptr() incremented.
   
   UNBUFFERED STREAMS:
   If a filebuf is unbuffered(), the _shortbuf[1] is used as the buffer.
*/

#define CLOSED_FILEBUF_FLAGS \
  (_IO_IS_FILEBUF+_IO_NO_READS+_IO_NO_WRITES+_IO_TIED_PUT_GET)


void
_IO_file_init(fp)
     register _IO_FILE *fp;
{
  fp->_offset = _IO_pos_0;

  _IO_link_in(fp);
  fp->_fileno = -1;
}

int
_IO_file_close_it(fp)
     register _IO_FILE* fp;
{
  int status;
  if (!_IO_file_is_open(fp))
    return EOF;

  /* This flushes as well as switching mode. */
  if (fp->_IO_write_ptr > fp->_IO_write_base || _IO_in_put_mode(fp))
    if (_IO_switch_to_get_mode(fp))
	  return EOF;

  _IO_unsave_markers(fp);

  status = fp->_jumps->__close(fp);
  
  /* Free buffer. */
  _IO_setb(fp, NULL, NULL, 0);
  _IO_setg(fp, NULL, NULL, NULL);
  _IO_setp(fp, NULL, NULL);

  _IO_un_link(fp);
  fp->_flags = _IO_MAGIC|CLOSED_FILEBUF_FLAGS;
  fp->_fileno = EOF;
  fp->_offset = _IO_pos_0;

  return status;
}

void
_IO_file_finish(fp)
     register _IO_FILE* fp;
{
  if (!(fp->_flags & _IO_DELETE_DONT_CLOSE))
    _IO_file_close_it(fp);
  _IO_default_finish(fp);
}

_IO_FILE *
_IO_file_fopen(fp, filename, mode)
     register _IO_FILE *fp;
     const char *filename;
     const char *mode;
{
  int oflags = 0, omode;
  int read_write, fdesc;
  int oprot = 0666;
  if (_IO_file_is_open (fp))
    return 0;
  switch (*mode++) {
  case 'r':
    omode = O_RDONLY;
    read_write = _IO_NO_WRITES;
    break;
  case 'w':
    omode = O_WRONLY;
    oflags = O_CREAT|O_TRUNC;
    read_write = _IO_NO_READS;
    break;
  case 'a':
    omode = O_WRONLY;
    oflags = O_CREAT|O_APPEND;
    read_write = _IO_NO_READS|_IO_IS_APPENDING;
    break;
  default:
    errno = EINVAL;
    return NULL;
  }
  if (mode[0] == '+' || (mode[0] == 'b' && mode[1] == '+')) {
    omode = O_RDWR;
    read_write &= _IO_IS_APPENDING;
  }
  fdesc = open(filename, omode|oflags, oprot);
  if (fdesc < 0)
    return NULL;
  fp->_fileno = fdesc;
  fp->_IO_file_flags = 
    _IO_mask_flags(fp, read_write,_IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);
  if (read_write & _IO_IS_APPENDING)
    if (fp->_jumps->__seekoff(fp, (_IO_off_t)0, _IO_seek_end) == _IO_pos_BAD)
      return NULL;
  _IO_link_in(fp);
  return fp;
}

_IO_FILE*
_IO_file_attach(fp, fd)
     _IO_FILE *fp;
     int fd;
{
  if (_IO_file_is_open(fp))
    return NULL;
  fp->_fileno = fd;
  fp->_flags &= ~(_IO_NO_READS+_IO_NO_WRITES);
  fp->_offset = _IO_pos_BAD;
  return fp;
}

int
_IO_file_setbuf(fp, p, len)
     register _IO_FILE *fp;
     char* p;
     _IO_ssize_t len;
{
    if (_IO_default_setbuf(fp, p, len) == EOF)
	return EOF;

    fp->_IO_write_base = fp->_IO_write_ptr = fp->_IO_write_end
      = fp->_IO_buf_base;
    _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);

    return 0;
}

int
_IO_do_write(fp, data, to_do)
     register _IO_FILE *fp;
     const char* data;
     _IO_size_t to_do;
{
  _IO_size_t count;
  if (to_do == 0)
    return 0;
  if (fp->_flags & _IO_IS_APPENDING)
    /* On a system without a proper O_APPEND implementation,
       you would need to sys_seek(0, SEEK_END) here, but is
       is not needed nor desirable for Unix- or Posix-like systems.
       Instead, just indicate that offset (before and after) is
       unpredictable. */
    fp->_offset = _IO_pos_BAD;
  else if (fp->_IO_read_end != fp->_IO_write_base)
    { 
      _IO_pos_t new_pos = fp->_jumps->__seek(fp, fp->_IO_write_base - fp->_IO_read_end, 1);
      if (new_pos == _IO_pos_BAD)
	return EOF;
      fp->_offset = new_pos;
    }
  count = fp->_jumps->__write(fp, data, to_do);
  if (fp->_cur_column)
    fp->_cur_column = _IO_adjust_column(fp->_cur_column - 1, data, to_do) + 1;
  _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
  fp->_IO_write_base = fp->_IO_write_ptr = fp->_IO_buf_base;
  fp->_IO_write_end = (fp->_flags & _IO_LINE_BUF+_IO_UNBUFFERED) ? fp->_IO_buf_base
    : fp->_IO_buf_end;
  return count != to_do ? EOF : 0;
}

int
_IO_file_underflow(fp)
     register _IO_FILE *fp;
{
  _IO_size_t count;
#if 0
  /* SysV does not make this test; take it out for compatibility */
  if (fp->_flags & _IO_EOF_SEEN)
    return (EOF);
#endif

  if (fp->_flags & _IO_NO_READS)
    return EOF;
  if (fp->_IO_read_ptr < fp->_IO_read_end)
    return *(unsigned char*)fp->_IO_read_ptr;

  if (fp->_IO_buf_base == NULL)
    _IO_doallocbuf(fp);

  /* Flush all line buffered files before reading. */
  /* FIXME This can/should be moved to genops ?? */
  if (fp->_flags & (_IO_LINE_BUF|_IO_UNBUFFERED))
    _IO_flush_all_linebuffered();

  _IO_switch_to_get_mode(fp);

  count = fp->_jumps->__read(fp, fp->_IO_buf_base,
			     fp->_IO_buf_end - fp->_IO_buf_base);
  if (count <= 0)
    {
      if (count == 0)
	fp->_flags |= _IO_EOF_SEEN;
      else
	fp->_flags |= _IO_ERR_SEEN, count = 0;
  }
  fp->_IO_read_base = fp->_IO_read_ptr = fp->_IO_buf_base;
  fp->_IO_read_end = fp->_IO_buf_base + count;
  fp->_IO_write_base = fp->_IO_write_ptr = fp->_IO_write_end
    = fp->_IO_buf_base;
  if (count == 0)
    return EOF;
  if (fp->_offset != _IO_pos_BAD)
    _IO_pos_adjust(fp->_offset, count);
  return *(unsigned char*)fp->_IO_read_ptr;
}

int _IO_file_overflow (f, ch)
     register _IO_FILE* f;
     int ch;
{
  if (f->_flags & _IO_NO_WRITES) /* SET ERROR */
    return EOF;
  /* If current reading or no buffer allocated. */
  if ((f->_flags & _IO_CURRENTLY_PUTTING) == 0)
    {
      /* Allocate a buffer if needed. */
      if (f->_IO_buf_base == 0)
	{
	  _IO_doallocbuf(f);
	  f->_IO_read_end = f->_IO_buf_base;
	  f->_IO_write_ptr = f->_IO_buf_base;
	}
      else /* Must be currently reading. */
	f->_IO_write_ptr = f->_IO_read_ptr;
      f->_IO_write_base = f->_IO_write_ptr;
      f->_IO_write_end = f->_IO_buf_end;
      f->_IO_read_base = f->_IO_read_ptr = f->_IO_read_end;

      if (f->_flags & _IO_LINE_BUF+_IO_UNBUFFERED)
	f->_IO_write_end = f->_IO_write_ptr;
      f->_flags |= _IO_CURRENTLY_PUTTING;
    }
  if (ch == EOF)
    return _IO_do_flush(f);
  if (f->_IO_write_ptr == f->_IO_buf_end ) /* Buffer is really full */
    if (_IO_do_flush(f) == EOF)
      return EOF;
  *f->_IO_write_ptr++ = ch;
  if ((f->_flags & _IO_UNBUFFERED)
      || ((f->_flags & _IO_LINE_BUF) && ch == '\n'))
    if (_IO_do_flush(f) == EOF)
      return EOF;
  return (unsigned char)ch;
}

int
_IO_file_sync(fp)
     register _IO_FILE* fp;
{
  _IO_size_t delta;
  /*    char* ptr = cur_ptr(); */
  if (fp->_IO_write_ptr > fp->_IO_write_base)
    if (_IO_do_flush(fp)) return EOF;
  delta = fp->_IO_read_ptr - fp->_IO_read_end; 
  if (delta != 0)
    {
#ifdef TODO
      if (_IO_in_backup(fp))
	delta -= eGptr() - Gbase();
#endif
      _IO_off_t new_pos = fp->_jumps->__seek(fp, delta, 1);
      if (new_pos == (_IO_off_t)EOF)
	return EOF;
      fp->_offset = new_pos;
      fp->_IO_read_end = fp->_IO_read_ptr;
    }
  /* FIXME: Cleanup - can this be shared? */
  /*    setg(base(), ptr, ptr); */
  return 0;
}

_IO_pos_t
_IO_file_seekoff(fp, offset, mode)
     register _IO_FILE *fp;
     _IO_off_t offset;
     _IO_seekflags mode;
{
  _IO_pos_t result;
  _IO_off_t delta, new_offset;
  long count;
  int dir = mode & 3;

  if ((mode & _IO_seek_not_in) && (mode & _IO_seek_not_out))
    dir = _IO_seek_cur, offset = 0; /* Don't move any pointers. */

  /* Flush unwritten characters.
     (This may do an unneeded write if we seek within the buffer.
     But to be able to switch to reading, we would need to set
     egptr to ptr.  That can't be done in the current design,
     which assumes file_ptr() is eGptr.  Anyway, since we probably
     end up flushing when we close(), it doesn't make much difference.)
     FIXME: simulate mem-papped files. */

  if (fp->_IO_write_ptr > fp->_IO_write_base || _IO_in_put_mode(fp))
    if (_IO_switch_to_get_mode(fp)) return EOF;

  if (fp->_IO_buf_base == NULL)
    {
      _IO_doallocbuf(fp);
      _IO_setp(fp, fp->_IO_buf_base, fp->_IO_buf_base);
      _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
    }

  switch (dir)
    {
    case _IO_seek_cur:
      if (fp->_offset == _IO_pos_BAD)
	goto dumb;
      /* Adjust for read-ahead (bytes is buffer). */
      offset -= fp->_IO_read_end - fp->_IO_read_ptr;
      /* Make offset absolute, assuming current pointer is file_ptr(). */
      offset += _IO_pos_as_off(fp->_offset);

      dir = _IO_seek_set;
      break;
    case _IO_seek_set:
      break;
    case _IO_seek_end:
      {
	struct stat st;
	if (fp->_jumps->__stat(fp, &st) == 0 && S_ISREG(st.st_mode))
	  {
	    offset += st.st_size;
	    dir = _IO_seek_set;
	  }
	else
	  goto dumb;
      }
    }
  /* At this point, dir==_IO_seek_set. */

#ifdef TODO
  /* If destination is within current buffer, optimize: */
  if (fp->_offset != IO_pos_BAD && fp->_IO_read_base != NULL)
    {
      /* Offset relative to start of main get area. */
      _IO_pos_t rel_offset = offset - _fb._offset
	+ (eGptr()-Gbase());
      if (rel_offset >= 0)
	{
	  if (_IO_in_backup(fp))
	    _IO_switch_to_main_get_area(fp);
	  if (rel_offset <= _IO_read_end - _IO_read_base)
	    {
	      _IO_setg(fp->_IO_buf_base, fp->_IO_buf_base + rel_offset,
		       fp->_IO_read_end);
	      _IO_setp(fp->_IO_buf_base, fp->_IO_buf_base);
	      return offset;
	    }
	    /* If we have streammarkers, seek forward by reading ahead. */
	    if (_IO_have_markers(fp))
	      {
		int to_skip = rel_offset
		  - (fp->_IO_read_ptr - fp->_IO_read_base);
		if (ignore(to_skip) != to_skip)
		  goto dumb;
		return offset;
	      }
	}
      if (rel_offset < 0 && rel_offset >= Bbase() - Bptr())
	{
	  if (!_IO_in_backup(fp))
	    _IO_switch_to_backup_area(fp);
	  gbump(fp->_IO_read_end + rel_offset - fp->_IO_read_ptr);
	  return offset;
	}
    }

  _IO_unsave_markers(fp);
#endif

  /* Try to seek to a block boundary, to improve kernel page management. */
  new_offset = offset & ~(fp->_IO_buf_end - fp->_IO_buf_base - 1);
  delta = offset - new_offset;
  if (delta > fp->_IO_buf_end - fp->_IO_buf_base)
    {
      new_offset = offset;
      delta = 0;
    }
  result = fp->_jumps->__seek(fp, new_offset, 0);
  if (result < 0)
    return EOF;
  if (delta == 0)
    count = 0;
  else
    {
      count = fp->_jumps->__read(fp, fp->_IO_buf_base,
				 fp->_IO_buf_end - fp->_IO_buf_base);
      if (count < delta)
	{
	  /* We weren't allowed to read, but try to seek the remainder. */
	  offset = count == EOF ? delta : delta-count;
	  dir = _IO_seek_cur;
	  goto dumb;
	}
    }
  _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base+delta, fp->_IO_buf_base+count);
  _IO_setp(fp, fp->_IO_buf_base, fp->_IO_buf_base);
  fp->_offset = result + count;
  _IO_mask_flags(fp, 0, _IO_EOF_SEEN);
  return offset;
 dumb:

  _IO_unsave_markers(fp);
  result = fp->_jumps->__seek(fp, offset, dir);
  if (result != EOF)
    _IO_mask_flags(fp, 0, _IO_EOF_SEEN);
  fp->_offset = result;
  _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
  _IO_setp(fp, fp->_IO_buf_base, fp->_IO_buf_base);
  return result;
}

_IO_ssize_t
_IO_file_read(fp, buf, size)
     register _IO_FILE* fp;
     void* buf;
     _IO_ssize_t size;
{
  for (;;)
    {
      _IO_ssize_t count = _IO_read(fp->_fileno, buf, size);
#ifdef EINTR
      if (errno == EINTR && count == -1)
	continue;
#endif
      return count;
    }
}

_IO_pos_t
_IO_file_seek(fp, offset, dir)
     _IO_FILE *fp;
     _IO_off_t offset;
     int dir;
{
  return _IO_lseek(fp->_fileno, offset, dir);
}

int
_IO_file_stat(fp, st)
     _IO_FILE *fp;
     void* st;
{
  return _IO_fstat(fp->_fileno, (struct stat*)st);
}

int
_IO_file_close(fp)
     _IO_FILE* fp;
{
  return _IO_close(fp->_fileno);
}

_IO_ssize_t
_IO_file_write(f, data, n)
     register _IO_FILE* f;
     const void* data;
     _IO_ssize_t n;
{
  _IO_ssize_t to_do = n;
  while (to_do > 0)
    {
      _IO_ssize_t count = _IO_write(f->_fileno, data, to_do);
      if (count == EOF)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
	  else
#endif
	    {
	      f->_flags |= _IO_ERR_SEEN;
	      break;
            }
        }
      to_do -= count;
      data = (void*)((char*)data + count);
    }
  n -= to_do;
  if (f->_offset >= 0)
    f->_offset += n;
  return n;
}

_IO_size_t
_IO_file_xsputn(f, data, n)
     _IO_FILE *f;
     const void *data;
     _IO_size_t n;
{
  register const char *s = data;
  _IO_size_t to_do = n;
  int must_flush = 0;
  _IO_size_t count;

  if (n <= 0)
    return 0;
  /* This is an optimized implementation.
     If the amount to be written straddles a block boundary
     (or the filebuf is unbuffered), use sys_write directly. */

  /* First figure out how much space is available in the buffer. */
  count = f->_IO_write_end - f->_IO_write_ptr; /* Space available. */
  if ((f->_flags & _IO_LINE_BUF) && (f->_flags & _IO_CURRENTLY_PUTTING))
    {
      count = f->_IO_buf_end - f->_IO_write_ptr;
      if (count >= n)
	{ register const char *p;
	  for (p = s + n; p > s; )
	    {
	      if (*--p == '\n') {
		count = p - s + 1;
		must_flush = 1;
		break;
	      }
	    }
	}
    }
  /* Then fill the buffer. */
  if (count > 0)
    {
      if (count > to_do)
	count = to_do;
      if (count > 20) {
	memcpy(f->_IO_write_ptr, s, count);
	s += count;
      }
      else
	{
	  register char *p = f->_IO_write_ptr;
	  register int i = (int)count;
	  while (--i >= 0) *p++ = *s++;
	}
      f->_IO_write_ptr += count;
      to_do -= count;
    }
  if (to_do + must_flush > 0)
    { _IO_size_t block_size, dont_write;
      /* Next flush the (full) buffer. */
      if (__overflow(f, EOF) == EOF)
	return n - to_do;

      /* Try to maintain alignment: write a whole number of blocks.
	 dont_write is what gets left over. */
      block_size = f->_IO_buf_end - f->_IO_buf_base;
      dont_write = block_size >= 128 ? to_do % block_size : 0;

      count = to_do - dont_write;
      if (_IO_do_write(f, s, count) == EOF)
	return n - to_do;
      to_do = dont_write;
      
      /* Now write out the remainder.  Normally, this will fit in the
	 buffer, but it's somewhat messier for line-buffered files,
	 so we let _IO_xsputn handle the general case. */
      if (dont_write)
	to_do -= _IO_default_xsputn(f, s+count, dont_write);
    }
  return n - to_do;
}

struct _IO_jump_t _IO_file_jumps = {
  _IO_file_overflow,
  _IO_file_underflow,
  _IO_file_xsputn,
  _IO_default_xsgetn,
  _IO_file_read,
  _IO_file_write,
  _IO_file_doallocate,
  _IO_default_pbackfail,
  _IO_file_setbuf,
  _IO_file_sync,
  _IO_file_finish,
  _IO_file_close,
  _IO_file_stat,
  _IO_file_seek,
  _IO_file_seekoff,
  _IO_default_seekpos,
};
