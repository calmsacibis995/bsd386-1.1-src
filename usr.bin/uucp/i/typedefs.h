/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Type definitions for C programming.
**
**	RCSID $Id: typedefs.h,v 1.1.1.1 1992/09/28 20:08:34 trent Exp $
**
**	$Log: typedefs.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:34  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include <sys/types.h>

typedef unsigned char	Uchar;
typedef unsigned int	Uint;
typedef unsigned long	Ulong;
typedef unsigned short	Ushort;

#ifdef	__GNUC__
typedef int		(*Funcp)();
#else	/* __GNUC__ */
typedef int		(*Funcp)(...);
#endif	/* __GNUC__ */

#ifdef	ENUM_NOT_INT

typedef int		bool;
#define	false		0
#define	true		1

typedef int		Wait;
#define	WAIT		1
#define	NOWAIT		0

#else	/* ENUM_NOT_INT */

typedef enum { false = 0, true = 1 } bool;
typedef enum { NOWAIT, WAIT } Wait;

#endif	/* ENUM_NOT_INT */

#ifdef	NO_VOID_STAR
#ifdef	cray
typedef int *		Pointer;
#else	/* cray */
typedef char *		Pointer;
#endif	/* cray */
typedef int		(*vFuncp)();
#else	/* NO_VOID_STAR */
typedef void *		Pointer;
#ifdef	__GNUC__
typedef void		(*vFuncp)();
#else	/* __GNUC__ */
typedef void		(*vFuncp)(...);
#endif	/* __GNUC__ */
#endif	/* NO_VOID_STAR */

typedef time_t		Time_t;

typedef struct
{
	Time_t	secs;
	Time_t	usecs;
}
	TimeBuf;

#define	Time		TimeNow.secs
#define	Timeusec	TimeNow.usecs

/*
**	Duration macros.
*/

#define	MINUTE		60L
#define	HOUR		(60*MINUTE)
#define	DAY		(24*HOUR)
#define	WEEK		(7*DAY)
#define	MONTH		(4*WEEK)
#define	YEAR		(365*DAY)
