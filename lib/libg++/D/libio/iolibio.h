#include "libio.h"

/* These emulate stdio functionality, but with a different name
   (_IO_ungetc instead of ungetc), and using _IO_FILE instead of FILE. */

extern int _IO_fclose _PARAMS((_IO_FILE*));
extern _IO_FILE *_IO_fdopen _PARAMS((int, const char*));
extern int _IO_fflush _PARAMS((_IO_FILE*));
extern int _IO_fgetpos _PARAMS((_IO_FILE*, _IO_fpos_t*));
extern char* _IO_fgets _PARAMS((char*, int, _IO_FILE*));
extern _IO_FILE *_IO_fopen _PARAMS((const char*, const char*));
extern int _IO_fprintf _PARAMS((_IO_FILE*, const char*, ...));
extern int _IO_fputs _PARAMS((const char*, _IO_FILE*));
extern int _IO_fsetpos _PARAMS((_IO_FILE*, const _IO_fpos_t *));
extern long int _IO_ftell _PARAMS((_IO_FILE*));
extern _IO_size_t _IO_fwrite _PARAMS((const void*,
				      _IO_size_t, _IO_size_t, _IO_FILE*));
extern char* _IO_gets _PARAMS((char*));
extern void _IO_perror _PARAMS((const char*));
extern int _IO_puts _PARAMS((const char*));
extern int _IO_scanf _PARAMS((const char*, ...));
extern void _IO_setbuffer _PARAMS((_IO_FILE *, char*, _IO_size_t));
extern int _IO_setvbuf _PARAMS((_IO_FILE*, char*, int, _IO_size_t));
extern int _IO_sscanf _PARAMS((const char*, const char*, ...));
extern int _IO_sprintf _PARAMS((char *, const char*, ...));
extern int _IO_ungetc _PARAMS((int, _IO_FILE*));
extern int _IO_vsscanf _PARAMS((const char *, const char *, _IO_va_list));
extern int _IO_vsprintf _PARAMS((char*, const char*, _IO_va_list));
#ifndef _IO_pos_BAD
#define _IO_pos_BAD ((_IO_fpos_t)(-1))
#endif
#define _IO_clearerr(FP) ((FP)->_flags &= ~(_IO_ERR_SEEN|_IO_EOF_SEEN))
#define _IO_feof(__fp) (((__fp)->_flags & _IO_EOF_SEEN) != 0)
#define _IO_ferror(__fp) (((__fp)->_flags & _IO_ERR_SEEN) != 0)
#define _IO_fseek(__fp, __offset, __whence) \
  (_IO_seekoff(__fp, __offset, (_IO_off_t)(__whence)) == _IO_pos_BAD ? EOF : 0)
#define _IO_rewind(FILE) (void)_IO_seekoff(FILE, 0, 0)
#define _IO_vprintf(FORMAT, ARGS) _IO_vfprintf(_IO_stdout, FORMAT, ARGS)
#define _IO_freopen(FILENAME, MODE, FP) \
  (_IO_file_close_it(FP), _IO_file_fopen(FP, FILENAME, MODE))
#define _IO_fileno(FP) ((FP)->_fileno)
extern _IO_FILE* _IO_popen _PARAMS((const char*, const char*));
#define _IO_pclose _IO_fclose
#define _IO_setbuf(_FP, _BUF) _IO_setbuffer(_FP, _BUF, _IO_BUFSIZ)
#define _IO_setlinebuf(_FP) _IO_setvbuf(_FP, NULL, 1, 0)

