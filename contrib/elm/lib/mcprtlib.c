
static char rcsid[] = "@(#)Id: mcprtlib.c,v 5.5 1993/08/23 02:54:31 syd Exp ";

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

03/25/91   4 hamilton	Handle null data in MCPrintFree
01/18/91   3 hamilton	#if not rescanned
01/12/91   2 schulert	conditionally use prototypes
01/06/91   1 schulert	check for NULL char* before calling strlen()
11/03/90   2 hamilton	Alphalpha->Alfalfa & OmegaMail->Poste
08/13/90   1 hamilton	Add <ctype.h> for sco
08/10/90   2 nazgul	Added support for %S and %M
08/10/90   1 nazgul	Supports %d and %s
*/

#define	CATGETS

#include <stdio.h>
#include "defs.h"

#include "mcprtlib.h"
#ifdef CATGETS
# include "nl_types.h"
# ifndef MCMakeId
#  define MCMakeId(s,m)	(unsigned long)(((unsigned short)s<<(sizeof(short)*8))\
					|(unsigned short)m)
#  define MCSetId(id)	(unsigned int) (id >> (sizeof(short) * 8))
#  define MCMsgId(id)	(unsigned int) ((id << (sizeof(short) * 8))\
					>> (sizeof(short) * 8))
# endif
#endif

#ifndef True
# define True	~0
# define False	0
#endif

#define MCShortMod	0x01
#define MCLongMod	0x02
#define MCLongFMod	0x04


#define	GETNUM(x)	x = 0; \
                        while (isdigit(*fptr)) x = (x * 10) + *fptr++ - '0'; \
                        if (!*fptr) goto err;
    
