/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_rhreg.h,v 1.1 1993/02/22 16:41:41 karels Exp $
 */

/*
 * Definitions for SDL Comm's RISCom/H2 dual
 * synchronous/asynchronous serial communications adapter
 *
 * This adapter is based on Hitachi HD64570 Serial Communications
 * Adapter chip.
 */

/* Channels per adapter */
#define RH_NCHANS       2

/* Check for the valid base address */
#define RH_IOBASEVALID(base)    (((base) & ~0x1f0) == 0x200)

#define RH_NPORT        16

/* Valid IRQs are 3,4,5,6,10,11,12,15 */
#define RH_IRQS         (IRQ3|IRQ4|IRQ5|IRQ6|IRQ10|IRQ11|IRQ12|IRQ15)
#define RH_IRQVALID(i)  ((1 << (i)) & RH_IRQS)

/* Valid DRQs are 5,6,7 */
#define RH_DRQVALID(d)  (5 <= (d) && (d) <= 7)

/*
 * Macro definitions for reading/writing byte to SCA ports
 */
#define RH_SCA(port)  (((port) & 0xf) | (((port) & 0xf0) << 6))

#define rinb(base, port)        inb((base) + RH_SCA(port) + 0x8000)
#define routb(base, port, val)  outb((base) + RH_SCA(port) + 0x8000, (val))

/*
 * PC Control Register (at base address)
 */
#define RHC_RUN         0x01    /* 0 - reset SCA */
#define RHC_EOUT0       0x02    /* enable ext clock out (chan 0) */
#define RHC_EOUT1       0x04    /* enable ext clock out (chan 1) */
#define RHC_TE0         0x08    /* enable transmitter (chan 0) */
#define RHC_TE1         0x10    /* enable transmitter (chan 1) */
#define RHC_DTR0        0x20    /* DTR out (chan 0) */
#define RHC_DTR1        0x40    /* DTR out (chan 1) */
#define RHC_DMAEN       0x80    /* DMA Enable */

/* Quartz oscillator frequency (Hertz) */
#define RH_OSC_FREQ     8000000
