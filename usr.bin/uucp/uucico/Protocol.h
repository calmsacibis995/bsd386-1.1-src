/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Protocol selection.
**
**	RCSID $Id: Protocol.h,v 1.1.1.1 1992/09/28 20:08:56 trent Exp $
**
**	$Log: Protocol.h,v $
 * Revision 1.1.1.1  1992/09/28  20:08:56  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

typedef struct Proto
{
	char		P_id;
	CallType	(*P_turnon)(void);
	CallType	(*P_rdmsg)(char *, int);
	CallType	(*P_wrmsg)(char, char *, int);
	CallType	(*P_rddata)(int, FILE *);
	CallType	(*P_wrdata)(FILE *, int);
	CallType	(*P_turnoff)(void);
}
		Proto;

extern char	Protocol;

extern CallType	Gturnon(void), gturnon(void), gturnoff(void);
extern CallType	grdmsg(char *, int), grddata(int, FILE *);
extern CallType	gwrmsg(char, char *, int), gwrdata(FILE *, int);

extern CallType	twrmsg(char, char *, int), trdmsg(char *, int);
extern CallType	twrdata(FILE *, int), trddata(int, FILE *);

#ifdef	X25_PAD
extern CallType	fturnon(void), fturnoff(void);
extern CallType	frdmsg(char *, int), frddata(int, FILE *);
extern CallType	fwrmsg(char, char *, int), fwrdata(FILE *, int);
#endif	/* X25_PAD */
