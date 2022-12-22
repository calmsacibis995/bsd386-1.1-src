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

#include "libioP.h"

#if _IO_HAVE_SYS_WAIT
#include <signal.h>
#include <unistd.h>
#ifdef __STDC__
/* This is now provided by gcc's fixproto. */
#include <stdlib.h>
#endif
#include <errno.h>
#ifndef errno
extern int errno;
#endif
#include <sys/wait.h>

#ifndef _IO_fork
#define _IO_fork vfork /* defined in libiberty, if needed */
_IO_pid_t _IO_fork();
#endif

#endif /* _IO_HAVE_SYS_WAIT */

#ifndef _IO_pipe
#define _IO_pipe pipe
extern int _IO_pipe();
#endif

#ifndef _IO_dup2
#define _IO_dup2 dup2
extern int _IO_dup2();
#endif

#ifndef _IO_waitpid
#define _IO_waitpid waitpid
#endif

#ifndef _IO_execl
#define _IO_execl execl
#endif
#ifndef _IO__exit
#define _IO__exit _exit
#endif

struct _IO_proc_file
{
  struct _IO_FILE_plus _file;
  /* Following fields must match those in class procbuf (procbuf.h) */
  _IO_pid_t _pid;
  struct _IO_proc_file *_next;
};
typedef struct _IO_proc_file _IO_proc_file;

static struct _IO_proc_file *proc_file_chain = NULL;

_IO_FILE *
_IO_proc_open(fp, command, mode)
     _IO_FILE* fp;
     const char *command;
     const char *mode;
{
#if _IO_HAVE_SYS_WAIT
  int read_or_write;
  int pipe_fds[2];
  int parent_end, child_end;
  _IO_pid_t child_pid;
  if (_IO_file_is_open(fp))
    return NULL;
  if (_IO_pipe(pipe_fds) < 0)
    return NULL;
  if (mode[0] == 'r')
    {
      parent_end = pipe_fds[0];
      child_end = pipe_fds[1];
      read_or_write = _IO_NO_WRITES;
    }
  else
    {
      parent_end = pipe_fds[1];
      child_end = pipe_fds[0];
      read_or_write = _IO_NO_READS;
    }
  ((_IO_proc_file*)fp)->_pid = child_pid = _IO_fork();
  if (child_pid == 0)
    {
      int child_std_end = mode[0] == 'r' ? 1 : 0;
      _IO_close(parent_end);
      if (child_end != child_std_end)
	{
	  _IO_dup2(child_end, child_std_end);
	  _IO_close(child_end);
	}
      /* Posix.2:  "popen() shall ensure that any streams from previous
         popen() calls that remain open in the parent process are closed
	 in the new child process." */
      while (proc_file_chain)
	{
	  _IO_close (_IO_fileno ((_IO_FILE *) proc_file_chain));
	  proc_file_chain = proc_file_chain->_next;
	}

      _IO_execl("/bin/sh", "sh", "-c", command, NULL);
      _IO__exit(127);
    }
  _IO_close(child_end);
  if (child_pid < 0)
    {
      _IO_close(parent_end);
      return NULL;
    }
  _IO_fileno(fp) = parent_end;

  /* Link into proc_file_chain. */
  ((_IO_proc_file*)fp)->_next = proc_file_chain;
  proc_file_chain = (_IO_proc_file*)fp;

  fp->_IO_file_flags
    = read_or_write | (fp->_IO_file_flags & ~(_IO_NO_READS|_IO_NO_WRITES));
  return fp;
#else /* !_IO_HAVE_SYS_WAIT */
  return NULL;
#endif
}

_IO_FILE *
_IO_popen(command, mode)
     const char *command;
     const char *mode;
{
  _IO_proc_file *fpx = (_IO_proc_file*)malloc(sizeof(_IO_proc_file));
  _IO_FILE *fp = (_IO_FILE*)fpx;
  if (fp == NULL)
    return NULL;
  _IO_init(fp, 0);
  fp->_jumps = &_IO_proc_jumps;
  _IO_file_init(fp);
  ((struct _IO_FILE_plus*)fp)->_vtable = NULL;
  if (_IO_proc_open (fp, command, mode) != NULL)
    return fp;
  free (fpx);
  return NULL;
}

int
_IO_proc_close(fp)
     _IO_FILE *fp;
{
  /* This is not name-space clean. FIXME! */
#if _IO_HAVE_SYS_WAIT
  int wstatus;
  _IO_proc_file **ptr = &proc_file_chain;
  _IO_pid_t wait_pid;
  int status = _IO_close(_IO_fileno(fp));

  /* Unlink from proc_file_chain. */
  for ( ; *ptr != NULL; ptr = &(*ptr)->_next)
    {
      if (*ptr == (_IO_proc_file*)fp)
	{
	  *ptr = (*ptr)->_next;
	  break;
	}
    }

  if (status < 0)
    return status;
  /* POSIX.2 Rationale:  "Some historical implementations either block
     or ignore the signals SIGINT, SIGQUIT, and SIGHUP while waiting
     for the child process to terminate.  Since this behavior is not
     described in POSIX.2, such implementations are not conforming." */
  do
    {
      wait_pid = _IO_waitpid (((_IO_proc_file*)fp)->_pid, &wstatus, 0);
    } while (wait_pid == -1 && errno == EINTR);
  if (wait_pid == -1)
    return -1;
  return wstatus;
#else /* !_IO_HAVE_SYS_WAIT */
  return -1;
#endif
}

struct _IO_jump_t _IO_proc_jumps = {
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
  _IO_proc_close,
  _IO_file_stat,
  _IO_file_seek,
  _IO_file_seekoff,
  _IO_default_seekpos,
};
