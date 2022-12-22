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

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "iolibio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _IO_seekflags_ {
  _IO_seek_set = 0,
  _IO_seek_cur = 1,
  _IO_seek_end = 2,

  /* These bits are ignored unless the _IO_FILE has independent
     read and write positions. */
  _IO_seek_not_in = 4,	/* Don't move read posistion. */
  _IO_seek_not_out = 8,	/* Don't move write posistion. */
  _IO_seek_pos_ignored = 16 /* Result is ignored (except EOF) */
} _IO_seekflags;

typedef int (*_IO_overflow_t) _PARAMS((_IO_FILE*, int));
typedef int (*_IO_underflow_t) _PARAMS((_IO_FILE*));
typedef _IO_size_t (*_IO_xsputn_t) _PARAMS((_IO_FILE*,const void*,_IO_size_t));
typedef _IO_size_t (*_IO_xsgetn_t) _PARAMS((_IO_FILE*, void*, _IO_size_t));
typedef _IO_ssize_t (*_IO_read_t) _PARAMS((_IO_FILE*, void*, _IO_ssize_t));
typedef _IO_ssize_t (*_IO_write_t) _PARAMS((_IO_FILE*,const void*,_IO_ssize_t));
typedef int (*_IO_stat_t) _PARAMS((_IO_FILE*, void*));
typedef _IO_fpos_t (*_IO_seek_t) _PARAMS((_IO_FILE*, _IO_off_t, int));
typedef int (*_IO_doallocate_t) _PARAMS((_IO_FILE*));
typedef int (*_IO_pbackfail_t) _PARAMS((_IO_FILE*, int));
typedef int (*_IO_setbuf_t) _PARAMS((_IO_FILE*, char *, _IO_ssize_t));
typedef int (*_IO_sync_t) _PARAMS((_IO_FILE*));
typedef void (*_IO_finish_t) _PARAMS((_IO_FILE*)); /* finalize */
typedef int (*_IO_close_t) _PARAMS((_IO_FILE*)); /* finalize */
typedef _IO_fpos_t (*_IO_seekoff_t) _PARAMS((_IO_FILE*, _IO_off_t, _IO_seekflags));

/* The _IO_seek_cur and _IO_seek_end options are not allowed. */
typedef _IO_fpos_t (*_IO_seekpos_t) _PARAMS((_IO_FILE*, _IO_fpos_t, _IO_seekflags));

struct _IO_jump_t {
    _IO_overflow_t __overflow;
    _IO_underflow_t __underflow;
    _IO_xsputn_t __xsputn;
    _IO_xsgetn_t __xsgetn;
    _IO_read_t __read;
    _IO_write_t __write;
    _IO_doallocate_t __doallocate;
    _IO_pbackfail_t __pbackfail;
    _IO_setbuf_t __setbuf;
    _IO_sync_t __sync;
    _IO_finish_t __finish;
    _IO_close_t __close;
    _IO_stat_t __stat;
    _IO_seek_t __seek;
    _IO_seekoff_t __seekoff;
    _IO_seekpos_t __seekpos;
#if 0
    get_column;
    set_column;
#endif
};

/* Generic functions */

extern _IO_fpos_t _IO_seekoff _PARAMS((_IO_FILE*, _IO_off_t, _IO_seekflags));
extern _IO_fpos_t _IO_seekpos _PARAMS((_IO_FILE*, _IO_fpos_t, _IO_seekflags));

extern int _IO_switch_to_get_mode _PARAMS((_IO_FILE*));
extern void _IO_init _PARAMS((_IO_FILE*, int));
extern int _IO_sputbackc _PARAMS((_IO_FILE*, int));
extern int _IO_sungetc _PARAMS((_IO_FILE*));
extern void _IO_un_link _PARAMS((_IO_FILE*));
extern void _IO_link_in _PARAMS((_IO_FILE *));
extern void _IO_doallocbuf _PARAMS((_IO_FILE*));
extern void _IO_unsave_markers _PARAMS((_IO_FILE*));
extern void _IO_setb _PARAMS((_IO_FILE*, char*, char*, int));
extern unsigned _IO_adjust_column _PARAMS((unsigned, const char *, int));
#define _IO_sputn(__fp, __s, __n) (__fp->_jumps->__xsputn(__fp, __s, __n))

/* Marker-related function. */

extern void _IO_init_marker _PARAMS((struct _IO_marker *, _IO_FILE *));
extern void _IO_remove_marker _PARAMS((struct _IO_marker*));
extern int _IO_marker_difference _PARAMS((struct _IO_marker *, struct _IO_marker *));
extern int _IO_marker_delta _PARAMS((struct _IO_marker *));
extern int _IO_seekmark _PARAMS((_IO_FILE *, struct _IO_marker *, int));

