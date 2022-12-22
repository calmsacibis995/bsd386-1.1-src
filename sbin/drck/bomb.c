/*	BSDI $Id: bomb.c,v 1.1.1.1 1992/02/19 17:34:28 trent Exp $	 */
/*-
 * bomb.c
 *
 * _bomb( format, args ... ) -- Print args with format on stderr a la printf,
 *	and exit with status 1.  If program_name, _file_name or _line_number
 *	are set, these values are printed.  If errno is set, the corresponding
 *	error string is also produced.
 * _complain( format, args ... ) -- Similar, but does not exit.
 * _sbomb(), _scomplain() -- Macros which are for 'software' errors (errno
 *	is not checked and system errors are not printed).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

char *program_name;
char *_file_name;
int _line_number;

#if __STDC__
volatile void
_bomb(const char *format, ...)
#else
/* VARARGS */
void
_bomb(va_alist)
va_dcl
#endif
{
	va_list lp;
#if __STDC__ == 0
	char *format;
#endif
	int save_errno = errno;

	/*
	 * Format of errors:
	 * 
	 * program_name: file_name, line_number: message: errlist
	 */

#if __STDC__
	va_start(lp, format);
#else
	va_start(lp);
	format = va_arg(lp, char *);
#endif

	fflush(stdout);
	if (ferror(stderr))
		exit(1);	/* Nothing we can do */

	if (program_name)
		fprintf(stderr, "%s: ", program_name);
	if (_file_name && _line_number > 0)
		fprintf(stderr, "%s, %d: ", _file_name, _line_number);
	vfprintf(stderr, format, lp);
	if (save_errno > 0 && save_errno < sys_nerr)
		fprintf(stderr, ": %s", strerror(save_errno));
	putc('\n', stderr);
	fflush(stderr);
	exit(1);
}

#if __STDC__
void
_complain(const char *format, ...)
#else
/* VARARGS */
void
_complain(va_alist)
va_dcl
#endif
{
	va_list lp;
#if __STDC__ == 0
	char *format;
#endif
	int save_errno = errno;

	/*
	 * Format of errors:
	 * 
	 * program_name: file_name, line_number: message: errlist
	 */

#if __STDC__
	va_start(lp, format);
#else
	va_start(lp);
	format = va_arg(lp, char *);
#endif

	fflush(stdout);
	if (ferror(stderr))
		return;		/* Nothing we can do */

	if (program_name)
		fprintf(stderr, "%s: ", program_name);
	if (_file_name && _line_number > 0)
		fprintf(stderr, "%s, %d: ", _file_name, _line_number);
	vfprintf(stderr, format, lp);
	if (save_errno > 0 && save_errno < sys_nerr)
		fprintf(stderr, ": %s", strerror(save_errno));
	putc('\n', stderr);
	fflush(stderr);
	return;
}
