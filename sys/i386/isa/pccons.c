/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: pccons.c,v 1.41 1994/01/30 00:42:57 karels Exp $
 */
 
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

/*
 * code to work keyboard & display for PC-style console
 */
#include "param.h"
#include "conf.h"
#include "ioctl.h"
#include "proc.h"
#include "user.h"
#include "tty.h"
#include "uio.h"
#include "callout.h"
#include "systm.h"
#include "kernel.h"
#include "malloc.h"
#include "reboot.h"
#include "syslog.h"
#include "device.h"

#include "machine/cpu.h"
#include "../i386/cons.h"

#include "icu.h"
#include "isa.h"
#include "timerreg.h"
#include "ic/i8042.h"
#include "isavar.h"
#include "pcconsvar.h"
#include "pcconsioctl.h"

#define NPCSCREENS	8		/* max for 32 KB display memory */
#define NMONOSCREENS	1		/* max for 4 KB mono display memory */

int npcscreens = NPCSCREENS;		/* or 1 for monochrome */
extern struct tty *constty;
struct tty *pcauxtty;
struct tty pccons;
struct tty *pctty[NPCSCREENS] = { &pccons };
struct tty pckbd;
extern int cold;

/*
 * Structures and variables in keyboard mapping tables
 */
extern	struct key pcconstab[];
extern	int pcconstabsize;
extern	int pcextstart;
extern	int pcextend;
extern	struct key pcexttab[];
extern	struct key pcaltconstab[];
extern	int pcaltconstabsize;
extern	struct key accenttab[];
extern	int maxaccent;

/*
 * The current state of virtual displays
 */
static struct screen {
	u_short *cp;		/* the current character address */
	enum state {
		NORMAL,			/* no pending escape */
		ESC,			/* saw ESC */
		EBRAC,			/* saw ESC[ */
		EBRACEQ,		/* saw ESC[= */
		EBRACEQU		/* saw ESC[? */
	} state;		/* command parser state */
#define	NACC	3		/* max parameters for escape sequence */
	int	acc[NACC];	/* the escape seq arguments */
#define	cx	acc[0]		/* the first escape seq argument */
#define	cy	acc[1]		/* the second escape seq argument */
	int	*accp;		/* pointer to the current processed argument */
	int	row;		/* current column */
	int	rows;		/* rows in scrolling region */
	u_short *startscroll;	/* top of current scrolling region */
	u_short *endscroll;	/* end of current scrolling region */
	int	charset;	/* "font", mostly for graphics */
/* charsets >= CSET_GRAPH are known to be "graphics" sets */
#define	CSET_STD	0	/* primary, control chars not displayed (?) */
#define	CSET_ISO	1	/* ISO rather than PC */
#define	CSET_GRAPH	2	/* 1st alt, *all* control chars are graphic */
#define	CSET_MGRAPH	3	/* 2nd alt, graphic, toggle bit 7 */
	int	so;		/* standout mode */
	u_short curcolor;	/* current char color (pos. mod from color) */
	u_short ca;		/* additional current attributes, bold/blink */
	/* The next three could be an array indexed by mode... */
	u_short color;		/* normal character color */
	u_short color_so;	/* standout color */
	u_short color_graph;	/* "graphics" color */
	u_short save_color;	/* saved normal color */
	u_short save_color_so;	/* saved standout color */
	int	acmode;		/* attribute controller mode */
	u_short	*savecp;	/* saved cursor location */
	struct	wayout wayout;	/* for async output */
} screen0, *screen[NPCSCREENS] = { &screen0 };

static int cnflags; 
static int run_cursor = 1;
static int in_pcrint;

static int cur_screen;		/* The current screen */

static int kbdinuse; 
static int kbdmajor = -1;

int	pcprobe();
void	pcattach();

struct cfdriver pcconscd = 
	{ NULL, "pccons", pcprobe, pcattach, sizeof(struct pcconsoftc) };

#define	COL		80
#define	ROW		25
#define	PAGE		(ROW * COL)
#ifdef SETSTATUS
#define	SROW		24		/* end of scrolling region */
#else
#define	SROW		25		/* end of scrolling region */
#endif
#define	CHR		2
#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000
#define	MCGA_NPORT	2		/* for now, # ports at MONO/CGA_BASE */

#define EGA_ISR1        0x3DA
#define EGA_AC_BASE     0x3C0

int	led_state;
int	numlock = 1;

/*
 * Attribute controller mode, initial mode is:
 * bit 0 - 0    - alphanumeric
 * bit 1 - 0    - color mode
 * bit 2 - 0    - 9th char dot repeats 8th for line graphics chars (in meta-set)
 * bit 3 - 1    - 8th attrib. bit is blinking, not bg intensity
 * bit 4 - 0    - unused
 * bit 5 - 0    - ignore line compare
 * bit 6 - 0    - not 256 colors
 * bit 7 - 0    - do not modify palette index
 */
#define	AC_LINEGRAPH	0x4
#define	AC_BLINK	0x8

#define	AC_DFLT	 AC_BLINK 	/* BIOS default for attribute controller reg */

int	acmode = AC_DFLT;

#define	enable_blink()		if ((acmode&AC_BLINK) == 0) \
					set_acmode(acmode|AC_BLINK);
#define	enable_bg_intens()	if (acmode&AC_BLINK) \
					set_acmode(acmode&~AC_BLINK);

extern	int pccons_trycolor;
unsigned int addr_6845 = MONO_BASE;
u_short *Crtat;
int	pcscreenmem = NPCSCREENS * PAGE * CHR;	/* used in vga.c */
int	pccmdbyte = KBC_KBDXLATE|KBC_INHIBDIS|KBC_SYSTEM|KBC_ENABLEKBDI;

#define	KBDATA		0x60	/* kbd data port */
#define	IO_PORTB	0x61	/* speaker control port (and more) */
#define		SPEAKER_ENABLE	0x3	/* enable speaker data, timer */
#define	KBSTAT		0x64	/* kbd status port */
#define		KBS_ORDY	0x01	/* kbd output scancode ready */
#define		KBS_IFULL	0x02	/* kbd input buffer full, not ready */

#define	S_NOBLOCK	0	/* sgetc should not block for input */
#define	S_BLOCK		1	/* sgetc should block for input */

static void cursor();
#define	CURS_NOW	0	/* set cursor now */
#define	CURS_REPEAT	1	/* set cursor now, then continuously */

int	pcstart();
int	pcparam();
void	pcrestart();
static	void initscreen __P((int)), initscreen1 __P((int)), pcinit();
static	void set_acmode();
int	ttrstrt();
char	partab[];

/*
 * Implement 8042 keyboard controller command (to port KBSTAT)
 * or keyboard command (to port KBDATA).  Also used to send
 * data to controller via KBDATA for writing command byte.
 */