/* Default jumptable functions. */

extern int _IO_default_underflow _PARAMS((_IO_FILE*));
extern int _IO_default_doallocate _PARAMS((_IO_FILE*));
extern void _IO_default_finish _PARAMS((_IO_FILE *));
extern int _IO_default_pbackfail _PARAMS((_IO_FILE*, int));
extern int _IO_default_setbuf _PARAMS((_IO_FILE *, char*, _IO_ssize_t));
extern _IO_size_t _IO_default_xsputn _PARAMS((_IO_FILE *, const void*, _IO_size_t));
extern _IO_size_t _IO_default_xsgetn _PARAMS((_IO_FILE *, void*, _IO_size_t));
extern _IO_fpos_t _IO_default_seekoff _PARAMS((_IO_FILE*, _IO_off_t, _IO_seekflags));
extern _IO_fpos_t _IO_default_seekpos _PARAMS((_IO_FILE*, _IO_fpos_t, _IO_seekflags));
extern _IO_ssize_t _IO_default_write _PARAMS((_IO_FILE*,const void*,_IO_ssize_t));
extern _IO_ssize_t _IO_default_read _PARAMS((_IO_FILE*, void*, _IO_ssize_t));
extern int _IO_default_stat _PARAMS((_IO_FILE*, void*));
extern _IO_fpos_t _IO_default_seek _PARAMS((_IO_FILE*, _IO_off_t, int));
extern int _IO_default_sync _PARAMS((_IO_FILE*));
#define _IO_default_close ((_IO_close_t)_IO_default_sync)

extern struct _IO_jump_t _IO_file_jumps;
extern struct _IO_jump_t _IO_proc_jumps;
extern struct _IO_jump_t _IO_str_jumps;
extern int _IO_do_write _PARAMS((_IO_FILE*, const char*, _IO_size_t));
extern int _IO_flush_all _PARAMS(());
extern void _IO_flush_all_linebuffered _PARAMS(());

#define _IO_do_flush(_f) \
  _IO_do_write(_f, _f->_IO_write_base, _f->_IO_write_ptr-_f->_IO_write_base)
#define _IO_in_put_mode(_fp) ((_fp)->_flags & _IO_CURRENTLY_PUTTING)
#define _IO_mask_flags(fp, f, mask) \
       ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))
#define _IO_setg(fp, eb, g, eg)  ((fp)->_IO_read_base = (eb),\
	(fp)->_IO_read_ptr = (g), (fp)->_IO_read_end = (eg))
#define _IO_setp(__fp, __p, __ep) \
       ((__fp)->_IO_write_base = (__fp)->_IO_write_ptr = __p, (__fp)->_IO_write_end = (__ep))
#define _IO_have_backup(fp) ((fp)->_other_gbase != NULL)
#define _IO_in_backup(fp) ((fp)->_flags & _IO_IN_BACKUP)
#define _IO_have_markers(fp) ((fp)->_markers != NULL)
#define _IO_blen(p) ((fp)->_IO_buf_end - (fp)->_IO_buf_base)

/* Jumptable functions for files. */

extern int _IO_file_doallocate _PARAMS((_IO_FILE*));
extern int _IO_file_setbuf _PARAMS((_IO_FILE *, char*, _IO_ssize_t));
extern _IO_fpos_t _IO_file_seekoff _PARAMS((_IO_FILE*, _IO_off_t, _IO_seekflags));
extern _IO_size_t _IO_file_xsputn _PARAMS((_IO_FILE*,const void*,_IO_size_t));
extern int _IO_file_stat _PARAMS((_IO_FILE*, void*));
extern int _IO_file_close _PARAMS((_IO_FILE*));
extern int _IO_file_underflow _PARAMS((_IO_FILE *));
extern int _IO_file_overflow _PARAMS((_IO_FILE *, int));
#define _IO_file_is_open(__fp) ((__fp)->_fileno >= 0)
extern void _IO_file_init _PARAMS((_IO_FILE*));
extern _IO_FILE* _IO_file_attach _PARAMS((_IO_FILE*, int));
extern _IO_FILE* _IO_file_fopen _PARAMS((_IO_FILE*, const char*, const char*));
extern _IO_ssize_t _IO_file_write _PARAMS((_IO_FILE*,const void*,_IO_ssize_t));
extern _IO_ssize_t _IO_file_read _PARAMS((_IO_FILE*, void*, _IO_ssize_t));
extern int _IO_file_sync _PARAMS((_IO_FILE*));
extern int _IO_file_close_it _PARAMS((_IO_FILE*));
extern _IO_fpos_t _IO_file_seek _PARAMS((_IO_FILE *, _IO_off_t, int));
extern void _IO_file_finish _PARAMS((_IO_FILE*));

