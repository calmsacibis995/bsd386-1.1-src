#ifndef mcprtlib_h
#define mcprtlib_h



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

03/20/91   2 schulert	Ultrix cc has trouble with void*, so change them to int*
01/18/91   3 hamilton	#if not rescanned
01/12/91   1 schulert	conditionally use prototypes
			rework to use either varargs or stdarg
11/03/90   2 hamilton	Alphalpha->Alfalfa & OmegaMail->Poste
08/10/90   1 nazgul	Initial version
*/

/* taken from Xm/lib/VaSimple.h
   currently no one defines MISSING_STDARG_H */
 
#ifdef	I_STDARG
# include <stdarg.h>
# define Va_start(a,b) va_start(a,b)
#else
# include <varargs.h>
# define Va_start(a,b) va_start(a)
#endif

#define MCFree		0x0010			/* Reminder to MCPrintFree */
#define MCCatalog	0x0100			/* Probably came from catalog */
#define	MCFmtArg        (0x0001|MCCatalog)	/* Came from format string */
#define MCUserArg	(0x0012|MCFree)		/* Came from program (e.g. %s) */
#define MCDataArg	(0x0013|MCFree)		/* Came from data (e.g. %d) */
#define MCMsgArg	(0x0004|MCCatalog)	/* Came from message catalog */

#define MCShortType	1
#define MCLongType	2
#define MCIntType	3
#define MCLongFloatType	4
#define MCFloatType	5
#define MCStringType	6
#define MCVoidPType	7
#define MCShortPType	8
#define MCLongPType	9
#define MCIntPType	10

typedef struct {
    int		type;
    union {
	short		shortV;
	unsigned short	ushortV;
	long		longV;
	unsigned long	ulongV;
	int		intV;
	unsigned int	uintV;
#ifdef LongFloat
	long float	lfloatV;
#endif
	float		floatV;
	char		*charPV;
	int		*voidPV;
	short		*shortPV;
	long		*longPV;
	int		*intPV;
    } u;
} MCTypesT;

#define MCSpaceFlag	0x01
#define MCPlusFlag	0x02
#define MCMinusFlag	0x04
#define MCAltFlag	0x08
#define MCZeroFlag	0x10


typedef struct {
    int		pos;
    int		type;
    char	c;
    int		flag;
    int		hasWidth;
    int		widthPos;
    int		width;
    int		hasPrec;
    int		precPos;
    int		prec;
    char	*data;
    int		dataLen;
} MCArgumentT;

typedef struct {
    int		argType;
    char	*data;
    long	dataLen;
} MCReplyT;

typedef struct {
    MCArgumentT	*argList;
    int		argCnt;
    MCTypesT	*typeList;
    int		typeCnt;
    MCReplyT	*replyList;
    int		replyCnt;
} MCRockT;


#define	MCPrintGet(start,rock)						      \
{									      \
    va_list	vl;							      \
    int		i;							      \
    int 	typeCnt = rock->typeCnt;				      \
    MCTypesT	*typeList = rock->typeList;				      \
									      \
    Va_start(vl, start);						      \
    for (i = 0; i < typeCnt; ++i) {					      \
	switch (typeList[i].type) {					      \
	  case MCShortType:						      \
	    typeList[i].u.shortV = va_arg(vl, short);			      \
	    break;							      \
	  case MCLongType:						      \
	    typeList[i].u.longV = va_arg(vl, long);			      \
	    break;							      \
	  case MCIntType:						      \
	    typeList[i].u.intV = va_arg(vl, int);			      \
	    break;							      \
	  case MCLongFloatType:						      \
/*#ifdef LongFloat							      \
	    typeList[i].u.lfloatV = va_arg(vl, long float);		      \
	    break;							      \
#endif*/								      \
	  case MCFloatType:						      \
	    typeList[i].u.floatV = va_arg(vl, float);			      \
	    break;							      \
	  case MCStringType:						      \
	    typeList[i].u.charPV = va_arg(vl, char *);			      \
	    break;							      \
	  case MCVoidPType:						      \
	    typeList[i].u.voidPV = va_arg(vl, int *);			      \
	    break;							      \
	  case MCShortPType:						      \
	    typeList[i].u.shortPV = va_arg(vl, short *);		      \
	    break;							      \
	  case MCLongPType:						      \
	    typeList[i].u.longPV = va_arg(vl, long *);			      \
	    break;							      \
	  case MCIntPType:						      \
	    typeList[i].u.intPV = va_arg(vl, int *);			      \
	    break;							      \
	}								      \
    }									      \
    va_end(vl);								      \
}

extern MCRockT	*MCPrintInit();
extern int	MCPrintParse();
extern void	MCPrintFree();

#endif
