/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sascsi.h,v 1.3 1992/10/25 22:07:54 karels Exp $
 */

struct scsi_driver {
	int	(*s_attach) __P((struct iob *));
	int	(*s_start) __P((struct iob *, int,
		    int (*) (struct iob *, int, struct scsi_cdb *),
		    struct scsi_cdb *));
	int	(*s_go) __P((struct iob *));
};

extern const char scsicmdlen[];

extern struct scsi_driver *scsi_driver[];

#define	SD_MAJORDEV		4

/* XXX to help ahareg.h */
struct device;
typedef void (*scintr_fn) __P((struct device *, int stat, int resid));

/* XXX to help eisa.h */
struct isa_attach_args;
struct cfdata;
