/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/kdb/print_princ.c,v $
 * $Author: kolstad $ 
 *
 * Copyright 1988 by the Massachusetts Institute of Technology. 
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>. 
 */

#ifndef	lint
static char rcsid_print_principal_c[] =
"$Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/kdb/print_princ.c,v 1.2 1992/01/04 18:36:40 kolstad Exp $";
#endif	lint

#include <mit-copyright.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <strings.h>
#include <des.h>
#include <krb.h>
#include <krb_db.h>

extern int debug;
extern char *strncpy();
extern char *ctime();
extern struct tm *localtime();
struct tm *time_p;

long    kerb_debug;

krb_print_principal(a_n)
    Principal *a_n;
{
    /* run-time database does not contain string versions */
    time_p = localtime((time_t *)&(a_n->exp_date));

    fprintf(stderr,
    "\n%s %s expires %4d-%2d-%2d %2d:%2d, max_life %d*5 = %d min  attr 0x%02x",
    a_n->name, a_n->instance,
    time_p->tm_year > 1900 ? time_p->tm_year : time_p->tm_year + 1900,
    time_p->tm_mon + 1, time_p->tm_mday,
    time_p->tm_hour, time_p->tm_min,
    a_n->max_life, 5 * a_n->max_life, a_n->attributes);

    fprintf(stderr,
    "\n\tkey_ver %d  k_low 0x%08x  k_high 0x%08x  akv %d  exists %d\n",
    a_n->key_version, a_n->key_low, a_n->key_high,
    a_n->kdc_key_ver, a_n->old);

    fflush(stderr);
}
