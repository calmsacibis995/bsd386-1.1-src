/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Declare all optional device routines.
**
**	RCSID $Id: devices.h,v 1.2 1993/02/28 15:36:34 pace Exp $
**
**	$Log: devices.h,v $
 * Revision 1.2  1993/02/28  15:36:34  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.1.1.1  1992/09/28  20:08:55  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

extern CallType	nullcls(int), nullopn(char *, char **, Device *), nodev(void);
extern CallType	diropn(char **), dircls(int);

#ifdef	DATAKIT
extern CallType	dkopn(char **);
#endif	/* DATAKIT */

#ifdef	DN11
extern CallType	dnopn(char **), dncls(int);
#endif	/* DN11 */

#ifdef	HAYES
extern CallType	hyspopn(char *, char **, Device *), hystopn(char *, char **, Device *), hyscls(int);
#endif	/* HAYES */

#ifdef	HAYES2400
extern CallType	hyspopn24(char *, char **, Device *), hystopn24(char *, char **, Device *), hyscls24(int);
#endif	/* HAYES2400 */

#ifdef	HAYESQ
extern CallType	hysqopn(char *, char **, Device *), hysqcls(int);  /* a version of hayes that doesn't use ret codes */
#endif	/* HAYESQ */

#ifdef	HAYESV
/* a version that is very aggressive about getting stupid and half-broken
 * modems to go off hook even though they need their roms upgraded */
extern CallType hysvpopn(char *, char **, Device *),
		hysvtopn(char *, char **, Device *),
		hysvcls(int);
#endif	/* HAYESV */

#ifdef	NOVATION
extern CallType	novopn(char *, char **, Device *), novcls(int);
#endif	/* NOVATION */

#ifdef	CDS224
extern CallType	cdsopn224(char *, char **, Device *), cdscls224(int);
#endif	/* CDs224 */

#ifdef	DF02
extern CallType	df2opn(char *, char **, Device *), df2cls(int);
#endif	/* DF02 */

#ifdef	DF112
extern CallType	df12popn(char *, char **, Device *), df12topn(char *, char **, Device *), df12cls(int);
#endif	/* DF112 */

#ifdef	PNET
extern CallType	pnetopn(char *, char **, Device *);
#endif	/* PNET */

#ifdef	VENTEL
extern CallType	ventopn(char *, char **, Device *), ventcls(int);
#endif	/* VENTEL */

#ifdef	PENRIL
extern CallType	penopn(char *, char **, Device *), pencls(int);
#endif	/* PENRIL */

#ifdef	UNETTCP
#define TO_ACTIVE	0
extern CallType	unetopn(char *, char **, Device *), unetcls(int);
#endif	/* UNETTCP */

#ifdef	BSDTCP
extern CallType	bsdtcpopn(char **), bsdtcpcls(int);
#endif	/* BSDTCP */

#ifdef	VADIC
extern CallType	vadopn(char *, char **, Device *), vadcls(int);
#endif	/* VADIC */

#ifdef	VA212
extern CallType	va212opn(char *, char **, Device *), va212cls(int);
#endif	/* VA212 */

#ifdef	VA811S
extern CallType	va811opn(char *, char **, Device *), va811cls(int);
#endif	/* VA811S */

#ifdef	VA820
extern CallType	va820opn(char *, char **, Device *), va820cls(int);
#endif	/* VA820 */

#ifdef	RVMACS
extern CallType	rvmacsopn(char *, char **, Device *), rvmacscls(int);
#endif	/* RVMACS */

#ifdef	VMACS
extern CallType	vmacsopn(char *, char **, Device *), vmacscls(int);
#endif	/* VMACS */

#ifdef	MICOM
extern CallType	micopn(char *, char **, Device *), miccls(int);
#endif	/* MICOM */

#ifdef	SYTEK
extern CallType	sykopn(char *, char **, Device *), sykcls(int);
#endif	/* SYTEK */

#ifdef	ATT2224
extern CallType	attopn(char *, char **, Device *), attcls(int);
#endif	/* ATT2224 */
