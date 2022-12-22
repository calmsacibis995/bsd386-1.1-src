/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/netwrite.c,v $
 * $Author: kolstad $
 *
 * Copyright 1987, 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef	lint
static char rcsid_netwrite_c[] =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/krb/netwrite.c,v 1.2 1992/01/04 18:38:52 kolstad Exp $";
#endif	lint

#include <mit-copyright.h>

/*
 * krb_net_write() writes "len" bytes from "buf" to the file
 * descriptor "fd".  It returns the number of bytes written or
 * a write() error.  (The calling interface is identical to
 * write(2).)
 *
 * XXX must not use non-blocking I/O
 */

int
krb_net_write(fd, buf, len)
int fd;
register char *buf;
int len;
{
    int cc;
    register int wrlen = len;
    do {
	cc = write(fd, buf, wrlen);
	if (cc < 0)
	    return(cc);
	else {
	    buf += cc;
	    wrlen -= cc;
	}
    } while (wrlen > 0);
    return(len);
}
