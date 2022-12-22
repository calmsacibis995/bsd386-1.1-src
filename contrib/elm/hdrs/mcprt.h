#ifndef mcprt_h
#define mcprt_h


/***********************************************************
Copyright 1990, by Alfalfa Software Incorporated, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that Alfalfa's name not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

ALPHALPHA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
ALPHALPHA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

If you make any modifications, bugfixes or other changes to this software
we'd appreciate it if you could send a copy to us so we can keep things
up-to-date.  Many thanks.
				Kee Hinckley
				Alfalfa Software, Inc.
				267 Allston St., #3
				Cambridge, MA 02139  USA
				nazgul@alfalfa.com
    
******************************************************************/


/* Edit History

01/18/91   3 hamilton	#if not rescanned
01/12/91   2 schulert	conditionally use prototypes
12/13/90   1 schulert	add ifdef __cplusplus
11/03/90   2 hamilton	Alphalpha->Alfalfa & OmegaMail->Poste
08/10/90   1 nazgul	Initial version
*/


#include <stdio.h>

#ifdef USENLS
#  ifdef I_STDARG
extern int     	MCprintf(char *fmt, ...);
extern int     	MCfprintf(FILE *fptr, char *fmt, ...);
extern int     	MCsprintf(char *cptr, char *fmt, ...);
#  else
extern int     	MCprintf();
extern int     	MCfprintf();
extern int     	MCsprintf();
#  endif
#endif

#endif
