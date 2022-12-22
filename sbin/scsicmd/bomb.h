/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI bomb.h,v 1.1.1.1 1992/02/19 17:34:27 trent Exp	*/
/*-
 * bomb.h
 *
 * External definitions for the 'bomb' routines.
 *
 * Remarks:
 *	If you reference and define the variable program_name,
 *	it will be used as a string to prefix error messages.
 */

#ifndef _BOMB_H_
#define _BOMB_H_

/*
 * Hidden variables.
 */
extern char *_file_name;
extern int _line_number;
extern volatile void _bomb __P((const char *, ...));
extern void _complain __P((const char *, ...));

extern char *program_name;
extern int errno;

/*
 * Print an error message and die, with a nonzero exit status.  Format is like
 * printf(); a newline is appended to the message after it is formatted.  The
 * structure of the message:
 * 
 * [program_name:][file_name, line_number:]message[:system-error]
 * 
 * If program_name is not defined, it is omitted.  To suppress the source file
 * and line number, use [s]xbomb().  To suppress the system error message
 * (when the error is 'soft'), use s[x]bomb().  Compile with '-Dxbomb' to
 * prevent messages with references to source files.  complain() is like
 * bomb() but does not force an exit.
 */
#ifndef xbomb

#define	bomb		_file_name = __FILE__, \
			_line_number = __LINE__, \
			_bomb
#define	sbomb		errno = 0, bomb

#define	complain	_file_name = __FILE__, \
			_line_number = __LINE__, \
			_complain
#define	scomplain	errno = 0, complain

#else

#undef xbomb
#define	bomb		xbomb
#define	sbomb		sxbomb
#define	complain	xcomplain
#define	scomplain	sxcomplain

#endif	/* xbomb */

#define	xbomb		_file_name = NULL, _bomb
#define	sxbomb		errno = 0, xbomb

#define	xcomplain	_file_name = NULL, _complain
#define	sxcomplain	errno = 0, xcomplain

#endif
