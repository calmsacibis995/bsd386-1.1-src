/*	BSDI $Id: sireg.h,v 1.3 1994/01/30 07:38:06 karels Exp $	*/

/*
 * @(#) (c) Specialix International, 1990,1992
 * @(#) (c) Andy Rutter, 1993
 * Id: sireg.h,v 1.12 1993/10/18 08:28:42 andy Exp
 *
 *	'C' definitions for Specialix serial multiplex driver.
 *
 * Module:	sireg.h
 * Target:	BSDI/386
 * Author:	Andy Rutter, andy@acronym.co.uk
 *
 * Revision: 1.12
 * Date: 1993/10/18 08:28:42
 * Status:
 *
 * Derived from:	SunOS 4.x version
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Andy Rutter of
 *	Advanced Methods and Tools Ltd. based on original information
 *	from Specialix International.
 * 4. Neither the name of Advanced Methods and Tools, nor Specialix
 *    International may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE
 */

/*
 * Macro to turn a device number into various parameters, and test for
 * CONTROL device and XPRINT version of port.
 * max of 2 controllers with up to 32 ports per controller.
 * minor device allocation is:
 * adapter	port	xprint	control_dev
 *   0          0-31	64-95	128-255
 *   1		32-63	96-127	   "
 */
#define	SI_MAXPORTPERCARD	32
#define	SI_MAXCONTROLLER	2
#define	SI_MAXPORT	(SI_MAXCONTROLLER * SI_MAXPORTPERCARD)

#define	SI_PORT(d)	(minor(d) & 0x1f)
#define	SI_CARD(d)	((minor(d) & 0x20) >> 5)
#define	SI_TTY(d)	(minor(d) & 0x3f)
#define	ISXPRINT(d)	(minor(d) & 0x40)
#define	ISCONTROL(d)	(minor(d) & 0x80)

#define	DEV2SC(d)	((struct si_softc *)sicd.cd_devs[SI_CARD((d))])
#define	PP2DEV(pp)	((pp)->sp_tty->t_dev)
#define	PP2SC(pp)	((struct si_softc *)sicd.cd_devs[SI_CARD(PP2DEV(pp))])
#define	DEV2PP(d)	(DEV2SC((d))->sc_ports + SI_PORT((d)))
#define	DEV2TP(d)	(DEV2PP((d))->sp_tty)

/* Max chars in an XPRINT enable/disable string */
#define	XP_MAXSTR	15

/*
 * Hardware parameters which should be changed at your peril!
 */
 
typedef	unsigned char	BYTE;		/* Type cast for unsigned 8 bit */
typedef	unsigned short	WORD;		/* Type cast for unsigned 16 bit */

/* Base and mask for SI Host 2.x (SIPLUS) */
#define SIPLSIG		0x7FF8			/* Start of control space */
#define SIPLCNTL	0x7FF8			/* Ditto */
#define SIPLRESET	SIPLCNTL		/* 0 = reset */
#define SIPLIRQ11	(SIPLCNTL+1)		/* 0 = mask irq 11 */
#define SIPLIRQ12	(SIPLCNTL+2)		/* 0 = mask irq 12 */
#define SIPLIRQ15	(SIPLCNTL+3)		/* 0 = mask irq 15 */
#define SIPLIRQSET	(SIPLCNTL+4)		/* 0 = interrupt host */
#define SIPLIRQCLR	(SIPLCNTL+5)		/* 0 = clear irq */

/* SI Host 1.x */
#define	SIRAM		0x0000			/* Ram Starts here */
#define	SIRESET		0x8000			/* Set reset */
#define	SIRESET_CL 	0xc000			/* Clear reset */
#define	SIWAIT		0x9000			/* Set wait */
#define	SIWAIT_CL 	0xd000			/* Set wait */
#define SIINTCL		0xA000			/* Clear host int */
#define SIINTCL_CL 	0xE000			/* Clear host int */

/* Adapter types */
#define	SIEMPTY		0
#define	SIHOST		1
#define	SI2		2
#define	SIPLUS		3
#define	SIEISA		4

/*
 * MEMSIZE is the total shared mem region
 * RAMSIZE is value to use when probing 
 */
