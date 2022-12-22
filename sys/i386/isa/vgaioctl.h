/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: vgaioctl.h,v 1.2 1992/01/04 18:45:26 kolstad Exp $
 */

#define VGAIOCMAP	_IOWR('G', 5, int)	/* map in framebuffer */
#define VGAIOCUNMAP	_IOW('G', 6, int)	/* unmap framebuffer */