kb_command(port, data)
	int port;
	u_char data;
{
	int i = 1000000;

	/* wait for chip command input ready */
	while (inb(KBSTAT)&KBS_IFULL)
		if (--i <= 0)
			return (-1);
	outb(port, data);
	return (0);
}

/*
 * Implement keyboard controller, keyboard, or auxilliary device
 * command followed by a data byte.  Used on KBSTAT for controller
 * command (including "write device"), on KBDATA for keyboard.
 * Note: it is up to the caller to ensure that no other commands
 * can intervene, e.g. by blocking keyboard interrupts.
 */
kb_data_cmd(port, cmd, data)
	int port;
	u_char cmd, data;
{

	if (kb_command(port, cmd) == -1 ||
	    kb_command(KBDATA, data) == -1)
		return (-1);
	return (0);
}

pcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int got_err = 0, got_ack = 0;
	int i;
	u_char c;

	/* Enable interrupts and keyboard controller */
	if (kb_data_cmd(KBSTAT, KCMD_WCMD, pccmdbyte) == -1)
		printf("keyboard controller timeout\n");

	/* Start keyboard RESET */
	if (kb_command(KBDATA, KCMD_RESET) == -1) {
		printf("keybard reset timeout\n");
		goto found;		/* attach just in case */
	}
	
	/*
	 * Pick up keyboard reset return code, waiting at most 10 sec.
	 * However, if we don't see an ack of the reset command,
	 * we wait at most 2 sec.
	 */
	for (i = 0; i < 100000; i++) {
		if (inb(KBSTAT) & KBS_ORDY) {
			if ((c = inb(KBDATA)) == KBD_RESETOK)
				goto found;
			if (c == KBD_ACK) {
				got_ack = 1;
				got_err = 0;
			} else if (got_err == 0)
				got_err = c;
		}
		if (got_ack == 0 && i >= 20000)
			break;
		DELAY(100);
	}
	/*
	 * If we weren't successful in doing a reset,
	 * print a message but configure the keyboard
	 * anyway.  If it works now or later, we'll get
	 * interrupts and start work.
	 */
	printf("keyboard");
	if (got_ack) {
		if (got_err)
			printf(" self-test failed (code %x)\n", got_err);
		else
			printf(": no reset code?\n");
	} else if (got_err)
		printf(" disconnected? (code %x)\n", got_err);

found:
	ia->ia_irq = IRQ1;	/* PC keyboard always here */
	return (1);
}

static int
iscolor()
{
	u_short volatile *cp;
	u_short save1, save2;
	int color;

	if (pccons_trycolor == 0)
		return (0);

	cp = (u_short volatile *)(atdevbase + CGA_BUF - IOM_BEGIN);

	save1 = cp[0];
	save2 = cp[1];

	cp[0] = 0xa55a;
	cp[1] = 0;

	color = (cp[0] == 0xa55a);

	cp[0] = save1;
	cp[1] = save2;

	return (color);
}

void
pcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{
	register struct pcconsoftc *sc = (struct pcconsoftc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	extern struct biosinfo *biosinfo;
	int pcrint();
	int i, j;

	if (iscolor())
		printf(": color, %d screens\n", npcscreens);
	else {
		printf(": mono\n");
		npcscreens = NMONOSCREENS;
	}
	if (Crtat == NULL)
		pcinit();		/* sets addr_6845 */
	isa_portalloc(addr_6845, MCGA_NPORT);

	/* if BIOS left numlock off, we will too */
	if (biosinfo && (biosinfo->kbflags & BIOS_KB_NUMLOCK) == 0)
		numlock = 0;
	if (numlock)
		led_state |= LED_NUM;
	update_led(led_state);
	cursor(CURS_REPEAT);

	isa_establish(&sc->cs_id, &sc->cs_dev);
	sc->cs_ih.ih_fun = pcrint;
	sc->cs_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->cs_ih, DV_TTY);

	strcpy(sc->cs_ttydev.tty_name, pcconscd.cd_name);
	sc->cs_ttydev.tty_count = 1;
	sc->cs_ttydev.tty_ttys = &pccons;
	tty_attach(&sc->cs_ttydev);

	/* wait for 2 acks from update_led */
	for (i = 10000, j = 0; i > 0; i--) {
		if (inb(KBSTAT)&KBS_ORDY) {
			if (inb(KBDATA) == KBD_ACK && ++j == 2)
				break;
		} else
			DELAY(100);
	}
}

/*
 * Initialize data structures for a virtual console screen other than
 * the primary.  Called on first open of each non-primary screen.
 * We allocate storage for the tty and the screen structure together,
 * as the total is just under an allocation size.
 */
static struct tty *
pcalloc(unit)
	int unit;
{
	struct ttydevice_tmp *dev;
	struct tty *tp;

	dev = malloc(sizeof(struct ttydevice_tmp), M_DEVBUF, M_WAITOK);
	bzero(dev, sizeof(*dev));
	tp = malloc(sizeof(struct tty) + sizeof(struct screen),
		M_DEVBUF, M_WAITOK);
	bzero(tp, sizeof(struct tty) + sizeof(struct screen));
	pctty[unit] = tp;
	screen[unit] = (struct screen *) (tp + 1);
	initscreen(unit);

	strcpy(dev->tty_name, "vcons");
	dev->tty_unit = unit;
	dev->tty_base = unit;
	dev->tty_count = 1;
	dev->tty_ttys = tp;
	tty_attach(dev);
	return (tp);
}

/* ARGSUSED */
#ifdef __STDC__
pcopen(dev_t dev, int flag, int mode, struct proc *p)
#else
pcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
#endif
{
	register struct tty *tp;
	int unit = minor(dev), kbd = (major(dev) == kbdmajor), error;
	int c, set_win = 0;

	if (unit >= npcscreens)
		return (ENXIO);
	tp = kbd ? &pckbd : pctty[unit];
	if (tp == NULL)
		tp = pcalloc(unit);
	tp->t_oproc = pcstart;
	tp->t_param = pcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		tp->t_termios = deftermios;
		pcparam(tp, &tp->t_termios);
		/* clear out any pending input to ensure we get interrupts */
		do {
			c = sgetc(NULL, S_NOBLOCK);
		} while (c != 0x100); 
		ttsetwater(tp);
		set_win = 1;
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	tp->t_state |= TS_CARR_ON;
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	if (kbd)
		kbdinuse = 1;
	else if (set_win) {
		/* set window size after ttyopen */
		tp->t_winsize.ws_row = SROW;
		tp->t_winsize.ws_col = COL;
	}
	return (error);
}

/* ARGSUSED */
#ifdef __STDC__
kbdopen(dev_t dev, int flag, int mode, struct proc *p)
#else
kbdopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
#endif
{

	kbdmajor = major(dev);
	return (pcopen(dev, flag, mode, p));
}
 

pcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct tty *tp;
	
	tp = major(dev) == kbdmajor ? &pckbd : pctty[minor(dev)];
	(*linesw[tp->t_line].l_close)(tp, flag);
	if (major(dev) == kbdmajor)
		kbdinuse = 0;
	if (major(dev) == kbdmajor || (tp == &pccons && kbdinuse == 0))  {
		cnflags &= ~CSF_RAW;		/* XXX */
		if (run_cursor == 0) {
			cursor(CURS_REPEAT);
			run_cursor = 1;
		}
	}
	ttyclose(tp);
	return (0);
}

/*ARGSUSED*/
pcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct tty *tp;
	
	tp = major(dev) == kbdmajor ? &pckbd : pctty[minor(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
pcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	struct tty *tp = pctty[minor(dev)];

	if (minor(dev) == 0 && constty &&
	    (constty->t_state&(TS_CARR_ON|TS_ISOPEN))==(TS_CARR_ON|TS_ISOPEN))
	    	tp = constty;
	else if (kbdinuse) {
		uio->uio_resid = 0;
		tp->t_outq.c_cc = 0;
		return (0);
	}
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

pcselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	struct tty *tp = pctty[minor(dev)];

	if (minor(dev) == 0 && constty &&
	    (constty->t_state&(TS_CARR_ON|TS_ISOPEN))==(TS_CARR_ON|TS_ISOPEN))
	    	tp = constty;
	else
		tp = major(dev) == kbdmajor ? &pckbd : pctty[minor(dev)];
	return (ttyselect(tp, flag, p));
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 * Catch the character, and see who it goes to.
 */
pcrint(sc)
	struct pcconsoftc *sc;
{

#ifdef notdef
	if (cnflags&CSF_POLLING)
		return;
#endif
	in_pcrint = 1;
	(void) sgetc(kbdinuse ? &pckbd : pctty[cur_screen], S_NOBLOCK);
	in_pcrint = 0;
	return (1);
}

pcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	register struct tty *tp;
	register error;

	tp = major(dev) == kbdmajor ? &pckbd : pctty[minor(dev)];
	error = 0;
	switch (cmd) {
		case PCCONIOCRAW:
			cnflags |= CSF_RAW;
			if (run_cursor) {
				untimeout(cursor, CURS_REPEAT);
				run_cursor = 0;
			}
			break;
		case PCCONIOCCOOK:
			cnflags &= ~CSF_RAW;
			if (run_cursor == 0) {
				cursor(CURS_REPEAT);
				run_cursor = 1;
			}
			break;
		case PCCONIOCSETLED:
			update_led(*(char *)data);
			break;
		case PCCONIOCBEEP:
			sysbeep();
			break;
		case PCCONIOCSTARTBEEP:
			sysbeepstart(*(unsigned short *) data);
			break;
		case PCCONIOCSTOPBEEP:
			sysbeepstop();
			break;
		case PCCONIOCMAPPORT:
			error = mapioport(((struct iorange *) data)->iobase,
			    ((struct iorange *) data)->cnt, p);
			break; 	
		case PCCONIOCUNMAPPORT:
			error = unmapioport(((struct iorange *) data)->iobase,
			    ((struct iorange *) data)->cnt, p);
			break; 	
		case PCCONENABIOPL:
			error = enable_IOPL(p);
			break; 	
		case PCCONDISABIOPL:
			error = disable_IOPL(p);
			break; 	
		default:			
			error = 
			   (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
			if (error >= 0)
			   return (error);
			error = ttioctl(tp, cmd, data, flag);
			if (tp->t_winsize.ws_row && tp->t_winsize.ws_row <= ROW)
				screen[minor(dev)]->rows = tp->t_winsize.ws_row;
			break;
	}
	if (error >= 0)
		return (error);
	return (ENOTTY);
}

/*
 * Color and attributes for normal, standout and kernel output
 * are stored in the most-significant byte of a u_short
 * so they don't have to be shifted for use.
 * This is all byte-order dependent.
 */
#define	BOLD	0x08			/* high intensity, foreground */
#define	BLINK	0x80			/* blink (or high intensity backgrnd) */
#define	BGHIGH	0x80			/* blink (or high intensity backgrnd) */

#define	CATTR(x) ((x) << 8)		/* store color/attributes shifted */
#define	ATTR_ADDR(which) ((u_char *)&(which) + 1) /* address of attributes */

#define	CA_BOLD		CATTR(BOLD)
#define	CA_BLINK	CATTR(BLINK)

u_short	pccolor = CATTR(0x7);		/* color/attributes for tty output */
u_short	pccolor_so = CATTR(0x70);	/* color/attributes, standout mode */
u_short	pccncolor = CATTR(0x1f);	/* color/attributes, kernel output */

/*
 * Common subexpressions from sput escape processing
 * for color-changing operations.
 */
static void
set_fg(cp, c)
	u_short *cp;
	int c;
{
	u_char *colp = ATTR_ADDR(*cp);

	*colp = (*colp & 0xf0) | c;
}

static void
set_bg(cp, c)
	u_short *cp;
	int c;
{
	u_char *colp = ATTR_ADDR(*cp);

	*colp = (*colp&0xf) | (c << 4);
}

static void
set_color(d)
	register struct screen *d;
{

	if (d->so)
		d->curcolor = d->color_so | d->ca;
	else
		d->curcolor = d->color | d->ca;
}

/*
 * Set normal/standout/graphic foreground or background color.
 * Standout color changes in the opposite way; if currently in standout,
 * reverse the change fg vs. bg.
 */
static void
set_allcolors(d, color, fg)
	register struct screen *d;
	int color, fg;
{

	if (d->so)
		fg = !fg;
	if (fg) {
		set_fg(&d->color, color);
		set_fg(&d->color_graph, color);
		set_bg(&d->color_so, color);
	} else {
		set_fg(&d->color_so, color);
		set_bg(&d->color, color);
		set_bg(&d->color_graph, color);
	}
}

/*
 * output block size before checking for preemption;
 * perhaps should be larger on faster machines.
 */
#define OBUFSIZE		64
int	pcoutsize = OBUFSIZE;

pcstart(tp)
	register struct tty *tp;
{
	register c, s;
	int unit = minor(tp->t_dev);
	extern u_char *getcblk();
	u_char *cp;

	if (in_pcrint) {
		struct wayout *wo = &screen[unit]->wayout;

		if (wo->pending == 0 && tp->t_outq.c_cc)
			wayout(wo);
		return;
	}

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	/*
	 * We do output in blocks to avoid calling splx/spltty
	 * once per character.  As we do output synchronously
	 * with a write, we could take a long time writing
	 * and scrolling while preventing rescheduling.
	 * Stop if want_resched is set anytime after the first
	 * buffer full.
	 */
	for (;;) {
		int count;
		if (tp->t_outq.c_cc == 0)
			break;
		if (kbdinuse) {
			ndflush(&tp->t_outq, tp->t_outq.c_cc);
			break;
		}
		cp = getcblk(&tp->t_outq, &count);
		if (count > pcoutsize)
			count = pcoutsize;
		tp->t_state |= TS_BUSY;
		splx(s);
		c = count;
		while (c) {
			if (tp->t_state & (TS_TTSTOP|TS_FLUSH))
				break;
			sput(*cp++, 0, unit);
			c--;
		}
		s = spltty();
		if ((tp->t_state & TS_FLUSH) == 0)
			ndflush(&tp->t_outq, count - c);
		tp->t_state &= ~(TS_BUSY|TS_FLUSH);
		if (want_resched)
			break;
	}
	if (tp->t_outq.c_cc) {
		tp->t_state |= TS_TIMEOUT;
		timeout(ttrstrt, tp, 1);
	}
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
out:
	splx(s);
}

void
pcrestart(tp)
	struct tty *tp;
{

	/*
	 * Although screen[]->wayout.pending is set at interrupt
	 * level, we don't bother synchronizing at spltty();
	 * if we clear it before calling pcstart, the worst
	 * that can happen is that we are called again later
	 * with nothing to do.
	 */
	screen[minor(tp->t_dev)]->wayout.pending = 0;
	pcstart(tp);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
pcstop(tp, flag)
	register struct tty *tp;
{

	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
}

pccnprobe(cp)
	struct consdev *cp;
{
	int maj;

	/* locate the major number of console */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == pcopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_tp = &pccons;
	cp->cn_pri = CN_INTERNAL;
}

/* ARGSUSED */
pccninit(cp)
	struct consdev *cp;
{

	if (Crtat == NULL)
		pcinit();
}

/* ARGSUSED */
pccnputc(dev, c)
	dev_t dev;
	char c;
{

	if (!kbdinuse)
		sput(c, pccncolor, cur_screen);
}

/* ARGSUSED */
pccngetc(dev)
	dev_t dev;
{
	register int c, s;

	s = spltty();		/* block pcrint while we poll */
	cursor(CURS_NOW);
	c = sgetc((struct tty *)NULL, S_BLOCK);
	if (c == '\r')
		c = '\n';
	splx(s);
	return (c);
}

#ifdef notdef
pcgetchar(tp)
	register struct tty *tp;
{
	int c;

	c = sgetc((struct tty *)NULL, S_BLOCK);
	return (c&0xff);
}
#endif

/*
 * Set line parameters
 */
pcparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register int cflag = t->c_cflag;
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	return(0);
}

extern int hz;
static int beeping;

struct beep {
	enum	beeptype { NONE, AUDIBLE /*, BLINK */ } type;
	int	freq;			/* as counter value */
	int	duration;		/* ms */
} beep = {
	AUDIBLE, 0x637 /* 750 Hz */, 250
};

sysbeepstart(freq)
	short freq;
{

	/* enable counter 2 */
	outb(IO_PORTB, inb(IO_PORTB)|SPEAKER_ENABLE);
	/* set command for counter 2, 2 byte write */
	outb(TIMER_MODE, TIMER_SEL2|TIMER_16BIT|TIMER_SQWAVE);
	outb(TIMER_CNTR2, freq & 0xff);
	outb(TIMER_CNTR2, freq >> 8);
}

sysbeepstop()
{
	/* disable counter 2 */
	outb(IO_PORTB, inb(IO_PORTB)&~SPEAKER_ENABLE);
	beeping = 0;
}

sysbeep()
{

	switch (beep.type) {
	case AUDIBLE:
		/* send 0x637 for 750 HZ */
		sysbeepstart(beep.freq);
		if (!beeping)
			timeout(sysbeepstop, 0,
			    beep.duration / (tick / 1000));
		beeping = 1;
		break;
	}
}

/*
 * Switch the screen
 */
static void
switchscreen(n)
	unsigned int n;
{
	register struct screen *d;
	int strt;

	if (n >= npcscreens || (d = screen[n]) == NULL)
		return;
	screen[cur_screen]->acmode = acmode;
	cur_screen = n;

	/* Set display start registers */
	strt = PAGE * n;
	outb(addr_6845, 12);
	outb(addr_6845+1, strt >> 8);
	outb(addr_6845, 13);
	outb(addr_6845+1, strt);
	cursor(CURS_NOW);
	if (d->acmode != acmode)
		set_acmode(d->acmode);
}

/*
 * cursor() sets an offset (0-1999) into the 80x25 text area   
 */
static void
cursor(repeat)
	int repeat;
{
 	int s = spltty();
 	int pos = screen[cur_screen]->cp - Crtat;
	static int lastpos = -1;

	if (lastpos != pos) {
		outb(addr_6845, 14);
		outb(addr_6845+1, pos >> 8);
		outb(addr_6845, 15);
		outb(addr_6845+1, pos);
		lastpos = pos;
	}
	splx(s);
	if (repeat)
		timeout(cursor, CURS_REPEAT, hz / 10);
}

static void
pcinit()
{
	register struct screen *d = screen[0];
	unsigned cursorat;

	/*
	 * Check for CGA memory; if present, initialize for color,
	 * otherwise assume monochrome.
	 */ 
	if (iscolor()) {
		Crtat = (u_short *)(atdevbase + (CGA_BUF - IOM_BEGIN));
		addr_6845 = CGA_BASE;
	} else
		Crtat = (u_short*)(atdevbase + (MONO_BUF - IOM_BEGIN));

	/* Extract cursor location */
	outb(addr_6845,14);
	cursorat = inb(addr_6845+1) << 8;
	outb(addr_6845,15);
	cursorat |= inb(addr_6845+1);

	if (cursorat >= PAGE)
		cursorat = 0;

	d->cp = Crtat + cursorat;
	d->startscroll = Crtat;
	initscreen1(0);
	fillw(pccolor|' ', d->cp, PAGE - cursorat);

	switchscreen(0);
#if SROW != ROW
	if (cursorat >= COL * SROW)
		sput(0, 0, 0);		/* force scroll */
#endif
}

static void
initscreen(scrn)
	int scrn;
{
	register struct screen *d = screen[scrn];

	d->cp = Crtat + scrn * PAGE;
	d->startscroll = d->cp;
	initscreen1(scrn);
	fillw(pccolor|' ', d->cp, PAGE);
}

static void
initscreen1(scrn)
	int scrn;
{
	register struct screen *d = screen[scrn];

	d->color = pccolor;
	d->curcolor = pccolor;
	d->color_graph = pccolor;
	d->save_color = pccolor;
	d->color_so = pccolor_so;
	d->save_color_so = pccolor_so;
	d->rows = SROW;
	d->endscroll = d->startscroll + SROW * COL;
	d->savecp = d->startscroll;
	d->acmode = AC_DFLT;
	d->wayout.func = pcrestart;
	d->wayout.arg = pctty[scrn];
}

/*
 * Set attribute controller mode register
 */
static void
set_acmode(m)
	int m;
{
	int s;

	/* Read Input Status Register 1 to reset AC flip-flop */
	s = splhigh();
	(void) inb(EGA_ISR1);
	DELAY(100);

	/* Select the Attribute Mode register of AC */
	outb(EGA_AC_BASE, 0x10);
	DELAY(100);

	outb(EGA_AC_BASE, m);
	DELAY(100);

	/* Now enable the display */
	(void) inb(EGA_ISR1);
	DELAY(100);
	outb(EGA_AC_BASE, 0x20);
	splx(s);

	acmode = m;
}


static char sco_to_cga[] = { 
	0,	/* black */
	4,	/* red */
	2,	/* green */
	6,	/* brown */
	1,	/* blue */
	3,	/* cyan */
	5,	/* magenta */
	7,	/* grey */
};

u_char scroll;
int	do_page;
int	page_lines = SROW - 5;	/* keep initial messages */
char	page_msg[] = "P\037r\037e\037s\037s\037 \037a\037n\037y\037 \037k\037e\037y\037 \037t\037o\037 \037c\037o\037n\037t\037i\037n\037u\037e\037:\037";

/* translation from ISO charset to PC screen char for top half of charset */
static u_char isotable[128-32]= {
0x20,0XAD,0X63,0X9C,0X0F,0X9D,0X7C,0X15,0X22,0XA8,0XA6,0XAE,0XAA,0XC4,0XA8,0XC4,
0XF8,0XF1,0XFD,0XA8,0X27,0XE6,0X14,0XF9,0X2C,0XA8,0XA7,0XAF,0XAC,0XAB,0XA8,0XA8,
0X41,0X41,0X41,0X41,0X8E,0X8F,0X92,0X80,0X45,0X90,0X45,0X45,0X49,0X49,0X49,0X49,
0X44,0XA5,0X4F,0X4F,0X4F,0X4F,0X99,0X58,0X9D,0X55,0X55,0X55,0X9A,0X59,0XA8,0XE1,
0X85,0XA0,0X83,0X61,0X84,0X86,0X91,0X87,0X8A,0X82,0X88,0X89,0X8D,0XA1,0X8C,0X8B,
0XA8,0XA4,0X95,0XA2,0X93,0X6F,0X94,0XF6,0X9B,0X97,0XA3,0X96,0X81,0X79,0XA8,0X98
};

/*
 * Write a character to the screen buffer, updating screen state.
 */
#define	wrtchar(c, d) { \
	*(d->cp) = (c); \
	d->cp++; \
	d->row++; \
}

/*
 * sput has support for emulation of the 'ibmpc' termcap entry.
 * This is a bare-bones implementation of a bare-bones entry
 * One modification: Change li#24 to li#25 to reflect 25 lines
 * "ca" is the color/attributes value (left-shifted by 8)
 * or 0 if the current regular color for that screen is to be used.
 */
sput(c, ca, scrn)
	u_char c;
	int ca;
	int scrn;
{
	register struct screen *d = screen[scrn];
	register u_short *base;
	int i, j, count;
	u_short *pp;

	base = Crtat + scrn * PAGE;
	i = ca;
	if (ca == 0)
		ca = d->curcolor;

	switch (d->state) {
	case NORMAL:
		if (d->charset >= CSET_GRAPH && i == 0 && c != '\033') {
			/* non-kernel, graphics chars, just print! */
			if (d->charset == CSET_MGRAPH)
				c ^= 0x80;	/* xor or or? */
			if (c)
				wrtchar(ca | c, d);
			if (d->row >= COL)
				d->row = 0;
		} else switch (c) {

		case 0x0:		/* Ignore pad characters */
		case 0xff:		/* Ignore 0xff */
			break;

		case 0x1B:
			d->state = ESC;
			break;

		case 0x9B:		/* "meta-escape" CSI */
			d->state = EBRAC;
			bzero(d->acc, sizeof(d->acc));
			d->accp = d->acc;
			break;

		case '\t':
			do {
				wrtchar(ca | ' ', d);
			} while (d->row % 8);
			break;

		case '\b':  /* non-destructive backspace */
			if (d->row > 0) {
				d->cp--;
				d->row--;
			}
			break;

		case '\r':
			/*
			 * Special case for kernel diagnostics --
			 * clear end of line with kernel background on CR.
			 */
			if (i)
				fillw(i|' ', d->cp,
				   COL - (d->cp - base) % COL);
			d->cp -= d->row;
			d->row = 0;
			break;

		case '\n':
			d->cp += COL;
			break;

		case '\007':
			sysbeep();
			break;

		default:
			if (d->charset == CSET_ISO) {
				if (c < 32)
					break;
				if (c > 127 + 32)
					c = isotable[c - 128 - 32]; 
			}
			wrtchar(ca | c, d);
			if (d->row >= COL)
				d->row = 0;
			break;
		}
		break;

	case EBRAC:
		if ((count = d->cx) < 1)
			count = 1;

		/*
		 * In this state, the action at the end of the switch
		 * on the character type is to go to NORMAL state,
		 * and intermediate states do a return rather than break.
		 */
		switch (c) {

		case 'm': {
		    int *mode;

		    /*
		     * The following is not understood, but WordPerfect
		     * uses this strange sequence which resets colors.
		     */
		    if (d->accp == d->acc + 2 && *d->accp == 0 &&
		    	d->acc[0] == 2 && d->acc[1] == 7) {
			    d->color = pccolor;
			    d->color_so = pccolor_so;
		    }

		    for (mode = d->acc; mode <= d->accp; mode++)
			switch (*mode) {
			case 0:	/* normal */
				d->ca = 0;
				d->so = 0;
				break;
			case 1:	/* bold */
				d->ca |= CA_BOLD;
				break;
			case 5:	/*blink*/
				d->ca |= CA_BLINK;
				enable_blink();
				break;
			case 7:	/* inverse */
				d->so = 1;
				break;
			case 8:	{ /* invisible */
				u_char *colp = ATTR_ADDR(d->curcolor);

				*colp = (*colp & 0xf0) | (*colp >> 4);
				goto no_setcolor;
				}

			/* 10/11/12: change "font" (character/graph set) */
			case 10: /* primary: ignore control chars */
				d->charset = CSET_STD;
				break;
			case 11: /* 1st alt: print all as font (even \n) */
				d->charset = CSET_GRAPH;
				d->curcolor = d->color_graph | d->ca;
				goto no_setcolor;
			case 12: /* 2nd alt: toggle bit 7, display as font */
				d->charset = CSET_MGRAPH;
				d->curcolor = d->color_graph | d->ca;
				goto no_setcolor;
				break;
			case 13: /* iso: map chars to iso set (BSDI only) */
				d->charset = CSET_ISO;
				break;

			default:
				/* set current (norm/so) fg, other bg */
				if ((i = *mode) >= 30 && i <= 37) {
					set_allcolors(d, sco_to_cga[i - 30], 1);
					break;
				}

				/* set current (norm/so) bg, other fg */
				if (i >= 40 && i <= 47) {
					set_allcolors(d, sco_to_cga[i - 40], 0);
					break;
				}
				break;
		    }
		    set_color(d);
no_setcolor:
		    break;
		}

		case 'A': /* back row(s) */
			if (count > (i = (d->cp - d->startscroll) / COL))
				count = i;
			d->cp -= count * COL;
			/*d->def_wrap = 0;*/
			break;

		case 'B': /* down row(s) */
			if (count > (i = (d->endscroll - d->cp) / COL))
				count = i;
			d->cp += count * COL;
			/*d->def_wrap = 0;*/
			break;

		case 'C': /* right cursor */
			if (count > COL - 1 - d->row)
				count = COL - 1 - d->row;
			d->cp += count;
			d->row += count;
			/* d->def_wrap = 0; */
			break;

		case 'D': /* left cursor */
			if (count > d->row)
				count = d->row;
			d->cp -= count;
			d->row -= count;
			/*d->def_wrap = 0;*/
			break;

		case 'J': /* Clear to end of display */
			switch (d->cx) {
			case 0:
				fillw(ca|' ', d->cp,
				    base + COL * d->rows - d->cp);
				break;
			case 1:
				fillw(ca|' ', base, d->cp - base + 1);
				break;
			case 2:
				fillw(ca|' ', base, COL * d->rows);
				break;
			}
			/*d->def_wrap = 0;*/
			break;

		case 'K': /* Clear to EOL */
			switch (d->cx) {
			case 0:
				fillw(ca|' ', d->cp, COL - d->row);
				break;
			case 1:
				fillw(ca|' ', d->cp - d->row, d->row + 1);
				break;
			case 2:
				fillw(ca|' ', d->cp - d->row, COL);
				break;
			}
			/*d->def_wrap = 0;*/
			break;

		case 'H': /* Cursor move */
			if (d->cx > d->rows)
				d->cx = d->rows;
			if (d->cy > COL)
				d->cy = COL;
			if (d->cx == 0 || d->cy == 0) {
				d->cp = base;
				d->row = 0;
			} else {
				d->cp = base + (d->cx - 1) * COL + d->cy - 1;
				d->row = d->cy - 1;
			}
			break;

		case 'L':	/* Insert line */
			if (d->cp < d->startscroll || d->cp >= d->endscroll)
				break;
			d->cp -= d->row;	/* move to col 0 ??? */
			d->row = 0;		/* ???; see bcopy/fillw */
			i = (d->cp - base) / COL;
			j = (d->endscroll - base) / COL;
			if (count > j - i)
				count = j - i;
			j = (j - i - count) * COL * CHR; /* amt to scroll */
			ovbcopy(d->cp, d->cp + (count * COL), j);
			fillw(d->color|' ', d->cp, COL * count);
			/* d->def_wrap = 0; */
			break;
			
		case 'M':	/* Delete line */
			if (d->cp < d->startscroll || d->cp >= d->endscroll)
				break;
			d->cp -= d->row;
			d->row = 0;
			i = (d->cp - base) / COL;
			j = (d->endscroll - base) / COL;
			if (count > j - i)
				count = j - i;
			bcopy(d->cp + COL * count, d->cp,
			    (j - count - i) * COL * CHR);
			fillw(d->color|' ', base + COL * (j - count),
			    COL * count);
			/* d->def_wrap = 0; */
			break;

#ifdef notyet
		case 'P':
			count = d->cx;
			if (count < 1) count = 1;
			if (count > COL - d->row) count = COL - d->row;
			bcopy(d->cp + count, d->cp, (COL - d->row - count)*CHR);
			fillw(d->color|' ', d->cp - d->row + COL - count, count );
			d->def_wrap = 0;
			
			break;

		case 'X':
			count = d->cx;
			if (count < 1) count = 1;
			if (count > COL - d->row) count = COL - d->row;
			fillw(d->color|' ', d->cp , count );
			d->def_wrap = 0;
			break;
#endif

		case '_': /* set cursor type */
			if (d->accp == d->acc) {
				if (d->cx == 2)
					d->cx = 0;	/* invisible */
				else if (d->cx)
					d->cx = 1;	/* block */
				else
					d->cx = 12;	/* underline */
				d->cy = 13;
			}
			outb(addr_6845, 10);
			outb(addr_6845+1, d->cx);
			outb(addr_6845, 11);
			outb(addr_6845+1, d->cy);
			break;

		case ';': /* Switch params in cursor def */
			if (d->accp < &d->acc[NACC-1]) {
				d->accp++;
				return;
			}
			break;	/* cancel escape */

		case '=': /* ESC[= color change */
			d->state = EBRACEQ;
			return;

		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				*(d->accp) *= 10;
				*(d->accp) += c - '0';
				return;
			} else
				break;
		}
		d->state = NORMAL;
		break;

	case EBRACEQ: {
		u_char *colp;

		/*
		 * In this state, the action at the end of the switch
		 * on the character type is to go to NORMAL state,
		 * and intermediate states do a return rather than break.
		 */

		/*
		 * Set foreground/background color
		 * for normal mode, standout mode
		 * or kernel output.
		 * Original version based on code from kentp@svmp03.
		 */
		switch (c) {
		case 'D':
			*ATTR_ADDR(d->curcolor) &= ~BGHIGH;
			break;
		case 'E':
			enable_bg_intens();
			break;
		case 'F':
			colp = ATTR_ADDR(d->color);
	do_fg:
			*colp = (*colp & 0xf0) | (d->cx&0xf);
			set_color(d);
			break;

		case 'G':
			colp = ATTR_ADDR(d->color);
	do_bg:
			if (d->cx & 0x8)
				enable_bg_intens();	/* XXX */
			*colp = (*colp & 0xf) | ((d->cx&0xf) << 4);
			set_color(d);
			break;

		case 'H':
			colp = ATTR_ADDR(d->color_so);
			goto do_fg;

		case 'I':
			colp = ATTR_ADDR(d->color_so);
			goto do_bg;

		case 'J':
			colp = ATTR_ADDR(d->color_graph);
			goto do_fg;

		case 'K':
			colp = ATTR_ADDR(d->color_graph);
			goto do_bg;

		case 'k': /* BSDI only */
			colp = ATTR_ADDR(pccncolor);
			goto do_fg;

		case 'l': /* BSDI only */
			colp = ATTR_ADDR(pccncolor);
			goto do_bg;

		case 'M': /* BSDI only - ??? */
			set_acmode(d->cx);
			break;

		case 'S':
			d->save_color = d->color;
			d->save_color_so = d->color_so;
			break;

		case 'R':
			d->color = d->save_color;
			d->color_so = d->save_color_so;
			set_color(d);
			break;

		case 's': /* save cursor position */
			d->savecp = d->cp;
			break;

		case 'u': /* restore cursor position */
			d->cp = d->savecp;
			d->row = (d->cp - base) % COL;
			break;

		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				d->cx *= 10;
				d->cx += c - '0';
				return;
			} else
				break;
		}
		d->state = NORMAL;
	    }
	    break;

	case ESC:
		switch (c) {
		case 'c':	/* Clear screen & home */
			fillw(d->color|' ', base, COL * d->rows);
			d->cp = base;
			d->row = 0;
			d->state = NORMAL;
			break;
		case '7': /* save cursor position */
			d->savecp = d->cp;
			d->state = NORMAL;
			break;
		case '8': /* restore cursor position */
			d->cp = d->savecp;
			d->row = (d->cp - base) % COL;
			d->state = NORMAL;
			break;
		case '[':	/* Start ESC [ sequence */
			d->state = EBRAC;
			bzero(d->acc, sizeof(d->acc));
			d->accp = d->acc;
			break;
		default: /* Invalid, clear state */
			d->state = NORMAL;
			break;
		}
		break;
	}
	if (d->cp >= base + (COL * d->rows)) { /* scroll check */
#if 0
		while (scroll)
			sgetc((struct tty *)NULL, S_NOBLOCK);
#endif
		bcopy(base + COL, base, COL * (d->rows - 1) * CHR);
		fillw(d->color|' ', base + COL * (d->rows - 1), COL);
		d->cp -= COL;
		if (do_page && d == &screen0 && --page_lines == 0) {
			bcopy(page_msg, base + COL * (d->rows - 1),
			    sizeof(page_msg) - 1);
			d->cp += (sizeof(page_msg) - 1) / sizeof(*Crtat);
			cursor(CURS_NOW);
			while (sgetc((struct tty *)NULL, S_NOBLOCK) == 0x100)
				;
			d->cp -= (sizeof(page_msg) - 1) / sizeof(*Crtat);
			fillw(pccolor|' ', base + COL * (d->rows - 1), COL);
			page_lines = d->rows - 1;
		}
	}
	if (cold || panicstr)
		cursor(CURS_NOW);
}

