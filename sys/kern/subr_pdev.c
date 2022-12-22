/*-
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: subr_pdev.c,v 1.2 1994/01/29 02:03:52 karels Exp $
 */

#include "sl.h"
#if NSL > 0
int	nsl = NSL;		/* patchable */
#endif

#ifdef PPP
#include "appp.h"
#if NAPPP > 0
int	nappp = NAPPP;		/* patchable */
#endif
#endif

/*
 * Attach pseudo-devices.
 * This function should be implemented with a list constructed by config.
 */
pdev_init()
{

#if NSL > 0
	slattach(nsl);			/* XXX */
#endif
#if defined(PPP) && NAPPP > 0
	apppattach(nappp);		/* XXX */
#endif
#include "loop.h"
#if NLOOP > 0
	loattach(NLOOP);		/* XXX */
#endif
}
