/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/getst.c,v $
 * $Author: kolstad $
 *
 * Copyright 1987, 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef lint
static char rcsid_getst_c[] =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/getst.c,v 1.2 1992/01/04 18:38:10 kolstad Exp $";
#endif /* lint */

#include <mit-copyright.h>

/*
 * getst() takes a file descriptor, a string and a count.  It reads
 * from the file until either it has read "count" characters, or until
 * it reads a null byte.  When finished, what has been read exists in
 * the given string "s".  If "count" characters were actually read, the
 * last is changed to a null, so the returned string is always null-
 * terminated.  getst() returns the number of characters read, including
 * the null terminator.
 */

getst(fd, s, n)
    int fd;
    register char *s;
{
    register count = n;
    while (read(fd, s, 1) > 0 && --count)
        if (*s++ == '\0')
            return (n - count);
    *s = '\0';
    return (n - count);
}
