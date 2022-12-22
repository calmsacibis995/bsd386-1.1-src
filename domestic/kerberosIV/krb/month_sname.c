/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/month_sname.c,v $
 * $Author: kolstad $
 *
 * Copyright 1985, 1986, 1987, 1988 by the Massachusetts Institute
 * of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef lint
static char *rcsid_month_sname_c =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/month_sname.c,v 1.2 1992/01/04 18:38:48 kolstad Exp $";
#endif /* lint */

#include <mit-copyright.h>

/*
 * Given an integer 1-12, month_sname() returns a string
 * containing the first three letters of the corresponding
 * month.  Returns 0 if the argument is out of range.
 */

char *month_sname(n)
    int n;
{
    static char *name[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    return((n < 1 || n > 12) ? 0 : name [n-1]);
}
