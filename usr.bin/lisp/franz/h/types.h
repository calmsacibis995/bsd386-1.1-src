/*					-[Sat Jan 29 14:01:58 1983 by jkf]-
 * 	types.h				$Locker:  $
 * Unix standard type definitions
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/types.h,v 1.2 1992/01/04 18:56:51 kolstad Exp $
 *
 * (c) copyright 1982, Regents of the University of California
 */

typedef	struct { int rrr[1]; } *	physadr;
typedef	long		daddr_t;
typedef char *		caddr_t;
typedef	unsigned short	ino_t;
typedef	long		time_t;
typedef	int		label_t[10];
typedef	short		dev_t;
typedef	long		off_t;
# ifdef UNIXTS
typedef unsigned short ushort;
# endif
/* major part of a device */
#define	major(x)	(int)(((unsigned)x>>8)&0377)

/* minor part of a device */
#define	minor(x)	(int)(x&0377)

/* make a device number */
#define	makedev(x,y)	(dev_t)(((x)<<8) | (y))