MCRockT	*MCPrintInit(fmt)
char *fmt;
{
    int		argCnt, curPos, typeCnt, done, mod, replyCnt, i, pos;
    char	*fptr, *cptr;
    MCArgumentT	*argList, arg;
    MCRockT	*rock;
    MCTypesT	*typeList;
    
    /* This can count too many, but that's okay */
    for (argCnt = 0, fptr = fmt; *fptr; ++fptr) {
	if (*fptr == '%') ++argCnt;
    }
    ++argCnt;	/* One for the end */
    
    rock = (MCRockT *) malloc(sizeof(MCRockT));
    bzero((char *) rock, sizeof(*rock));
    
    rock->argList = argList = (MCArgumentT *) malloc(sizeof(MCArgumentT) * argCnt);
    
    bzero((char *) &arg, sizeof(arg));
    arg.data = fmt;
    arg.dataLen = 0;
    argCnt = 0;
    curPos = 1;
    typeCnt = 0;
    for (fptr = fmt; *fptr; ++fptr) {
	if (*fptr == '%') {
	    arg.dataLen = fptr - arg.data;
	    arg.pos = 0;
	    ++fptr;
	    
	    /* Check for position */

	    if (isdigit(*fptr)) {
		for (cptr = fptr; isdigit(*cptr); ++cptr);
		if (*cptr == '$') {
		    GETNUM(arg.pos);
		    ++fptr;
		}
	    }
	    
	    /* Check for flags (%   + - # 0) */
	    
	    arg.flag = 0;
	    done = False;
	    while (!done) {
		switch (*fptr) {
		  case ' ':
		    arg.flag |= MCSpaceFlag;
		    break;
		  case '+':
		    arg.flag |= MCPlusFlag;
		    break;
		  case '-':
		    arg.flag |= MCMinusFlag;
		    break;
		  case '#':
		    arg.flag |= MCAltFlag;
		    break;
		  case '0':
		    arg.flag |= MCZeroFlag;
		    break;
		  default:
		    done = True;
		    break;
		}
		++fptr;
	    }
	    --fptr;
	    
	    /* Check the width argument */
	    
	    arg.hasWidth = False;
	    arg.widthPos = 0;
	    if (isdigit(*fptr)) {			/* width */
		arg.hasWidth = True;
		GETNUM(arg.width);
	    } else if (*fptr == '*') {
		arg.hasWidth = True;
		++fptr;
		if (isdigit(*fptr)) {		/* reference to width pos */
		    GETNUM(arg.widthPos);
		    ++fptr;
		} else {
		    arg.widthPos = curPos++;
		}
	    }
	    
	    /* Check for precision argument */
	    
	    arg.hasPrec = False;
	    arg.precPos = 0;
	    if (*fptr == '.') {
		++fptr;
		if (isdigit(*fptr)) {		/* precision */
		    arg.hasPrec = True;
		    GETNUM(arg.prec);
		} else if (*fptr == '*') {
		    arg.hasPrec = True;
		    ++fptr;
		    if (isdigit(*fptr)) {		/* reference to prec pos */
			GETNUM(arg.precPos);
			++fptr;
		    } else {
			arg.precPos = curPos++;
		    }
		}
	    }
	    
	    /* Check for size modifier */
	    
	    for (mod = 0, done = False; !done; ++fptr) {
		switch (*fptr) {
		  case 'h':
		    mod |= MCShortMod;
		    break;
		  case 'l':
		    mod |= MCLongMod;
		    break;
		  case 'L':
		    mod |= MCLongFMod;
		    break;
		  default:
		    done = True;
		    --fptr;
		    break;
		}
	    }
	    
	    /* Check for the real thing */
	    
	    arg.c = *fptr;
	    arg.type = 0;
	    switch (*fptr) {
#ifdef CATGETS
	      case 'M': case 'S':
#endif
	      case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
	      case 'c':
		if (mod & MCShortMod) arg.type = MCShortType;
		else if (mod & MCLongMod) arg.type = MCLongType;
		else arg.type = MCIntType;
		break;
	      case 'f': case 'e': case 'E': case 'g': case 'G':
		if (mod & MCLongFMod) arg.type = MCLongFloatType;
		else arg.type = MCFloatType;
		break;
	      case 's':
		arg.type = MCStringType;
		break;
#ifdef CATGETS
	      case 'C':
#endif		
	      case 'p':
		arg.type = MCVoidPType;
		break;
	      case 'n':
		if (mod & MCShortMod) arg.type = MCShortPType;
		else if (mod & MCLongMod) arg.type = MCLongPType;
		else arg.type = MCIntPType;
		break;
	      case '%':
		++arg.dataLen;	/* An empty arg with a data element including % */
		break;
	      default:
		goto err;
		break;
	    }
	    
	    /* They should never mix with and without positions, but be nice */
	    if (!arg.pos) arg.pos = curPos++;
	    else curPos = arg.pos+1;
	    
	    /* Keep track of how many type elements we'll need */
	    if (arg.pos > typeCnt) typeCnt = arg.pos;
	    if (arg.precPos > typeCnt) typeCnt = arg.precPos;
	    if (arg.widthPos > typeCnt) typeCnt = arg.widthPos;
	    
	    argList[argCnt++] = arg;
	    bzero((char *) &arg, sizeof(arg));
	    arg.data = fptr+1;
	} else {
	    /* Otherwise things will just end up in arg.data */
	}
    }
    /* Catch any trailing text */
    arg.dataLen = fptr - arg.data;
    argList[argCnt++] = arg;
    
    /* 
     * We now have an array which precisely describes all of the arguments.
     * Now we allocate space for each of the arguments.
     * Then we loop through that array and fill it in with the correct arguments.
     * Finally we loop through the arglist and print out each of the arguments.
     * Simple, no?
     */
    
    /* Allocate the type list */
    
    rock->typeList = typeList = (MCTypesT *) malloc(sizeof(MCTypesT) * typeCnt);
    
    /* Go through and set the types */
    
    /*
     * I was going to check here and see if, when a type was referenced twice,
     * they typed it differently, and return an error when they did.  But then
     * it occured to me that doing that could be useful.  Consider passing a
     * pointer and wanting to see it in both string and hex form.  If they think
     * they know what they're doing we might as well let them do it.
     */
    
    /*
     * Set the correct types and figure out how many data segments we are going
     * to have.
     */
    /* Initialize typeList */
    for (i = 0; i < typeCnt; i++)
      typeList[i].type = 0;

    for (replyCnt = i = 0; i < argCnt; ++i) {
	if (argList[i].type) {
	    pos = argList[i].pos-1;
	    typeList[pos].type = argList[i].type;
	    ++replyCnt;
	    if (argList[i].precPos) typeList[argList[i].precPos-1].type = MCIntType;
	    if (argList[i].widthPos) typeList[argList[i].widthPos-1].type = MCIntType;
	}
	if (argList[i].dataLen) ++replyCnt;
    }
    ++replyCnt;	/* with NULLs */

    rock->replyCnt = replyCnt;
    rock->argCnt = argCnt;
    rock->typeCnt = typeCnt;
    rock->replyList = NULL;
    return(rock);
err:
    MCPrintFree(rock);
    return(NULL);
}