#define	SIHOST_MEMSIZE		0xffff
#define	SIHOST_RAMSIZE		0x8000
#define	SIPLUS_MEMSIZE		0x7fff
#define	SIPLUS_RAMSIZE		0x7ff7

/* Buffer parameters */
#define	SLXOS_BUFFERSIZE	256
#define	MIN_TXWATER		20
#define	MAX_TXWATER		(SLXOS_BUFFERSIZE-20)

/*
 * Hardware `registers'
 */
struct si_reg	{
	BYTE	initstat;
	BYTE	memsize;
	WORD	int_count;
	WORD	revision;
	BYTE	rx_int_count;
	BYTE	spare;
	WORD	int_pending;
	WORD	int_counter;
	BYTE	int_scounter;
	BYTE	res[0x80 - 13];
};

/* info communicated from siprobe() to siattach() */
struct si_type {
	char	*st_typename;
	u_int	st_ramsize;
};

struct si_softc {
	struct	device sc_dev;  	/* base device */
	struct	isadev sc_id;   	/* ISA device */
	struct	intrhand sc_ih; 	/* interrupt vectoring */

	struct si_type	*sc_type;	/* adapter type */
#define	sc_ramsize	sc_type->st_ramsize
	struct si_port	*sc_ports;	/* port structures for this card */
	struct	ttydevice_tmp sc_ttydev; /* tty stuff */
#ifdef	SI_XPRINT
	struct	ttydevice_tmp sc_xttydev; /* XPRINT ttys */
#endif
	caddr_t		sc_paddr;	/* physical addr of iomem */
	caddr_t		sc_maddr;	/* kvaddr of iomem */
	int		sc_nport;	/* # ports on this card */
	int		sc_irq;		/* copy of attach irq */
	int		sc_flags;	/* copy of modem flags */
	WORD		sc_revision;	/* card revision ? */
};

/*
 *	Per module control structure
 */
struct si_module {
	WORD	sm_next;		/* Next module */
	BYTE	sm_type;		/* Number of channels */
	BYTE	sm_number;		/* Module number on cable */	
	BYTE	sm_dsr;			/* Private dsr copy */
	BYTE	sm_res[0x80 - 5];	/* Reserve space to 128 bytes */
};

/*
 *	The 'next' pointer & with 0x7fff + SI base addres give
 *	the address of the next module block if fitted. (else 0)
 *	Note that next points to the TX buffer so 0x60 must be
 *	subtracted to find the true base.
 *
 *	Type is a bit field as follows:  The bottom 5 bits are the
 *	number of channels  on this module,  the top 3 bits are
 *	as the module type thus:
 *
 *	000	2698 RS232 module (4 port or 8 port)
 *	001	Reserved for 2698 RS422 module 
 *	010	Reserved for 8530 based sync module
 *	011	Reserved for parallel printer module
 *	100	Reserved for network module
 *	101-111	Reserved for expansion.
 *
 *	The number field is the cable position of the module.
 */
#define	M232	0x00
#define M422	0x20
#define MSYNC	0x40
#define MCENT	0x60
#define MNET	0x80
#define MMASK	0x1F

/*
 *	Per channel control structure
 */