/* Other file functions. */
extern _IO_FILE* _IO_file_attach _PARAMS((_IO_FILE *, int));

/* Jumptable functions for proc_files. */
extern _IO_FILE* _IO_proc_open _PARAMS((_IO_FILE*, const char*, const char *));
extern int _IO_proc_close _PARAMS((_IO_FILE*));

/* Jumptable functions for strfiles. */
extern int _IO_str_underflow _PARAMS((_IO_FILE*));
extern int _IO_str_overflow _PARAMS((_IO_FILE *, int));
extern int _IO_str_pbackfail _PARAMS((_IO_FILE*, int));
extern _IO_fpos_t _IO_str_seekoff _PARAMS((_IO_FILE*,_IO_off_t,_IO_seekflags));

/* Other strfile functions */
extern void _IO_str_init_static _PARAMS((_IO_FILE *, char*, int, char*));
extern void _IO_str_init_readonly _PARAMS((_IO_FILE *, const char*, int));
extern _IO_ssize_t _IO_str_count _PARAMS ((_IO_FILE*));

extern _IO_size_t _IO_getline _PARAMS((_IO_FILE*,char*,_IO_size_t,int,int));
extern double _IO_strtod _PARAMS((const char *, char **));
extern char * _IO_dtoa _PARAMS((double __d, int __mode, int __ndigits,
				int *__decpt, int *__sign, char **__rve));
extern int _IO_outfloat _PARAMS((double __value, _IO_FILE *__sb, int __type,
				 int __width, int __precision, int __flags,
				 int __sign_mode, int __fill));

extern _IO_FILE *_IO_list_all;
extern void (*_IO_cleanup_registration_needed)();

#ifndef EOF
#define EOF (-1)
#endif
#ifndef NULL
#if !defined(__cplusplus) || defined(__GNUC__)
#define NULL ((void*)0)
#else
#define NULL (0)
#endif
#endif

#define FREE_BUF(_B) free(_B)
#define ALLOC_BUF(_S) (char*)malloc(_S)

#ifndef OS_FSTAT
#define OS_FSTAT fstat
#endif
struct stat;
extern _IO_ssize_t _IO_read _PARAMS((int, void*, _IO_size_t));
extern _IO_ssize_t _IO_write _PARAMS((int, const void*, _IO_size_t));
extern _IO_off_t _IO_lseek _PARAMS((int, _IO_off_t, int));
extern int _IO_close _PARAMS((int));
extern int _IO_fstat _PARAMS((int, struct stat *));

/* Operations on _IO_fpos_t.
   Normally, these are trivial, but we provide hooks for configurations
   where an _IO_fpos_t is a struct.
   Note that _IO_off_t must be an integral type. */

/* _IO_pos_BAD is an _IO_fpos_t value indicating error, unknown, or EOF. */
#ifndef _IO_pos_BAD
#define _IO_pos_BAD ((_IO_fpos_t)(-1))
#endif
/* _IO_pos_as_off converts an _IO_fpos_t value to an _IO_off_t value. */
#ifndef _IO_pos_as_off
#define _IO_pos_as_off(__pos) ((_IO_off_t)(__pos))
#endif
/* _IO_pos_adjust adjust an _IO_fpos_t by some number of bytes. */
#ifndef _IO_pos_adjust
#define _IO_pos_adjust(__pos, __delta) ((__pos) += (__delta))
#endif
/* _IO_pos_0 is an _IO_fpos_t value indicating beginning of file. */
#ifndef _IO_pos_0
#define _IO_pos_0 ((_IO_fpos_t)0)
#endif

#ifdef __cplusplus
}
#endif

#if defined(__STDC__) || defined(__cplusplus)
#define _IO_va_start(args, last) va_start(args, last)
#else
#define _IO_va_start(args, last) va_start(args)
#endif

#if 1
#define COERCE_FILE(FILE) /* Nothing */
#else
/* This is part of the kludge for binary compatibility with old stdio. */
#define COERCE_FILE(FILE) \
  (((FILE)->_IO_file_flags & _IO_MAGIC_MASK) == _OLD_MAGIC_MASK \
    && (FILE) = *(FILE**)&((int*)fp)[1])
#endif
