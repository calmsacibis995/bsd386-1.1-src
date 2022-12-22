/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/conf-bsdvax.h,v $
 * $Author: kolstad $
 * $Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/conf-bsdvax.h,v 1.2 1992/01/04 18:35:51 kolstad Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Machine-type definitions: VAX
 */

#include <mit-copyright.h>

#define VAX
#define BITS32
#define BIG
#define LSBFIRST
#define BSDUNIX

#ifndef __STDC__
#ifndef NOASM
#define VAXASM
#endif /* no assembly */
#endif /* standard C */
