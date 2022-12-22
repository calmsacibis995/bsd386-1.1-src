/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: pcconsioctl.h,v 1.6 1993/02/21 21:04:02 karels Exp $
 */

/*
 * Structure used in mapping/unmapping ranges of I/O ports to user processes.
 */
struct iorange {
	int  iobase;		/* base ioport */
	int  cnt;		/* number of ports */
};

#define	PCCONIOCRAW	_IO('G', 1)		/* turn kbd raw mode on */
#define	PCCONIOCCOOK	_IO('G', 2)		/* turn kbd raw mode off */
#define PCCONIOCSETLED	_IOW('G', 3, char)	/* set keyboard leds */
#define	PCCONIOCBEEP	_IO('G', 4)		/* kbd beep */
#define	PCCONIOCSTARTBEEP	_IOW('G', 5, short)	/* start beep */
#define	PCCONIOCSTOPBEEP	_IO('G', 6)		/* stop beep */

#define	PCCONIOCMAPPORT	_IOW('G', 7, struct iorange)	/* enable io ports */
#define	PCCONIOCUNMAPPORT _IOW('G', 8, struct iorange)	/* disable io ports */
#define	PCCONENABIOPL _IO('G', 9)	/* enable IOPL privileges */
#define	PCCONDISABIOPL _IO('G', 10)	/* disable IOPL privileges */

#define LED_SCR			0x01
#define	LED_NUM			0x02
#define LED_CAP			0x04