update_led(n)
	u_char n;
{
	int s;
	static int once;

	s = spltty();
	if (kb_data_cmd(KBDATA, KCMD_SETLED, n) == -1 && once++ == 0)
		printf("update_led timed out\n");
	splx(s);
}

reset_cpu()
{

	/*
 	 * This will simply disappear if we are successful in
	 * causing a processor reset.  Otherwise, it will be
	 * the last thing on the screen.
	 */
	printf("press reset button to reboot if necessary\n");

	/*
	 * Attempt to reset via the keyboard processor.
	 * Apparently not all machines will do this,
	 * and the bus won't be reset anyway, but try anyway.
	 */
	kb_command(KBSTAT, KCMD_PULSE | 0xE);	/* Reset Command, low bit 0 */
	DELAY(500000);

	/* 
	 * Still alive.
	 * Cause a processor "shutdown" cycle by making an
	 * empty interrupt descriptor table, then causing
	 * an interrupt.
	 */
	lidt(0, 0);
	asm("int3");

	/*
	 * The processor certainly won't get here.  The only way
	 * out of a shutdown cycle is a processor reset.
	 */
}

#ifdef DEBUG
unsigned __debug = 0;
#endif

/* state for locking keys, so they don't autorepeat: */
static char keydown;
#define	D_CAPSLOCK	0x01
#define	D_NUMLOCK	0x02
#define	D_SCROLL	0x04	/* unused currently */
#define	D_ALTSHIFT	0x08

