/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/conf.h,v $
 * $Author: kolstad $
 * $Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/conf.h,v 1.2 1992/01/04 18:36:01 kolstad Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Configuration info for operating system, hardware description,
 * language implementation, C library, etc.
 *
 * This file should be included in (almost) every file in the Kerberos
 * sources, and probably should *not* be needed outside of those
 * sources.  (How do we deal with /usr/include/des.h and
 * /usr/include/krb.h?)
 */

#ifndef _CONF_H_

#include <mit-copyright.h>

#include "osconf.h"

#ifdef SHORTNAMES
#include "names.h"
#endif

/*
 * Language implementation-specific definitions
 */

/* special cases */
#ifdef __HIGHC__
/* broken implementation of ANSI C */
#undef __STDC__
#endif

#ifndef __STDC__
typedef char *pointer;		/* pointer to generic data */
#define PROTOTYPE(p) ()
#else
typedef void *pointer;
#define PROTOTYPE(p) p
#endif

/* Does your compiler understand "void"? */
#ifdef notdef
#define void int
#endif

/*
 * A few checks to see that necessary definitions are included.
 */

/* byte order */

#ifndef MSBFIRST
#ifndef LSBFIRST
/* #error byte order not defined */
Error: byte order not defined.
#endif
#endif

/* machine size */
#ifndef BITS16
#ifndef BITS32
Error: how big is this machine anyways?
#endif
#endif

/* end of checks */

#endif /* _CONF_H_ */
