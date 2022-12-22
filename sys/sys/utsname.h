/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: utsname.h,v 1.2 1993/02/05 21:53:30 karels Exp $
 */

#ifndef _UTSNAME_H_
#define	_UTSNAME_H_

#define	_UTS_NAMESIZE	16
#define	_UTS_NODESIZE	256

struct utsname {
	char	nodename[_UTS_NODESIZE];
	char	sysname[_UTS_NAMESIZE];
	char	release[_UTS_NAMESIZE];
	char	version[_UTS_NAMESIZE];
	char	machine[_UTS_NAMESIZE];
};

#ifndef KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int	uname __P((struct utsname *));
__END_DECLS

#endif

#endif /* !_UTSNAME_H_ */
