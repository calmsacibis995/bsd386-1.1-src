/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: port.c,v 1.1 1994/01/14 23:36:43 polk Exp $ */
#include "doscmd.h"

#define	MINPORT		0x000
#define	MAXPORT_MASK	(MAXPORT - 1)

/* 
 * Fake input/output ports
 */

static void
outb_nullport(int port, unsigned char byte)
{
    debug(D_PORT, "outb_nullport called for port 0x%03X = 0x%02X.\n",
		   port, byte);
}

static unsigned char
inb_nullport(int port)
{
    debug(D_PORT, "inb_nullport called for port 0x%03X.\n", port);
    return(0xff);
}

/*
 * configuration table for ports' emulators
 */

struct portsw {
	unsigned char	(*p_inb)(int port);
	void		(*p_outb)(int port, unsigned char byte);
} portsw[MAXPORT];

void
init_io_port_handlers(void)
{
	int i;

	for (i = 0; i < MAXPORT; i++) {
		portsw[i].p_inb = inb_nullport;
		portsw[i].p_outb = outb_nullport;
	}
}

void
define_input_port_handler(int port, unsigned char (*p_inb)(int port))
{
	if ((port >= MINPORT) && (port < MAXPORT)) {
		portsw[port].p_inb = p_inb;
	} else
		fprintf (stderr, "attempt to handle invalid port 0x%04x", port);
}

void
define_output_port_handler(int port, void (*p_outb)(int port, unsigned char byte))
{
	if ((port >= MINPORT) && (port < MAXPORT)) {
		portsw[port].p_outb = p_outb;
	} else
		fprintf (stderr, "attempt to handle invalid port 0x%04x", port);
}


void
inb(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_al = (*in_handler)(port);
	debug(D_PORT, "IN  on port %02x -> %02x\n", port, b->tf_al);
}

void
insb(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);
	unsigned char data;

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port);
	*(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di) = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 1) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 1) & 0xffff;
}

void
inx(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_al = (*in_handler)(port);
	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port + 1].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_ah = (*in_handler)(port + 1);
	debug(D_PORT, "IN  on port %02x -> %04x\n", port, tf->tf_ax);
}

void
insx(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);
	unsigned char data;

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port);
	*(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di) = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port + 1].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port + 1);
	((u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di))[1] = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 2) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 2) & 0xffff;
}

void
outb(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	void (*out_handler)(int, unsigned char);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port, b->tf_al);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, b->tf_al);
}

void
outx(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	void (*out_handler)(int, unsigned char);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port, b->tf_al);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, b->tf_al);
	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port + 1].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port + 1, b->tf_ah);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port + 1, b->tf_ah);
}

void
outsb(struct trapframe *tf, int port)
{
	void (*out_handler)(int, unsigned char);
	unsigned char value;

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	value = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, value);
	(*out_handler)(port, value);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 1) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 1) & 0xffff;
}

void
outsx(struct trapframe *tf, int port)
{
	void (*out_handler)(int, unsigned char);
	unsigned char value;

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	value = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, value);
	(*out_handler)(port, value);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port + 1].p_outb;
	else
		out_handler = outb_nullport;
	value = ((u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di))[1];
	debug(D_PORT, "OUT on port %02x <- %02x\n", port+1, value);
	(*out_handler)(port + 1, value);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 2) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 2) & 0xffff;
}
