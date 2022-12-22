/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: diskstats.h,v 1.1 1992/08/20 18:32:58 karels Exp $
 */

#ifndef _DISKSTATS_H_
#define _DISKSTATS_H_

/* 
 * Per-disk statistics
 */
struct diskstats {
	u_long	dk_seek;	/* explicit seeks */
	u_long	dk_sectors;	/* sectors transfered */
	u_long	dk_xfers;	/* disk transfers */
	u_long	dk_busy;	/* drive used? used in gatherstats() */
	u_long	dk_time;	/* time busy, accumulated by gatherstats() */
	u_long	dk_bpms;	/* bytes/ms (a precalcuated constant) */
	u_long	dk_secsize;	/* units for dk_sectors */
};

#endif /* _DISKSTATS_H_ */
