/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/kdestroy/kdestroy.c,v $
 * $Author: kolstad $ 
 *
 * Copyright 1987, 1988 by the Massachusetts Institute of Technology. 
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>. 
 *
 * This program causes Kerberos tickets to be destroyed.
 * Options are: 
 *
 *   -q[uiet]	- no bell even if tickets not destroyed
 *   -f[orce]	- no message printed at all 
 */

#include <mit-copyright.h>

#ifndef	lint
static char rcsid_kdestroy_c[] =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/kdestroy/kdestroy.c,v 1.2 1992/01/04 18:37:00 kolstad Exp $";
#endif	lint

#include <stdio.h>
#include <des.h>
#include <krb.h>
#include <string.h>

static char *pname;

main(argc, argv)
    char   *argv[];
{
    int     fflag=0, qflag=0, k_errno;
    register char *cp;

    cp = rindex (argv[0], '/');
    if (cp == NULL)
	pname = argv[0];
    else
	pname = cp+1;

    if (argc > 2)
	usage();
    else if (argc == 2) {
	if (!strcmp(argv[1], "-f"))
	    ++fflag;
	else if (!strcmp(argv[1], "-q"))
	    ++qflag;
	else usage();
    }

    k_errno = dest_tkt();

    if (fflag) {
	if (k_errno != 0 && k_errno != RET_TKFIL)
	    exit(1);
	else
	    exit(0);
    } else {
	if (k_errno == 0)
	    printf("Tickets destroyed.\n");
	else if (k_errno == RET_TKFIL)
	    fprintf(stderr, "No tickets to destroy.\n");
	else {
	    fprintf(stderr, "Tickets NOT destroyed (error).\n");
	    if (!qflag)
		fprintf(stderr, "\007");
	    exit(1);
	}
    }
    exit(0);
}

usage()
{
    fprintf(stderr, "usage: %s [-fq]\n", pname);
    exit(1);
}
