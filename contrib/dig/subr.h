
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
** Distributed with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

#define  SUCCESS                0
#define  TIME_OUT               -1
#define  NO_INFO                -2
#define  ERROR                  -3
#define  NONAUTH                -4

#define RES_DEBUG2      0x80000000
#define NAME_LEN 80

extern int   Print_query();
extern char *Print_cdname();
extern char *Print_cdname2();   /* fixed width */
extern char *Print_rr();
extern char *DecodeType();      /* descriptive version of p_type */
extern char *DecodeError();
extern char *Calloc();
extern void NsError();
extern void PrintServer();
extern void PrintHostInfo();
extern void ShowOptions();
extern void FreeHostInfoPtr();
extern FILE *OpenFile();
extern char *inet_ntoa();
extern char *res_skip();
 
