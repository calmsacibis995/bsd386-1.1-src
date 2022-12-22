/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Array of 64 bytes used for encoding binary numbers 6 bits at a time.
**
**	Chosen bytes must be in lexicograhic order.
**
**	RCSID $Id: ValidChars.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: ValidChars.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"


	/* Old */
char	ValidOChars[]	= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	/* New */
char	ValidNChars[]	= "0123456789=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

char *	ValidChars;	/* Depends on ORIG_UUCP */
int	VC_size;	/*	"	"	*/
int	SEQLEN;		/* Bytes in sequence field */
Ulong	MaxSeq;		/* Max number representable in SEQLEN */

#define	NEWSEQLEN	5
#define	NEWMAXSEQ	1073741824	/* 64**5 */
#define	OLDSEQLEN	4
#define	OLDMAXSEQ	14776336	/* 62**4 */


void
InitVC()
{
	if ( ORIG_UUCP )
	{
		SEQLEN = OLDSEQLEN;
		MaxSeq = OLDMAXSEQ;
		ValidChars = ValidOChars;
	}
	else
	{
		SEQLEN = NEWSEQLEN;
		MaxSeq = NEWMAXSEQ;
		ValidChars = ValidNChars;
	}

	VC_size = strlen(ValidChars);
}