/*
 * sgetc(tp, block): get a character from the keyboard.
 * If tty tp is specified, call the input function with any resulting
 * character(s), otherwise return at most one character.
 * If block == S_BLOCK, wait until a key is gotten, otherwise return
 * 0x100 (256) if there is no input.
 */
sgetc(tp, block)
	struct tty *tp;
	int block;
{
	u_char dt, *cp;
	int isbreak;
	struct key *pc;
	static u_char l_mod, r_mod, capslock, num = 1, exts;
	static u_char altshift, accent;
	extern int cntlaltdel, cntlaltdelcore;

	for (;;) {
		u_char stat;

		/* First see if there is something in the keyboard port */
		while (((stat = inb(KBSTAT))&KBS_ORDY) == 0)
			if (block == S_NOBLOCK)
				return (0x100);
		DELAY(7);		/* needed on PS/2 */
		dt = inb(KBDATA);

		/* check for input from auxiliary device */
		if (stat&KBS_AUXDATA && pcauxtty) {
			(*linesw[pcauxtty->t_line].l_rint)(dt, pcauxtty);
			continue;
		}

		if (cnflags & CSF_RAW && tp)
			goto got_char;

		/* Check for cntl-alt-del */
		if ((dt == 83) && (l_mod|r_mod) == (M_CONTROL|M_ALT) &&
		    cntlaltdel) {
			if (cnquery("cntl-alt-del: reboot")) {
				if (cntlaltdelcore && cnquery(" dump core"))
					boot(RB_AUTOBOOT|RB_DUMP);
				printf(" rebooting: ");
				boot(RB_AUTOBOOT);
			} else
				continue;
		}

		/* Check for 0xE0 prefix for extended keyboard characters */
		if (dt == 0xe0) {
			exts = 1;
			continue;
		}
		isbreak = dt & 0x80;
		dt &= 0x7f;
		/*
		 * If exts is set, look in extension table, which holds codes
		 * for scancodes "E0 pcextstart" up to "E0 pcextend".
		 */
		if (exts) {
			exts = 0;
			if (dt < pcextstart || dt >= pcextend)
				continue;
			pc = &pcexttab[dt - pcextstart];
		} else if (altshift) {
			if (dt >= pcaltconstabsize)
				continue;
			pc = &pcaltconstab[dt];
		} else {
			if (dt >= pcconstabsize)
				continue;
			pc = &pcconstab[dt];
		}
		/* Check for make/break */
		if (isbreak) {
			/* break */
			switch (pc->action) {
			case SHF:
				l_mod &= ~M_SHIFT;
				continue;
			case R_SHF:
				r_mod &= ~M_SHIFT;
				continue;
			case ALT:
				l_mod &= ~M_ALT;
				continue;
			case R_ALT:
				r_mod &= ~M_ALT;
				continue;
			case CTL:
				l_mod &= ~M_CONTROL;
				continue;
			case R_CTL:
				r_mod &= ~M_CONTROL;
				continue;
#ifdef DEBUG
			case DFUNC:
				/* Toggle debug flags */
				__debug ^= (1 << pc->code[0]);
				continue;
#endif
			case NUM:
				keydown &= ~D_NUMLOCK;
				continue;

			case CPS:
				keydown &= ~D_CAPSLOCK;
				continue;

#if 0
			case SCROLL:
				keydown &= ~D_SCROLL;
				continue;
#endif

			case ALTSHIFT:
				keydown &= ~D_ALTSHIFT;
				continue;
			default:
				continue;
			}
		} else {
			/* make */
keyaction:
			switch (pc->action) {

			/* LOCKING KEYS */
			case NUM:
				if ((keydown & D_NUMLOCK) == 0) {
					keydown |= D_NUMLOCK;
					num ^= 1;
					led_state ^= LED_NUM;
					update_led(led_state);
				}
				continue;

			case CPS:
				if ((keydown & D_CAPSLOCK) == 0) {
					keydown |= D_CAPSLOCK;
					capslock ^= M_SHIFT;
					led_state ^= LED_CAP;
					update_led(led_state);
				}
				continue;

			case SCROLL:
#if 0
				if ((keydown & D_SCROLL) == 0) {
					keydown |= D_SCROLL;
					scroll ^= 1;
					led_state ^= LED_SCR;
					update_led(led_state);
				}
#endif
				continue;

			case ALTSHIFT:
				/*
				 * This is a hack.  If the action
				 * is ALTSHIFT, we look at the value.
				 * If zero, we shift input sets;
				 * if non-zero, we use the value to find
				 * a replacement action in accenttab.
				 * This allows something like CNTL-ALT-keypad
				 * as altshift.
				 */
				dt = pc->code[l_mod|r_mod];
				if (dt == 0) {
					if ((keydown & D_ALTSHIFT) == 0) {
						keydown |= D_ALTSHIFT;
						altshift ^= 1;
						led_state ^= LED_SCR;
						update_led(led_state);
					}
				} else {
					pc = &accenttab[dt];
					goto keyaction;
				}
				continue;

			/* NON-LOCKING KEYS */
			case SHF:
				l_mod |= M_SHIFT;
				continue;
			case R_SHF:
				r_mod |= M_SHIFT;
				continue;
			case ALT:
				l_mod |= M_ALT;
				continue;
			case R_ALT:
				r_mod |= M_ALT;
				continue;
			case CTL:
				l_mod |= M_CONTROL;
				continue;
			case R_CTL:
				r_mod |= M_CONTROL;
				continue;
			case ACCENT:
				dt = pc->code[l_mod|r_mod];
				if (dt <= maxaccent) {
					accent = dt;
					continue;
				}
				break;
			case CHAR:
				dt = pc->code[l_mod|r_mod];
				break;
			case ACCENTED:
				pc = &accenttab[pc->code[0] + accent];
				/* FALLTHOUGH */
			case ALPHA:
				dt = pc->code[(l_mod|r_mod)^capslock];
				break;
			case FUNC:
			case SFUNC:
				/* FUNC + ALT is a screen switch */
#define F1	0x3b
#define	ESC	'\033'
				if (dt >= F1 && dt < F1 + NPCSCREENS &&
				    (l_mod|r_mod) & M_ALT) {
					switchscreen(dt - F1);
					continue;
				}

				/*
				 * function key: can only input string
				 * if given a tty pointer, not if returning
				 * a char
				 */
				if (tp == NULL)
					continue;
				if (pc->action == FUNC) {
				    for (cp = pc->code; *cp; )
					(*linesw[tp->t_line].l_rint)(*cp++, tp);
				    continue;
				} else {
					(*linesw[tp->t_line].l_rint)(ESC, tp);
					(*linesw[tp->t_line].l_rint)('[', tp);
					dt = pc->code[l_mod|r_mod];
				}
				break;
			case NUMPAD:
				/*
				 * numeric keypad: produces one char
				 * if numlock is on, or kernel getc
				 * returning one char, else produces a string
				 */
				if (num || tp == NULL)
					dt = pc->code[0];
				else {
					for (cp = pc->code + 1; *cp; )
					    (*linesw[tp->t_line].l_rint)(*cp++,
					    tp);
					continue;
				}
				break;
			case IGNORE:
			default:
				continue;
			}
		}
got_char:
		accent = 0;
		if (tp)
			(*linesw[tp->t_line].l_rint)(dt, tp);
		else
			return ((int) dt);
	}
}