struct	si_channel {
	/*
	 * Generic stuff
	 */
	WORD	next;			/* Next Channel */	
	WORD	addr_uart;		/* Uart address */
	WORD	module;			/* address of module struct */
	BYTE 	type;			/* Uart type */
	BYTE	fill;
	/*
	 * Uart type specific stuff
	 */
	BYTE	x_status;		/* XON / XOFF status */
	BYTE	c_status;		/* cooking status */
	BYTE	hi_rxipos;		/* stuff into rx buff */
	BYTE	hi_rxopos;		/* stuff out of rx buffer */
	BYTE	hi_txopos;		/* Stuff into tx ptr */
	BYTE	hi_txipos;		/* ditto out */
	BYTE	hi_stat;		/* Command register */
	BYTE	dsr_bit;		/* Magic bit for DSR */
	BYTE	txon;			/* TX XON char */
	BYTE	txoff;			/* ditto XOFF */
	BYTE	rxon;			/* RX XON char */
	BYTE	rxoff;			/* ditto XOFF */
	BYTE	hi_mr1;			/* mode 1 image */
	BYTE	hi_mr2;			/* mode 2 image */
        BYTE	hi_csr;			/* clock register */
	BYTE	hi_op;			/* Op control */
	BYTE	hi_ip;			/* Input pins */
	BYTE	hi_state;		/* status */
	BYTE	hi_prtcl;		/* Protocol */
	BYTE	hi_txon;		/* host copy tx xon stuff */
	BYTE	hi_txoff;
	BYTE	hi_rxon;
	BYTE	hi_rxoff;
	BYTE	close_prev;		/* Was channel previously closed */
	BYTE	hi_break;		/* host copy break process */
	BYTE	break_state;		/* local copy ditto */
	BYTE	hi_mask;		/* Mask for CS7 etc. */
	BYTE	mask_z280;		/* Z280's copy */
	BYTE	res[0x60 - 36];
	BYTE	hi_txbuf[SLXOS_BUFFERSIZE];
	BYTE	hi_rxbuf[SLXOS_BUFFERSIZE];
	BYTE	res1[0xA0];
};

/*
 *	Register definitions
 */

/*
 *	Break input control register definitions
 */
#define	BR_IGN		0x01	/* Ignore any received breaks */
#define	BR_INT		0x02	/* Interrupt on received break */
#define BR_PARMRK	0x04	/* Enable parmrk parity error processing */
#define	BR_PARIGN	0x08	/* Ignore chars with parity errors */

/*
 *	Protocol register provided by host for XON/XOFF and cooking
 */	
#define	SP_TANY		0x01	/* Tx XON any char */
#define	SP_TXEN		0x02	/* Tx XON/XOFF enabled */
#define	SP_CEN		0x04	/* Cooking enabled */
#define	SP_RXEN		0x08	/* Rx XON/XOFF enabled */
#define	SP_DCEN		0x20	/* DCD / DTR check */
#define	SP_PAEN		0x80	/* Parity checking enabled */

/*
 *	HOST STATUS / COMMAND REGISTER
 */
#define	IDLE_OPEN	0x00	/* Default mode, TX and RX polled
				   buffer updated etc */
#define	LOPEN		0x02	/* Local open command (no modem ctl */
#define MOPEN		0x04	/* Open and monitor modem lines (blocks
				   for DCD */
#define MPEND		0x06	/* Wating for DCD */
#define CONFIG		0x08	/* Channel config has changed */
#define CLOSE		0x0A	/* Close channel */
#define SBREAK		0x0C	/* Start break */
#define EBREAK		0x0E	/* End break */
#define IDLE_CLOSE	0x10	/* Closed channel */
#define IDLE_BREAK	0x12	/* In a break */
#define FCLOSE		0x14	/* Force a close */
#define RESUME		0x16	/* Clear a pending xoff */
#define WFLUSH		0x18	/* Flush output buffer */
#define RFLUSH		0x1A	/* Flush input buffer */

/*
 *	Host status register
 */
#define	ST_BREAK	0x01	/* Break received (clear with config) */

/*
 *	OUTPUT PORT REGISTER
 */
#define	OP_CTS	0x01	/* Enable CTS */
#define OP_DSR	0x02	/* Enable DSR */
/*
 *	INPUT PORT REGISTER
 */
#define	IP_DCD	0x04	/* DCD High */
#define IP_DTR	0x20	/* DTR High */
#define IP_RTS	0x02	/* RTS High */
#define	IP_RI	0x40	/* RI  High */

/*
 *	Mode register and uart specific stuff
 */
/*
 *	MODE REGISTER 1
 */
#define	MR1_5_BITS	0x00
#define	MR1_6_BITS	0x01
#define	MR1_7_BITS	0x02
#define	MR1_8_BITS	0x03
/*
 *	Parity
 */
#define	MR1_ODD		0x04
#define	MR1_EVEN	0x00
/*
 *	Parity mode
 */
#define	MR1_WITH	0x00
#define	MR1_FORCE	0x08
#define	MR1_NONE	0x10
#define	MR1_SPECIAL	0x18
/*
 *	Error mode
 */
