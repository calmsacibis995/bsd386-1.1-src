
static char rcsid[] = "@(#)Id: mcprt.c,v 5.5 1993/08/23 02:49:35 syd Exp ";

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
01/12/91   1 schulert	conditionally use prototypes
			rework to use either varargs or stdarg
11/03/90   2 hamilton	Alphalpha->Alfalfa & OmegaMail->Poste
08/10/90   1 nazgul	printf, sprintf and fprintf
*/


#include <stdio.h>
#include "defs.h"

#include "mcprt.h"
#include "mcprtlib.h"

#ifdef	I_STDARG
int	MCprintf(char *fmt, ...)
#else
int	MCprintf(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    MCRockT	*rock;
    int		len, i;
    
    if ((rock = MCPrintInit(fmt)) == NULL) return -1;
    MCPrintGet(fmt, rock);
    if ((len = MCPrintParse(rock)) < 0) return -1;

    for (i = 0; i < rock->replyCnt; ++i)
      fwrite(rock->replyList[i].data, 1, rock->replyList[i].dataLen, stdout);
    MCPrintFree(rock);

    return len;
}

#ifdef	I_STDARG
int	MCfprintf(FILE *fptr, char *fmt, ...)
#else
int	MCfprintf(fptr, fmt, va_alist)
FILE *fptr;
char *fmt;
va_dcl
#endif
{
    MCRockT	*rock;
    int		len, i;
    
    if ((rock = MCPrintInit(fmt)) == NULL) return -1;
    MCPrintGet(fmt, rock);
    if ((len = MCPrintParse(rock)) < 0) return -1;

    for (i = 0; i < rock->replyCnt; ++i)
      fwrite(rock->replyList[i].data, 1, rock->replyList[i].dataLen, fptr);
    MCPrintFree(rock);

    return len;
}

#ifdef	I_STDARG
int	MCsprintf(char *cptr, char *fmt, ...)
#else
int	MCsprintf(cptr, fmt, va_alist)
char *cptr;
char *fmt;
va_dcl
#endif
{
    MCRockT	*rock;
    int		len, i;
    
    if ((rock = MCPrintInit(fmt)) == NULL) return -1;
    MCPrintGet(fmt, rock);
    if ((len = MCPrintParse(rock)) < 0) return -1;

    for (i = 0; i < rock->replyCnt; ++i) {
	bcopy(rock->replyList[i].data, cptr, rock->replyList[i].dataLen);
	cptr += rock->replyList[i].dataLen;
    }
    *cptr = '\0';

    MCPrintFree(rock);

    return len;
}
