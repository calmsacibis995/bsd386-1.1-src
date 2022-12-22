
/*	@(#)param.h 1.1 86/07/07 SMI; from UCB 4.35 83/06/10	*/
    
/*
** Modified version distributed with 'dig' version 2.0 from 
** University of Southern California Information Sciences Institute
** (USC-ISI). 9/1/90
*/

#ifndef __PARAM_HEADER__
#define __PARAM_HEADER__

/*
 * Fundamental constants of the implementation.
 */
#define	NBBY	8		/* number of bits in a byte */
#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#define	NULL	0
#define	CMASK	0		/* default mask for file creation */

#include	<sys/types.h>

/*
 * bit map related macros
 */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/*
 * Macros for fast min/max.
 */
#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif


#endif !__PARAM_HEADER__