pg(p,q,r,s,t,u,v,w,x,y,z)			/* XXX */
	char *p;
{
	printf(p,q,r,s,t,u,v,w,x,y,z);
	printf("\n");
	sput('>', CATTR(0x6), 0);
	return(getchar());
}

/* special characters */
#define bs	8
#define lf	10	
#define cr	13	
#define cntlc	3	
#define del	0177	
#define cntld	4
int	pc_echocolor = CATTR(0x1f);

/* XXX this shouldn't be here */
getchar()
{
	register char thechar;
	int s;

#ifdef notdef
	cnflags |= CSF_POLLING;
#endif
	s = splhigh();
	cursor(CURS_NOW);
	thechar = (char) sgetc((struct tty *)NULL, S_BLOCK);
#ifdef notdef
	cnflags &= ~CSF_POLLING;
#endif
	splx(s);
	switch (thechar) {
	    default: if (thechar >= ' ')
			sput(thechar, pc_echocolor, 0);
		     return (thechar);
	    case cr:
	    case lf: sput(cr, pc_echocolor, 0);
		     sput(lf, pc_echocolor, 0);
		     return (lf);
	    case bs:
	    case del:
		     sput(bs, pc_echocolor, 0);
		     sput(' ', pc_echocolor, 0);
		     sput(bs, pc_echocolor, 0);
		     return (thechar);
	    case cntld:
		     sput('^', pc_echocolor, 0);
		     sput('D', pc_echocolor, 0);
		     sput('\r', pc_echocolor, 0);
		     sput('\n', pc_echocolor, 0);
		     return (0);
	}
}

#ifdef SETSTATUS
setstatus(col, c, attr)
	int col, c, attr;
{
	u_short *cp = Crtat + cur_screen * PAGE + COL * (ROW - 1) + col;
	
	if (c)
		*cp = (attr << 8) | c;
	else
		*(((char *)cp) + 1) = attr;
}
#endif

#ifdef broken
#ifdef DEBUG
#include "machine/dbg.h"
#include "machine/stdarg.h"
static nrow;
static __color;


void
#ifdef __STDC__
dprintf(unsigned flgs, const char *fmt, ...)
#else
dprintf(flgs, fmt /*, va_alist */)
        char *fmt;
	unsigned flgs;
#endif
{	extern unsigned __debug;
	va_list ap;

	if((flgs&__debug) > DPAUSE) {
		__color = ffs(flgs&__debug)+1;
		va_start(ap,fmt);
		kprintf(fmt, 1, (struct tty *)0, ap);
		va_end(ap);
		if (flgs&DPAUSE || nrow%24 == 23) { 
			int x;
			x = splhigh();
			if (nrow%24 == 23) nrow = 0;
			sgetc((struct tty *)NULL, S_BLOCK);
			splx(x);
		}
	}
	__color = 0;
}
#endif DEBUG
#endif
