/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Define structure to select operating parameters.
**
**	RCSID $Id: params.h,v 1.1.1.1 1992/09/28 20:08:34 trent Exp $
**
**	$Log: params.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:34  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

typedef struct
{
	char *	id;	/* Name of parameter */
	char **	val;	/* Where to put its value */
	short	type;	/* Type (below) */
	short	flag;	/* SET|NEW */
}
	ParTb;

#define	SET	01	/* Parameter has been set */
#define	NEW	02	/* Value has been malloc'd */

/**	A ParTb table is searched using `bsearch(3)' **/

#ifndef	ENUM_NOT_INT
enum {			/* Parameter is a ... */
	PSTRING,	/* string */
	PVECTOR,	/* vector of strings */
	PLONG,		/* long integer */
	PINT,		/* decimal integer */
	POCT,		/* octal integer */
	PHEX,		/* hexadecimal integer */
	PDIR,		/* string needing a '/' appended */
	PSPOOLD,	/* string needing SPOOLDIR pre-pended and a '/' appended */
	PPARAMD,	/* string needing PARAMSDIR pre-pended and a '/' appended */
	PSPOOL,		/* string needing SPOOLDIR pre-pended */
	PPARAM,		/* string needing PARAMSDIR pre-pended */
	PPROG,		/* string needing PROGDIR pre-pended */
};
#else	/* !ENUM_NOT_INT */

#define	PSTRING	0
#define	PVECTOR	1
#define	PLONG	2
#define	PINT	3
#define	PDIR	4
#define	PSPOOLD	5
#define	PPARAMD	6
#define	PSPOOL	7
#define	PPARAM	8
#define	PPROG	9
#define	POCT	10
#define	PHEX	11

#endif	/* !ENUM_NOT_INT */

/*
**	Externals.
*/

extern bool	CheckParams;
extern void	GetParams(char *, ParTb *, int);
extern bool	NewParamsFile;
extern ParTb	Params[];
extern bool	ParamsErr;
extern int	SizeofParams;
