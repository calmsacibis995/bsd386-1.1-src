/*
 * $Source: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/kdc.h,v $
 * $Author: kolstad $
 * $Header: /bsdi/MASTER/BSDI_OS/domestic/kerberosIV/include/kdc.h,v 1.2 1992/01/04 18:36:09 kolstad Exp $
 *
 * Copyright 1987, 1988 by the Massachusetts Institute of Technology. 
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>. 
 *
 * Include file for the Kerberos Key Distribution Center. 
 */

#include <mit-copyright.h>

#ifndef KDC_DEFS
#define KDC_DEFS

#define S_AD_SZ		sizeof(struct sockaddr_in)

#define max(a,b)	(a>b ? a : b)
#define min(a,b)	(a<b ? a : b)

#define TRUE		1
#define FALSE		0

#define MKEYFILE	"/etc/kerberosIV/master_key"
#define K_LOGFIL	"/var/log/kpropd.log"
#define KS_LOGFIL	"/var/log/kerberos_slave.log"
#define KRB_ACL		"/etc/kerberosIV/kerberos.acl"
#define KRB_PROG	"./kerberos"

#define ONE_MINUTE	60
#define FIVE_MINUTES	(5 * ONE_MINUTE)
#define ONE_HOUR	(60 * ONE_MINUTE)
#define ONE_DAY		(24 * ONE_HOUR)
#define THREE_DAYS	(3 * ONE_DAY)

#endif /* KDC_DEFS */

