/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: device.c,v 1.1 1994/01/14 23:26:01 polk Exp $ */
#include "doscmd.h"

#define	MAXDEVICE	0x3F

/*
 * configuration table for device initialization routines
 */

struct devinitsw {
	void		(*p_devinit)();
} devinitsw[MAXDEVICE + 1];

static int devinitsw_ptr;

void null_devinit_handler(void) { }

void init_devinit_handlers(void)
{
	int i;

	for (i = 0; i <= MAXDEVICE; i++)
		devinitsw[i].p_devinit = null_devinit_handler;
	devinitsw_ptr = 0;
}

void define_devinit_handler(void (*p_devinit)())
{
	if (devinitsw_ptr < MAXDEVICE)
		devinitsw[devinitsw_ptr++].p_devinit = p_devinit;
	else
		fprintf (stderr, "attempt to install more than %d devices",
			MAXDEVICE);
}

init_optional_devices(void)
{
	int i;

	if (devinitsw_ptr > 0)
		for (i = 0; i < devinitsw_ptr; i++)
			(*devinitsw[i].p_devinit)();
}
