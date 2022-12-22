/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	%W% (Berkeley) %G%
 */

#ifndef _ERR_H_
#define	_ERR_H_

/*
 * Don't use va_list in the err/warn prototypes.   Va_list is typedef'd in two
 * places (<machine/varargs.h> and <machine/stdarg.h>), so if we include one
 * of them here we may collide with the utility's includes.  It's unreasonable
 * for utilities to have to include one of them to include err.h, so we get
 * _VA_LIST_ from <machine/ansi.h> and use it.
 */
#include <machine/ansi.h>
#include <sys/cdefs.h>

__BEGIN_DECLS
volatile void	err __P((int, const char *, ...));
volatile void	verr __P((int, const char *, _VA_LIST_));
volatile void	errx __P((int, const char *, ...));
volatile void	verrx __P((int, const char *, _VA_LIST_));
void		warn __P((const char *, ...));
void		vwarn __P((const char *, _VA_LIST_));
void		warnx __P((const char *, ...));
void		vwarnx __P((const char *, _VA_LIST_));
__END_DECLS

#endif /* !_ERR_H_ */
