/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: mcdreg.h,v 1.3 1993/01/21 20:08:41 karels Exp $
 */

/*
 * Definitions for the Mitsumi CD-ROM model CRMC
 */

/* io port offsets */
#define	MCD_DATA	0
#define	MCD_FLAGS	1	/* note: active low port- invert when reading */
#define MCD_HCON	2
#define MCD_CHN		3

/* 
 * The manual seems to say that a port at base+2 is used, but it doesn't
 * say how.  Claiming that we use 4 ports should be safe.
 */
#define	MCD_NPORT 4

#define	mcd_get_flags(base) (inb((base)+MCD_FLAGS) ^ 0xff)

/* bits in MCD_FLAGS */
#define	MCD_DATA_AVAIL		2
#define	MCD_STATUS_AVAIL	4

/* opcodes */
#define	MCD_SOFT_RESET		0x60
#define	MCD_DRIVE_CONFIGURATION	0x90
#define	MCD_REQUEST_TOC_DATA	0x10
#define	MCD_SEEK_AND_READ	0xc0
#define	MCD_REQUEST_STATUS	0x40
#define MCD_HOLD		0x70
#define MCD_REQUEST_VERSION	0xdc
#define MCD_SET_MODE		0xa0

/* bits in status byte */
#define	MCD_STATUS_DOOR_OPEN	0x80
#define	MCD_STATUS_DISK_CHECK	0x40
#define	MCD_STATUS_DISK_CHANGED	0x20
#define	MCD_STATUS_SERVO_CHECK	0x10
#define	MCD_STATUS_DISK_TYPE	0x08
#define	MCD_STATUS_READ_ERROR	0x04
#define	MCD_STATUS_AUDIO_BUSY	0x02
#define	MCD_STATUS_COMMAND_CHECK 0x01

