
static char rcsid[] = "@(#)Id: safemalloc.c,v 5.1 1993/04/12 01:51:01 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
 *
 * 			Copyright (c) 1992 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: safemalloc.c,v $
 * Revision 1.1  1994/01/14  01:35:04  cp
 * 2.4.23
 *
 * Revision 5.1  1993/04/12  01:51:01  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include "defs.h"

/*
 * These routines perform dynamic memory allocation with error checking.
 * The "safe_malloc_fail_handler" vector points to a routine that is invoked
 * if memory allocation fails.  The default error handler displays a message
 * and aborts the program.
 */


void dflt_safe_malloc_fail_handler(proc, len)
char *proc;
unsigned len;
{
	fprintf(stderr,
		"error - out of memory [%s failed allocating %d bytes]\n",
		proc, len);
	exit(1);
}

void (*safe_malloc_fail_handler)() = dflt_safe_malloc_fail_handler;


malloc_t safe_malloc(len)
unsigned len;
{
	malloc_t p;
	if ((p = malloc(len)) == NULL)
		(*safe_malloc_fail_handler)("safe_malloc", len);
	return p;
}


malloc_t safe_realloc(p, len)
malloc_t p;
unsigned len;
{
	if ((p = (p == NULL ? malloc(len) : realloc((malloc_t)p, len))) == NULL)
		(*safe_malloc_fail_handler)("safe_realloc", len);
	return p;
}


char *safe_strdup(s)
char *s;
{
	char *p;
	if ((p = (char *) malloc(strlen(s)+1)) == NULL)
		(*safe_malloc_fail_handler)("safe_strdup", strlen(s)+1);
	return strcpy(p, s);
}