#define	MR1_CHAR	0x00
#define	MR1_BLOCK	0x20
/*
 *	Request to send line automatic control
 */
#define	MR1_CTSCONT	0x80

/*
 *	MODE REGISTER 2
 */
/*
 *	Number of stop bits
 */
#define	MR2_1_STOP	0x07
#define	MR2_2_STOP	0x0F
/*
 *	Clear to send automatic testing before character sent
 */
#define	MR2_RTSCONT	0x10
/*
 *	Reset RTS automatically after sending character?
 */
#define	MR2_CTSCONT	0x20
/*
 *	Channel mode
 */
#define	MR2_NORMAL	0x00
#define	MR2_AUTO	0x40
#define	MR2_LOCAL	0x80
#define	MR2_REMOTE	0xC0

/*
 *	CLOCK SELECT REGISTER - this and the code assumes ispeed == ospeed
 */
#define U0	0x00	/* 0 Baud maps to 75 ! */
#define	U50	0xdd	/* 50 Baud maps 57.6 Kilobits  */
#define U75	0x00	/* 75 baud */
#define U110	0x11	/* 110 Baud  - maps to 115.2Kbaud on XIO */
#define U134	0x60	/* 134 maps to 1200/75 */
#define U150	0x33	/* 150 baud */
#define U200	0x06	/* 200 maps to 75/1200 */
#define U300	0x44	/* 300 baud */
#define U600	0x55	/* 600 baud */
#define U1200	0x66	/* 1200 baud */
#define U1800	0x77	/* 1800 maps to 2000 */
#define U2400	0x88	/* 2400 baud */
#define U4800	0x99	/* 4800 baud */
#define U9600	0xbb	/* 9600 baud */
#define U19200	0xcc	/* 19200 baud */
#define U38400	0x22	/* 38400 baud */

/*
 * Per-port (channel) soft information structure
 */
struct si_port {
	struct si_channel *sp_ccb;
	struct tty	*sp_tty;
	int		sp_pend;	/* pending command */
	int		sp_state;
	int		sp_flags;
#ifdef	SI_DEBUG
	int		sp_debug;	/* debug mask */
#endif
#ifdef SI_XPRINT
	struct tty	*sp_xtty;
	int		sp_xpopen;	/* XPRINT channel is open */
	int		sp_xpcps;
	char		sp_xpon[XP_MAXSTR+1];
	char		sp_xpoff[XP_MAXSTR+1];
#endif
#ifdef	TTY_BLKINPUT
	char		*sp_rbuf;
	int		sp_rcnt;
#endif
};

/* sp_state */
#define	SS_CLOSED	0x0000
#define SS_MOPEN	0x0001	/* Open as modem */
/*#define	SS_WOPEN	0x0002	/* waiting for carrier (DCD) */
#define	SS_LOPEN	0x0004	/* Open as local */
#define	SS_BUSY		0x0008	/* Busy (buffer full wait) */
#define	SS_XPBUSY	0x0010	/* Xprint is busy */
/*#define	SS_CARR_ON	0x0020	/* DCD asserted */
#define	SS_TBLK		0x0040	/* transmitter blocked */
#define	SS_TIMEOUT	0x0080	/* currently in timeout for a BREAK */
#define	SS_OASLP	0x0100	/* raw output asleep */
/*#define	SS_EXCLUSIVE	0x0200	/* exclusive open */
#define	SS_XPXCLUDE	0x0400	/* exclusive on XPRINT channel */
#define	SS_XPWANTR	0x0800	/* the XPRINT queue service wants to read */
#define	SS_WAITWRITE	0x1000
#define	SS_BLOCKWRITE	0x2000

