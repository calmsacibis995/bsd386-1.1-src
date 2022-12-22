/* This is part of the iostream/stdio library, providing -*- C -*- I/O.
   Define ANSI C stdio on top of C++ iostreams.
   Copyright (C) 1991 Per Bothner.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.


This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef _STDIO_H
#define _STDIO_H
#define _STDIO_USES_IOSTREAM

#include <libio.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL (void*)0
#endif
#endif

#ifndef EOF
#define EOF (-1)
#endif
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

 /* define size_t.  Crud in case <sys/types.h> has defined it. */
#if !defined(_SIZE_T) && !defined(_T_SIZE_) && !defined(_T_SIZE)
#if !defined(__SIZE_T) && !defined(_SIZE_T_) && !defined(___int_size_t_h)
#if !defined(_GCC_SIZE_T) && !defined(_SIZET_)
#define _SIZE_T
#define _T_SIZE_
#define _T_SIZE
#define __SIZE_T
#define _SIZE_T_
#define ___int_size_t_h
#define _GCC_SIZE_T
#define _SIZET_
typedef _IO_size_t size_t;
#endif
#endif
#endif

typedef struct _IO_FILE FILE;
typedef _IO_fpos_t fpos_t;

#define FOPEN_MAX     _G_FOPEN_MAX
#define FILENAME_MAX _G_FILENAME_MAX
#define TMP_MAX 999 /* Only limited by filename length */

#define L_ctermid     9
#define L_cuserid     9
#define P_tmpdir      "/tmp"
#define L_tmpnam      20

/* For use by debuggers. These are linked in if printf or fprintf are used. */
extern FILE *stdin, *stdout, *stderr; /* TODO */

#define stdin _IO_stdin
#define stdout _IO_stdout
#define stderr _IO_stderr

#define getc(fp) _IO_getc(fp)
#define putc(c, fp) _IO_putc(c, fp)
#define putchar(c) putc(c, stdout)
#define getchar() getc(stdin)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define _ARGS(args) args
#else
#define _ARGS(args) ()
#endif

extern void clearerr _ARGS((FILE*));
extern int fclose _ARGS((FILE*));
extern int feof _ARGS((FILE*));
extern int ferror _ARGS((FILE*));
extern int fflush _ARGS((FILE*));
extern int fgetc _ARGS((FILE *));
extern int fgetpos _ARGS((FILE* fp, fpos_t *pos));
extern char* fgets _ARGS((char*, int, FILE*));
extern FILE* fopen _ARGS((const char*, const char*));
extern int fprintf _ARGS((FILE*, const char* format, ...));
extern int fputc _ARGS((int, FILE*));
extern int fputs _ARGS((const char *str, FILE *fp));
extern size_t fread _ARGS((void*, size_t, size_t, FILE*));
extern FILE* freopen _ARGS((const char*, const char*, FILE*));
extern int fscanf _ARGS((FILE *fp, const char* format, ...));
extern int fseek _ARGS((FILE* fp, long int offset, int whence));
extern int fsetpos _ARGS((FILE* fp, const fpos_t *pos));
extern long int ftell _ARGS((FILE* fp));
extern size_t fwrite _ARGS((const void*, size_t, size_t, FILE*));
extern char* gets _ARGS((char*));
extern void perror _ARGS((const char *));
extern int printf _ARGS((const char* format, ...));
extern int puts _ARGS((const char *str));
extern int remove _ARGS((const char*));
extern int rename _ARGS((const char* _old, const char* _new));
extern void rewind _ARGS((FILE*));
extern int scanf _ARGS((const char* format, ...));
extern void setbuf _ARGS((FILE*, char*));
extern void setlinebuf _ARGS((FILE*));
extern void setbuffer _ARGS((FILE*, char*, int));
extern int setvbuf _ARGS((FILE*, char*, int mode, size_t size));
extern int sprintf _ARGS((char*, const char* format, ...));
extern int sscanf _ARGS((const char* string, const char* format, ...));
extern FILE* tmpfile _ARGS((void));
extern char* tmpnam _ARGS((char*));
extern int ungetc _ARGS((int c, FILE* fp));
extern int vfprintf _ARGS((FILE *fp, char const *fmt0, _G_va_list));
extern int vprintf _ARGS((char const *fmt, _G_va_list));
extern int vsprintf _ARGS((char* string, const char* format, _G_va_list));

#if !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)
extern FILE *fdopen _ARGS((int, const char *));
extern int fileno _ARGS((FILE*));
extern FILE* popen _ARGS((const char*, const char*));
extern int pclose _ARGS((FILE*));
#endif

extern int __underflow _ARGS((struct _IO_FILE*));
extern int __overflow _ARGS((struct _IO_FILE*, int));

#ifdef __cplusplus
}
#endif

#endif /*!_STDIO_H*/
