/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/one.c,v $
 * $Author: kolstad $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef	lint
static char rcsid_one_c[] =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/one.c,v 1.2 1992/01/04 18:38:54 kolstad Exp $";
#endif	lint

#include <mit-copyright.h>

/*
 * definition of variable set to 1.
 * used in krb_conf.h to determine host byte order.
 */

int krbONE = 1;