/* sp_flags */
#define	SPF_COOKMODE		0x0003
#define	 SPFC_WELL			0
#define	 SPFC_MEDIUM			1
#define	 SPFC_RAW			2
#define	 spfc_clear(pp)		(pp)->sp_flags &= ~SPF_COOKMODE
#define	 SPF_COOK_WELL(pp)	spfc_clear(pp)
#define	 SPF_COOK_MEDIUM(pp)	{spfc_clear(pp);(pp)->sp_flags|=SPFC_MEDIUM;}
#define	 SPF_COOK_RAW(pp)	{spfc_clear(pp);(pp)->sp_flags|=SPFC_RAW;}
#define	 SPF_SETCOOK(pp, c)	{spfc_clear(pp);(pp)->sp_flags|=(c);}
#define  SPF_ISCOOKWELL(pp)	(((pp)->sp_flags & SPF_COOKMODE) == SPFC_WELL)
#define  SPF_ISCOOKMEDIUM(pp)	(((pp)->sp_flags & SPF_COOKMODE) == SPFC_MEDIUM)
#define  SPF_ISCOOKRAW(pp)	(((pp)->sp_flags & SPF_COOKMODE) == SPFC_RAW)
#define	SPF_COOKWELL_ALWAYS	0x0004	/* always use line disc */
#define	SPF_XPRINT		0x0008
#define	SPF_MODEM		0x0010
#define	 SPF_ISMODEM(pp)	((pp)->sp_flags & SPF_MODEM)
#define	SPF_IXANY		0x0020		/* IXANY enable/disable flag */
#define	SPF_CTSOFLOW		0x0040		/* use CTS to handle o/p flow */
#define	SPF_RTSIFLOW		0x0080		/* use RTS to handle i/p flow */
#define	SPF_PPP			0x0100		/* special handling for upper
						 * level protocol code */

/*
 *	Command post flags
 */
#define	SI_NOWAIT	0x00	/* Don't wait for command */
#define SI_WAIT		0x01	/* Wait for complete */

/*
 * Support for XPRINT - use of secondary port on asynch terminals
 */
#ifdef SI_XPRINT
#define	XP_ON	"\033d#"		/* default xprint on string */
#define	XP_OFF	"\024"			/* default xprint off string */
/* 
 * Number of "packets" to send to the transparent print device per second.
 * For greatest accuracy, this should be a divisor of hz
 */
#define XP_HZ	5
/* Number of characters to send in a "packet" */
#define XP_MAXPKT	(255 - (XP_MAXSTR*2) - 2)
#define XP_COUNT(cps)	(((cps/XP_HZ) < XP_MAXPKT) ? (cps/XP_HZ) : XP_MAXPKT)
/*
 * Delay corresponding to a given count of characters queued at the desired
 * cps rate
 */
#define XP_DELAY(cps,count)	((count * hz) / cps)
#define XP_CPS 120
#endif	/* SI_XPRINT */

/*
 * Extensive debugging stuff - manipulated using siconfig(8)
 */
#ifdef	KERNEL
#ifdef SI_DEBUG
static void si_dprintf();
static char *si_mctl2str();
#define	DPRINT(x)	si_dprintf x
#else
#define	DPRINT(x)	/* void */
#endif
#endif

#define	DBG_ENTRY		0x00000001
#define	DBG_DRAIN		0x00000002
#define	DBG_OPEN		0x00000004
#define	DBG_CLOSE		0x00000008
#define	DBG_READ		0x00000010
#define	DBG_WRITE		0x00000020
#define	DBG_PARAM		0x00000040
#define	DBG_INTR		0x00000080
#define	DBG_IOCTL		0x00000100
#define	DBG_XPRINT		0x00000200
#define	DBG_SELECT		0x00000400
#define	DBG_DIRECT		0x00000800
#define	DBG_START		0x00001000
#define	DBG_EXIT		0x00002000
#define	DBG_FAIL		0x00004000
#define	DBG_STOP		0x00008000
#define	DBG_AUTOBOOT		0x00010000
#define	DBG_MODEM		0x00020000
#define	DBG_DOWNLOAD		0x00040000
#define	DBG_LSTART		0x00080000
#define	DBG_POLL		0x00100000
#define	DBG_ALL			0xffffffff

/*
 * Cache support only needed when (if) the driver is checked out
 * for adapters at maddr > 1Mb.
 */
#ifdef CACHE
# define CACHEON cacheon();
# define CACHEOFF cacheoff();
#else
# define CACHEON 
# define CACHEOFF
#endif

/* 
 *	SI ioctls
 */
/*
 * struct for use by Specialix ioctls - used by siconfig(8)
 */