int	MCPrintParse(rock)
MCRockT *rock;
{
    MCArgumentT	*argList = rock->argList, arg;
    int		argCnt = rock->argCnt;
    MCTypesT	*typeList = rock->typeList, type;
    int		typeCnt = rock->typeCnt;
    MCReplyT	*replyList;
    int		replyCnt = rock->replyCnt;

    int		i, pos, ri, j, base, isSigned, isNeg, len, alen, retlen = 0;
    char	*index, *cptr, buf[BUFSIZ], *fptr, charbuf[2];
#ifdef CATGETS
    int		setId = 0, msgId = 0;
    nl_catd	catd;
#endif
    
    
    /* Allocate the reply structure */
    rock->replyList = replyList = (MCReplyT *) malloc(sizeof(MCReplyT) * replyCnt);
    
    /* Now we do the dirty work of actually formatting the stuff */

    for (ri = i = 0; i < argCnt; ++i) {
	pos = argList[i].pos-1;

	/* Handle the data from the format string */
	if (argList[i].dataLen) {
	    replyList[ri].argType = MCFmtArg;
	    replyList[ri].data = argList[i].data;
	    retlen += replyList[ri].dataLen = argList[i].dataLen;
	    ++ri;
	}

	/* Fill in the indirect width and precision stuff */
	if (argList[i].hasPrec && argList[i].precPos) {
	    argList[i].prec = typeList[argList[i].precPos-1].u.intV;
	}
	if (argList[i].hasWidth && argList[i].widthPos) {
	    argList[i].width = typeList[argList[i].widthPos-1].u.intV;
	}
	
	if (argList[i].type) {
	    type = typeList[pos];
	    arg = argList[i];
	    if (arg.type == MCShortType || arg.type == MCLongType ||
		arg.type == MCIntType) {
intType:	index = "0123456789abcdef";
		switch (arg.c) {
		  case 'd': case 'i':
		    base = 10;
		    isSigned = True;
		    break;
		  case 'u':
		    base = 10;
		    isSigned = False;
		    break;
		  case 'o':
		    base = 8;
		    isSigned = False;
		    break;
		  case 'X':
		    index = "0123456789ABCDEF";
		  case 'x':
		    base = 16;
		    isSigned = False;
		    break;
		  case 'c':
		    if (arg.type == MCShortType)
			charbuf[0] = (char) type.u.ushortV;
		    else if (arg.type == MCLongType)
			charbuf[0] = (char) type.u.ulongV;
		    else
			charbuf[0] = (char) type.u.uintV;
		    charbuf[1] = '\0';
		    type.u.charPV = charbuf;
		    goto strType;
#ifdef CATGETS
		  case 'S':
		    setId = type.u.intV;
		    goto nextArg;
		  case 'M':
		    if (arg.type == MCLongType) {
			setId = MCSetId(type.u.longV);
			msgId = MCMsgId(type.u.longV);
		    } else msgId = type.u.intV;
		    type.u.charPV = catgets(catd, setId, msgId, "<unable to load msg>");
		    goto strType;
#endif
		  default:
		    goto err;
		}
		if (base == -1) {
		}
		
		cptr = buf;
		isNeg = False;
		switch (arg.type) {
		  case MCShortType:
		    if (!type.u.shortV) break;
		    if (isSigned) {
			if (type.u.shortV < 0) {
			    isNeg = True;
			    type.u.shortV = -type.u.shortV;
			}
			while (type.u.shortV) {
			    *cptr++ = index[type.u.shortV % base];
			    type.u.shortV /= 10;
			}
		    } else {
			while (type.u.ushortV) {
			    *cptr++ = index[type.u.ushortV % base];
			    type.u.ushortV /= 10;
			}
		    }
		    break;
		  case MCLongType:
		    if (!type.u.longV) break;
			if (isSigned) {
			    if (type.u.longV < 0) {
			    isNeg = True;
			    type.u.longV = -type.u.longV;
			}
			while (type.u.longV) {
			    *cptr++ = index[type.u.longV % base];
			    type.u.longV /= 10;
			}
		    } else {
			while (type.u.ulongV) {
			    *cptr++ = index[type.u.ulongV % base];
			    type.u.ulongV /= 10;
			}
		    }
		    break;
		  case MCIntType:
		    if (!type.u.intV) break;
		    if (isSigned) {
			if (type.u.intV < 0) {
			    isNeg = True;
			    type.u.intV = -type.u.intV;
			}
			while (type.u.intV) {
			    *cptr++ = index[type.u.intV % base];
			    type.u.intV /= 10;
			}
		    } else {
			while (type.u.uintV) {
			    *cptr++ = index[type.u.uintV % base];
			    type.u.uintV /= 10;
			}
		    }
		    break;
		}

		/* precision */
		if (cptr == buf && !arg.hasPrec) *cptr++ = '0';
		if (arg.hasPrec && cptr-buf < arg.prec) {
		    for (j = cptr-buf; j < arg.prec; ++j) *cptr++ = '0';
		}

		/* zero width padding */
		if ((arg.flag & MCZeroFlag) && arg.hasWidth && !(arg.flag & MCMinusFlag)) {
		    for (j = cptr-buf; j < arg.width; ++j) *cptr++ = '0';
		    if (isNeg || (arg.flag & MCPlusFlag) || (arg.flag & MCSpaceFlag)) --cptr;
		}
		
		/* signs */
		if (isNeg) *cptr++ = '-';
		else if (arg.flag & MCPlusFlag) *cptr++ = '+';
		else if (arg.flag & MCSpaceFlag) *cptr++ = ' ';

		/* alternate forms */
		if (arg.flag & MCAltFlag) {
		    if (arg.c == 'x') *cptr++ = 'x';
		    else if (arg.c == 'X') *cptr++ = 'X';
		    else if (arg.c != 'o') *cptr++ = '#';	/* Undefined */
		    *cptr++ = '0';
		}
		
		/* get the storage space */
		if (arg.hasWidth && arg.width > cptr-buf) len = arg.width;
		else len = cptr-buf;
		
		replyList[ri].argType = MCDataArg;
		fptr = replyList[ri].data = (char *) malloc(len);
		replyList[ri].dataLen = len;

		if (arg.hasWidth && arg.width > cptr-buf) {
		    if (arg.flag & MCMinusFlag) {
			/* pad the end */
			fptr += len;
			for (j = cptr-buf; j < arg.width; ++j) *--fptr = ' ';
			fptr = replyList[ri].data;
		    } else {
			/* pad the beginning */
			for (j = cptr-buf; j < arg.width; ++j) *fptr++ = ' ';
		    }
		}
		for (j = cptr-buf; j > 0; --j) {
		    *fptr++ = *--cptr;
		}
		++ri;
	    } else if (arg.type == MCStringType) {
strType:
		if (arg.hasPrec) len = arg.prec;
		else len = type.u.charPV?strlen(type.u.charPV):0;
		if (arg.hasWidth && arg.width > len) alen = arg.width;
		else alen = len;
		
		replyList[ri].argType = MCUserArg;
		fptr = replyList[ri].data = (char *) malloc(alen);
		replyList[ri].dataLen = alen;
		
		if (len < alen) {
		    if (arg.flag & MCMinusFlag) {
			fptr += alen;
			for (j = len; j < alen; ++j) *--fptr = ' ';
			fptr = replyList[ri].data;
		    } else {
			/* pad the beginning */
			for (j = len; j < alen; ++j) *fptr++ = ' ';
		    }
		}
		bcopy(type.u.charPV, fptr, len);
		++ri;
	    } else if (arg.type == MCVoidPType) {
#ifdef CATGETS
		if (arg.c == 'C') {
		    catd = (nl_catd) type.u.voidPV;
		    goto nextArg;
		}
#endif
		arg.c = 'X';
		arg.flag |= MCAltFlag;
		goto intType;
	    } else {
		/* MCLongFloatType  MCFloatType */
		/* MCShortPType MCLongPType MCIntPType */
		goto err;
	    }
	    retlen += replyList[ri-1].dataLen;
	}
nextArg:;	/* Used for arguments with no output */
    }
    replyList[ri].argType = 0;
    replyList[ri].data = NULL;
    replyList[ri].dataLen = 0;
    rock->replyCnt = ri;
    
    return(retlen);
err:
    MCPrintFree(rock);
    return(-1);
}    
	    
void MCPrintFree(rock)
MCRockT *rock;
{
    int		i;
    
    if (!rock) return;
    
    if (rock->argList) free((char *) rock->argList);
    if (rock->typeList) free((char *) rock->typeList);
    
    if (rock->replyList) {
	for (i = 0; i < rock->replyCnt; ++i) {
	    if ((rock->replyList[i].argType & MCFree) && rock->replyList[i].data)
	      free(rock->replyList[i].data);
	}
	free((char *) rock->replyList);
    }
    free((char *) rock);
}
