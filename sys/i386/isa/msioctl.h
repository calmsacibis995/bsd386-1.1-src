/*	BSDI $Id: msioctl.h,v 1.2 1993/03/17 16:35:45 karels Exp $	*/

/*
 * Maxpeed SS-4, SS-8 and SS-16 ioctl definitions.
 *
 * Copyright (c) 1992, Douglas L. Urner.  All rights reserved.
 *
 * from Id: msreg.h,v 1.0 1992/11/22 16:34:01 dlu Exp
 */

#ifdef MSDEBUG
#define	MSIOSDEBUG	_IOW('M', 0, int)
#endif
#ifdef MSDIAG
#define	MSIOSDIAG	_IOW('M', 1, int)
#endif
#define	MSIODEFAULT _IO('M', 2)

#define MSMAXSIG    64
#define MSIOSIG     _IOR('M', 3, char[MSMAXSIG])

#ifdef MSDEBUG
#define	MSIORESET	_IOW('M', 4, int)
#define	MSIOCHRESET	_IOW('M', 5, int)
#endif

#define MSIOGFLAGS	_IOR('M', 6, int)
#define MSIOSFLAGS	_IOW('M', 7, int)

#define	MSIOSTEAL	_IOW('M', 10, int)

#define	MSIODONOR	_IOWR('M', 11, int)
#define	MSIONPORTS	_IOR('M', 12, int)
#define	MSIORXQSZ	_IOR('M', 13, int)
#define	MSIOTXQSZ	_IOR('M', 14, int)

#define	MSIOGFREQ	_IOR('M', 15, int)
#define	MSIOSFREQ	_IOW('M', 16, int)
#define	MSIOGLOWAT	_IOR('M', 17, int)
#define	MSIOSLOWAT	_IOW('M', 18, int)

#ifdef MS_KGDB
#define	MSIOSKGDB	_IOW('M', 20, int)
#define	MSIOCKGDB	_IOW('M', 21, int)
#endif