typedef struct {
	unsigned char
		sid_port:5,			/* 0 - 31 ports per card */
		sid_card:1,			/* 0 - 1 cards */
		sid_control:1,			/* controlling device (all cards) */
		sid_xprint:1;			/* xprint version of line */
} sidev_t;
struct si_tcsi {
	sidev_t	tc_dev;
	union {
		int	x_int;
		int	x_dbglvl;
		int	x_modemflag;
		char	x_string[XP_MAXSTR+1];
	}	tc_action;
#define	tc_card		tc_dev.sid_card
#define	tc_port		tc_dev.sid_port
#define	tc_int		tc_action.x_int
#define	tc_dbglvl	tc_action.x_dbglvl
#define	tc_modemflag	tc_action.x_modemflag
#define	tc_string	tc_action.x_string
};
#define	IOCTL_MIN	96
#define	TCSIDEBUG	_IOW('S', 96, struct si_tcsi)	/* Toggle debug */
#define	TCSIRXIT	_IOW('S', 97, struct si_tcsi)	/* RX int throttle */
#define	TCSIIT		_IOW('S', 98, struct si_tcsi)	/* TX int throttle */
#define	TCSIMIN		_IOW('S', 99, struct si_tcsi)	/* TX min xfer amount */
#define	TCSIXPON	_IOW('S', 100, struct si_tcsi)	/* Set XPON string */
#define	TCSIXPOFF	_IOW('S', 101, struct si_tcsi)	/* Set XPOFF string */
#define	TCSIXPCPS	_IOW('S', 102, struct si_tcsi)	/* Chars per second */
#define	TCSIIXANY	_IOW('S', 103, struct si_tcsi)	/* enable ixany */
			/* 104 defunct */
#define	TCSISTATE	_IOWR('S', 105, struct si_tcsi)	/* get current state of RTS
						   DCD and DTR pins */
		/* Set/reset/enquire cook mode, 1 = always use line disc
		 * -1 = enquire current setting */
#define	TCSICOOKMODE	_IOWR('S', 106, struct si_tcsi)
#define	TCSIPORTS	_IOR('S', 107, int)	/* Number of ports found */
#define	TCSISDBG_LEVEL	_IOW('S', 108, struct si_tcsi)	/* equivalent of TCSIDEBUG which sets a
					 * particular debug level (DBG_??? bit
					 * mask), default is 0xffff */
#define	TCSIGDBG_LEVEL	_IOWR('S', 109, struct si_tcsi)
#define	TCSIGRXIT	_IOWR('S', 110, struct si_tcsi)
#define	TCSIGIT		_IOWR('S', 111, struct si_tcsi)
#define	TCSIGMIN	_IOWR('S', 112, struct si_tcsi)
#define	TCSIGXPON	_IOWR('S', 113, struct si_tcsi)
#define	TCSIGXPOFF	_IOWR('S', 114, struct si_tcsi)
#define	TCSIGXPCPS	_IOWR('S', 115, struct si_tcsi)
			/* 116 defunct */
#define	TCSIMODEM	_IOWR('S', 117, struct si_tcsi)	/* set/clear/query the modem bit */

#define	TCSISDBG_ALL	_IOW('S', 118, int)		/* set global debug level */
#define	TCSIGDBG_ALL	_IOR('S', 119, int)		/* get global debug level */

#define	TCSIFLOW	_IOWR('S', 120, struct si_tcsi)	/* set/get h/w flow state */

/*
 * Stuff for downloading and initialising an adapter
 */
struct si_loadcode {
	int	sd_offset;
	int	sd_nbytes;
	char	sd_bytes[1024];
};
#define	TCSIDOWNLOAD	_IOW('S', 121, struct si_loadcode)
#define	TCSIBOOT	_IOR('S', 122, struct si_tcsi)

#define	TCSIPPP		_IOWR('S', 123, struct si_tcsi)	/* set/get PPP flag bit */

#define	IOCTL_MAX	123

#define	IS_SI_IOCTL(cmd)	((u_int)((cmd)&0xff00) == ('S'<<8) && \
		(u_int)((cmd)&0xff) >= IOCTL_MIN && \
		(u_int)((cmd)&0xff) <= IOCTL_MAX)

#define	CONTROLDEV	"/dev/si_control"
