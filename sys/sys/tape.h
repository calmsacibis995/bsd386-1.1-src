/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: tape.h,v 1.5 1992/09/22 20:01:26 karels Exp $
 */

/*
 * Common definitions for tape devices.
 */
struct tpdevice {
	struct	device tp_dev;
	struct	tpdriver *tp_driver;
	u_long	tp_reclen;
	u_long	tp_fileno;
	u_short	tp_flags;
	tpr_t	tp_ctty;
};

struct tpdriver {
	void	(*t_strategy) __P((struct buf *));
#ifdef notyet
	int	(*t_open) __P((dev_t, int, int, struct proc *));
	int	(*t_close) __P((dev_t, int, int, struct proc *));
	int	(*t_ioctl) __P((dev_t, int, caddr_t, int, struct proc *));
	int	(*t_dump) __P((int));
#endif
};

/*
 * Flags
 */
#define	TP_OPEN		0x0001	/* block multiple opens */
#define	TP_MOVED	0x0002	/* we're past BOT */
#define	TP_WRITING	0x0004	/* do we need to write filemarks on close? */
#define	TP_STREAMER	0x0008	/* use fixed length records */
#define	TP_DOUBLEEOF	0x0010	/* write two filemarks at end of tape */
#define	TP_SHORTEOF	0x0020	/* use 'short' filemarks (exabyte) */
#define	TP_SEENEOF	0x0040	/* report EOF on the next read */
#define	TP_8MMVIDEO	0x0080	/* special handling for 8mm videotapes */
#define	TP_NODENS	0x0100  /* drive with no density selection */
#define	TP_PHYS_ADDR	0x0200	/* drive can retrieve physical block address */

#define	tpunit(dev)	((dev) & 0x3)
#define	tpnorew(dev)	((dev) & 0x4)
