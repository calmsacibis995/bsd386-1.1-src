/*	BSDI $Id: sb_driver.c,v 1.2 1994/01/30 00:57:58 karels Exp $	*/

/*
 * SoundBlaster Pro driver:
 *	Original author, Steve Haehnichen <shaehnic@ucsd.edu>.
 *	Ported to BSDI by John Kohl, jtkohl, and Steve McCoole (smm).
 *	Additional modifications made by BSDI.
 * 
 *	from Id: sb_driver.c,v 1.6 1992/12/10 17:03:34 jtkohl Exp
 */

/*======================================================================

  This original source for this file was released as part of
  SBlast-BSD-1.5 on 9/14/92 by Steve Haehnichen <shaehnic@ucsd.edu>
  under the terms of the GNU Public License.  Effective of 1/28/94, I
  (Steve Haehnichen) am replacing the GPL terms with a more
  kernel-friendly BSD-style agreement, as follows:

  *
  * Copyright (C) 1994 Steve Haehnichen.
  *
  * Permission to use, copy, modify, distribute, and sell this software
  * and its documentation for any purpose is hereby granted without
  * fee, provided that the above copyright notice appear in all copies
  * and that both that copyright notice and this permission notice
  * appear in supporting documentation.
  *
  * This software is provided `as-is'.  Steve Haehnichen disclaims all
  * warranties with regard to this software, including without
  * limitation all implied warranties of merchantability, fitness for
  * a particular purpose, or noninfringement.  In no event shall Steve
  * Haehnichen be liable for any damages whatsoever, including
  * special, incidental or consequential damages, including loss of
  * use, data, or profits, even if advised of the possibility thereof,
  * and regardless of whether in an action in contract, tort or
  * negligence, arising out of or in connection with the use or
  * performance of this software.
  *
  
======================================================================*/

#include <sb.h>
#if NSB > 0

#ifdef __bsdi__
#include "param.h"
#include "systm.h"
#include "ioctl.h"
#include "tty.h"
#include "proc.h"
#include "user.h"
#include "conf.h"
#include "file.h"
#include "uio.h"
#include "kernel.h"
#include "syslog.h"
#include "malloc.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"

/* SMM */
#include "buf.h"
#include "sys/device.h"
#include "i386/isa/sblast.h"
#include "i386/isa/sb_regs.h"
#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"

#define isa_dev isa_device
#define ESUCCESS 0
#define PSLEP PPAUSE|PCATCH
#define tenmicrosec() DELAY(10)
#define kvtophys kvtop
#undef DEBUG				/* I turn on DEBUG for other files,
					   but don't want it here--too
					   verbose. */
#else
#include <sys/user.h>
#include <sys/file.h>
#include <i386/ipl.h>
#include <i386/pio.h>
#include <i386at/atbus.h>

#include <i386at/sblast.h>	/* User-level structures and defs */
#include "sb_regs.h"		/* Register and command values */
#endif

/*
 * Defining DEBUG will print out lots of useful kernel activity
 * information.  If there are problems, it's the first thing you
 * should do.  You can also define FULL_DEBUG for more info than
 * you probably want.  (If you consider any DEBUG useless, just
 * turn it into a FULL_DEBUG.)
 */
/* #define DEBUG */
/* #define FULL_DEBUG */
#if defined (FULL_DEBUG) && !defined (DEBUG)
#  define DEBUG
#endif

/*
 * The Mixer is an odd deal.  I think Mixer ioctls should be
 * allowed on any device.  It's very safe.
 * If all your users complain that other users are messing up the
 * mixer while they are playing, well, undef this and make
 * mixer open's exclusive.
 */
#define 	ALWAYS_ALLOW_MIXER_IOCTLS

/*
 * This is the highest DSP speed that will use "Low-Speed" mode.
 * Anything greater than this will use the "High-Speed" mode instead.
 */
#define		MAX_LOW_SPEED	22222

/*
 * This is the speed the DSP will start at when you reboot the system.
 * If I can read the speed registers, then maybe I should just use
 * the current card setting.  Does it matter?
 * (43478 is the closest we can get to 44 KHz)
 * Make sure this is a valid speed for your card!
 */
#define		INITIAL_DSP_SPEED	43478

/*
 * SB Device minor numbers
 * I have copied the numbers used in Brian Smith's ISC Unix driver,
 * with the faint hope of future compatibility.
 * Oh, make sure this is a valid speed for your card!
 * The listed device names are simply the ones that I am using.
 * Note that there is absolutely no CMS support in this driver.
 */
#define SB_CMS_NUM  	0	/* /dev/sb_cms   CMS square wave chips   */
#define SB_FM_NUM    	1	/* /dev/sb_fm    FM synthesis chips      */
#define SB_DSP_NUM    	2	/* /dev/sb_dsp   DSP DAC/ADC             */
#define SB_MIDI_NUM   	3	/* /dev/sb_midi  MIDI port               */
#define SB_MIXER_NUM  	4	/* /dev/sb_mixer Non-exclusive-use Mixer */
#define SB_BIGGEST_MINOR 4	/* Highest minor number used.            */

/*
 * This is the "channel" and IRQ used for the FM timer functions.
 * Since I can't seem to get one to trigger yet,
 * I don't know what they should be.  (see fm_delay() comments)
 */
#define TIMER_CHANNEL		FM_BOTH
#define FM_TIMER_IRQ		8

/*
 * Here's the debugging macro I use here and there, so I can turn
 * them all on or off in one place.
 */
#ifdef DEBUG
#  define DPRINTF(x)	printf x
#else
#  define DPRINTF(x)
#endif

/* Unix interrupt priority.
   I set it to just above Hard Disk priority to keep things smooth. */
#define INTR_PRIORITY	(PRIBIO - 5)

#define GOOD 1
#define FAIL 0
#define ON   1
#define OFF  0

/* These are used to indicate what the DSP last did,
   and can safely do again without preparation */
#define NONE_READY	0
#define READ_READY	1
#define WRITE_READY	2

/* These are the possible values for argument 'rw' in the select handlers.
   Shouldn't they be in an include file somewhere? */
#define SELECT_READ	1
#define SELECT_WRITE	2

/* Number of "cycles" in one second.  Shouldn't this be somewhere else? */
#define HZ 		100

#define TIMEOUT		(10 * HZ) /* FM interrupt patience in clock ticks */

/* This sets dsp.timeout to an appropriate number of ticks */
#define COMPUTE_DSP_TIMEOUT \
  { dsp.timeout = HZ + (HZ * dsp.bufsize / dsp.speed); }

/* Semaphore return codes.  This tells the reason for the wakeup(). */
#define WOKEN_BY_INTERRUPT	1
#define WOKEN_BY_TIMEOUT	2

/* How many times should it cycle to wait for a DSP acknowledge? */
#define DSP_LOOP_MAX		1000

/* Interrupt-generating device units.  Just arbitrary numbers. */
#define DSP_UNIT		0
#define FM_TIMER_UNIT		1
#define NUM_UNITS		2 /* How many units above */

enum { PLAY, RECORD };		/* DSP sampling directions */

#define LOWBYTE(x)	((x) & 0x00ff)
#define HIBYTE(x)	(((x) & 0xff00) >> 8)

/* Handy macro from Brian Smith's/Lance Norskog's driver */
#define crosses_64k_boundary(a,b) ((((int)(a) & 0xffff) + (b)) > 0x10000)

/*
 * This is the DSP memory buffer size.  Choose wisely. :-)
 * I started with 21K buffers, and then later changed to 64K buffers.
 * This hogs more kernel memory, but gives fewer Pops and
 * needs servicing less often.  Adjust to taste.
 * Note that it must be an EVEN number if you want stereo to work.
 * (See dsp_find_buffers() for a better explanation
 * Note that the same amount of memory (192K) is hogged, regardless of
 * the buffer size you choose.  This makes it easier to allocate page-safe
 * buffers, but should probably be changed to attempt to pack more into
 * one 64K page.  Be really sure you understand what you are doing if
 * you change the size of memory_chunk[].
 * 64K is the biggest DSP_BUF_SIZE you can have.
 * Smaller buffer sizes make aborts and small reads more responsive.
 * For real-time stuff, perhaps ~10K would be better.
 * Note that you can now change this to any smaller value with the ioctl
 * DSP_IOCTL_BUFSIZE, so you are probably best off with a 64K limit.
 */
#define DSP_CHUNK_SIZE	(2*DSP_MAX_BUFSIZE+(64*1024)) /* + 64k, so that we're
							 assured of finding a 64k
							 boundary in the area so
							 we can insure proper
							 DMA workings. */
#ifndef __bsdi__
static char memory_chunk[DSP_CHUNK_SIZE]; /* Be careful!  See above */
#endif

/* This structure will be instantiated as a single global holder
   for all DSP-related variables and flags. */
struct sb_dsp_type
{
  int	speed;			/* DSP sampling rate */
  int	timeout;		/* Timeout for one DSP buffer */
  sbBYTE	compression;		/* Current DAC decompression mode */
  sbFLAG	hispeed;		/* 1 = High Speed DSP mode, 0 = low speed */
  sbFLAG  in_stereo;		/* 1 = currently in stereo, 0 = mono */
  sbBYTE	start_command;		/* current DSP start command */
  int	error;			/* Current error status on read/write */
  int	semaphore;		/* Silly place-holder int for dsp_dma_start */
  void  (*cont_xfer)(void);	/* Function to call to continue a DMA xfer */
  char	*buf[2];		/* Two pointers to mono-page buffers */
  int	phys_buf[2];		/* Physical addresses for dsp buffers */
  sbFLAG	full[2];		/* True when that buffer is full */
  int	used[2];		/* Buffer bytes used by read/write */
  sbBYTE  active;			/* The buffer currently engaged in DMA */
  sbBYTE  hi;			/* The buffer being filled/emptied by user */
  int	bufsize;		/* DMA transfer buffer size */
  int	last_xfer_size;		/* How many bytes we last transfered */
  int   overrun_errno;		/* errno code for a DSP overrun (0 to ignore)*/
  int	ver_major, ver_minor;	/* Major and Minor version number */
  sbBYTE	state;			/* Either READ_READY, WRITE_READY, or 0 */
};
static struct sb_dsp_type dsp;	/* 'dsp' structure used everywhere */

#ifdef __bsdi__
struct sb_softc {
    struct  device  sc_dev;     /* Base Device */
    struct  isadev  sc_id;      /* ISA Device */
    struct  intrhand  sc_ih;    /* Interrupt Handler */
};
#endif

/* This will be used as a global holder for the current Sound Blaster
   general status values. */
struct sb_status_type
{
  sbFLAG	dsp_in_use;		/* DSP or MIDI open for reading or writing */
  sbFLAG  fm_in_use;		/* FM open for ioctls */
  sbFLAG  cms_in_use;		/* CMS open for ioctls */
  
  sbFLAG  alive;                  /* Card present? */
  unsigned int addr;		/* Sound Blaster card address */
  int  *wake[NUM_UNITS];	/* What to wakeup on interrupt */

  /* Select handler stuff, one entry for each "device" */
#ifdef __bsdi__
  struct selinfo sel[SB_BIGGEST_MINOR];
#else
  struct proc *selects[SB_BIGGEST_MINOR];
  sbFLAG sel_coll[SB_BIGGEST_MINOR];
#endif
  struct proc *sig_proc[SB_BIGGEST_MINOR];
  int flags[SB_BIGGEST_MINOR];	/* fcntl flags */
};
static struct sb_status_type status; /* Global current status */

/*
 * Here is the screwey register ordering for the first operator
 * of each FM cell.  The second operators are all +3 from the first
 * operator of the same cell.
 */
static const char fm_op_order[] =
{ 0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12 };

#ifdef __bsdi__
#define sb_probe sbprobe
#define sb_attach sbattach
#define sb_open sbopen
#define sb_close sbclose
#define sb_read sbread
#define sb_write sbwrite
#define sb_ioctl sbioctl
#define sb_select sbselect
#define dev_addr id_iobase
#define sploff splhigh
#define splon splx
#endif
/*
 * Forward declarations galore!
 */
#ifdef __bsdi__
int sb_probe ( struct device *parent, struct cfdata *cf, void *aux );
void sb_attach ( struct device *parent, struct device *dev, void *aux );
int sb_intr ( struct sb_softc *sc );
int sb_open ( dev_t dev, int flags, int fmt, struct proc *p );
int sb_close ( dev_t dev, int flags, int fmt, struct proc *p );
int sb_ioctl ( dev_t dev, int cmd, caddr_t arg, int mode, struct proc
              *p );
int sb_read ( dev_t dev, struct uio *uio, int flag );
int sb_write ( dev_t dev, struct uio *uio, int flag );
int sb_select (int dev, int rw, struct proc *p);
#else
int sb_probe (struct isa_dev *dev);
int sb_attach (struct isa_dev *dev);
int sb_intr (int unit);
int sb_open (dev_t dev, int flags);
int sb_close (dev_t dev, int flags);
int sb_ioctl (dev_t dev, int cmd, caddr_t arg, int mode);
int sb_read (int dev, struct uio *uio);
int sb_write (int dev, struct uio *uio);
int sb_select (int dev, int rw);
#endif
static void sb_sendb (unsigned select_addr, sbBYTE reg,
		      unsigned data_addr, sbBYTE value);
static int dsp_reset (void);
static int dsp_get_version (void);
#ifdef __bsdi__
static int dsp_open (struct proc *);
#define dsp_procargs struct proc *procp
#else
static int dsp_open (void);
#define dsp_procargs void
#define procp /**/
#endif
static int dsp_close (void);
static int dsp_set_speed (int *speed);
static int dsp_command (sbBYTE val);
static int dsp_ioctl (int cmd, caddr_t arg);
static int dsp_set_voice (int on);
static int dsp_set_bufsize (int *newsize);
static int dsp_flush_dac (void);
static int dsp_set_compression (int mode);
static int dsp_set_stereo (sbFLAG on);
static int dsp_write (struct uio *uio);
static int dsp_read (struct uio *uio);
static int dsp_select (int rw, struct proc *);
static void dsp_find_buffers (void);
inline static void dsp_dma_start (int dir);
inline static void dsp_next_write (void);
inline static void dsp_next_read (void);
static int fm_open (void);
static int fm_close (void);
static int fm_reset (void);
static int fm_send (int channel, unsigned char reg, unsigned char val);
static int fm_ioctl (int cmd, caddr_t arg);
static int fm_delay (int pause);
static int fm_play_note (struct sb_fm_note *n);
static int fm_set_params (struct sb_fm_params *p);
static int fm_write (struct uio *uio);
static int mixer_read (struct uio *uio);
static int mixer_write (struct uio *uio);
static void mixer_send (sbBYTE reg, sbBYTE val);
static sbBYTE mixer_read_reg (sbBYTE reg);
static int mixer_ioctl (int cmd, caddr_t arg);
static int mixer_reset (void);
static int mixer_set_levels (struct sb_mixer_levels *l);
static int mixer_set_params (struct sb_mixer_params *p);
static int mixer_get_levels (struct sb_mixer_levels *l);
static int mixer_get_params (struct sb_mixer_params *params);
static int midi_open (void);
static int midi_close (void);
static int midi_read (struct uio *uio);
static int midi_write (struct uio *uio);
static int midi_ioctl (int cmd, caddr_t arg);
static int midi_select (int rw, struct proc *procp);


/*
 * This page has the required driver functions for the kernel.
 *
 * This is your basic set of functions that the BSD kernel needs.
 * Nothing really Sound Blaster specific here.
 */
#ifdef __bsdi__
struct cfdriver sbcd = 
{
    NULL, "sb", sbprobe, sbattach, sizeof ( struct sb_softc ) 
};
#else
int (*sb_intrs[])() = { sb_intr, 0 };
struct isa_driver sb_driver = { sb_probe, 0, sb_attach, "sb", 0, 0, 0 };
#endif


#ifdef __bsdi__
/* 
 * The gamma version of BSD/386 really changed around the probe and
 * attach functions.  I went ahead and re-wrote them rather than try
 * to mess around with getting them merged into a single routing.
 * The non-BSD/386 routine follows this one.  SMM
 */
int
sb_probe ( struct device *parent, struct cfdata *cf, void *aux )
{
    register struct isa_attach_args *ia = ( struct isa_attach_args *)
        aux;
    
    status.addr = ( unsigned int ) ia->ia_iobase;
    if ( dsp_reset () == GOOD )
        status.alive = 1;
    else 
        status.alive = 0;

    if ( ia->ia_irq == IRQUNK )
        return ( 0 );  /* Must have IRQ defined. */
    if (ia->ia_drq != SB_DMA_CHAN) {
        printf("sb0: drq %d not set to %d\n",
	    ia->ia_drq, SB_DMA_CHAN);
        return (0);
    }
    ia->ia_iosize = SB_NPORT;
    return ( status.alive );
}

#else  /* NOT BSDI */

/* 
 * Probing sets dev->dev_alive, status.alive, and returns 1 if the
 * card is detected.  Note that we are not using the "official" Adlib
 * of checking for the presence of the card because it's tacky and
 * takes too much mucking about.  We just attempt to reset the DSP and
 * assume that a DSP-READY signal means the card is there.  If you
 * have another card that fools this probe, I'd really like to hear
 * about it. :-) Future versions may query the DSP version number
 * instead.
 */
int
sb_probe (struct isa_dev *dev)
{
#ifdef FULL_DEBUG
  printf ("sb: sb_probe() called.\n");
  printf ("sb: unit=%d, ctlr=%d, slave=%d, alive=%d, addr=0x%x, spl=%d\n",
	  dev->dev_unit, dev->dev_ctlr, dev->dev_slave, dev->dev_alive,
	  dev->dev_addr, dev->dev_spl);
  printf ("sb: pic=%d  dk=%d  flags=%d  start=0x%x  len=%d  type=%d\n",
	  dev->dev_pic, dev->dev_dk, dev->dev_flags, dev->dev_start,
	  dev->dev_len, dev->dev_type);
#endif

  status.addr = (unsigned int) dev->dev_addr;

  if (dsp_reset () == GOOD)
    status.alive = 1;
  else
    {
      status.alive = 0;
      printf ("sb: Sound Blaster Not found!  Driver not installed.\n");
    }
  dev->dev_alive = status.alive;
  return (status.alive);
}

#endif  /* NOT BSDI */

#ifdef __bsdi__
/*
 * The attach routine is so different in BSDI BSD/386 that I just
 * rewrote a function rather than try to merge it in.  SMM
 */

void
sb_attach ( struct device *parent, struct device *dev, void *aux )
{
  int i;
  register struct isa_attach_args *ia = ( struct isa_attach_args *)
      aux;
  register struct sb_softc *sc = ( struct sb_softc *) dev;
  
  dsp_get_version ();
  printf ("%s: v%d.%02d\n", /* %02d is hosed? */
	  ia->ia_drq == 0 ? " drq 0" : "",
	  dsp.ver_major, dsp.ver_minor);

  status.addr = ia->ia_iobase;
  
#if 0
  status.fm_in_use = 0;
  status.dsp_in_use = 0;

  for (i = 0; i < NUM_UNITS; i++)
    status.wake[i] = NULL;
#endif

  /*
   * These are default startup settings.
   * I'm wondering if I should just leave the card in the same
   * speed/stereo state that it is in.  I decided to leave the mixer
   * alone, and like that better.  Ideas?
   */
  dsp.compression = PCM_8;
  dsp.speed = INITIAL_DSP_SPEED;
  dsp_set_stereo (FALSE);
  dsp.bufsize = DSP_MAX_BUFSIZE;
  /* mixer_reset(); */

  isa_establish ( &sc->sc_id, &sc->sc_dev );
  sc->sc_ih.ih_fun = sb_intr;
  sc->sc_ih.ih_arg = (void *) sc;
  intr_establish ( ia->ia_irq, &sc->sc_ih, DV_TAPE );

  sb_meminit();
}

#else  /* NOT BSDI */

/* This is called by the kernel if probe() is successful. */
int
sb_attach (struct isa_dev *dev)
{
  int i;

  take_dev_irq (dev);		/* Set up DSP interrupt */

  dsp_get_version ();
  printf ("sb: Sound Blaster v%d.%02d installed. ", /* %02d is hosed? */
	  dsp.ver_major, dsp.ver_minor);
  printf ("(Port 0x%x, IRQ %d, DMA %d)\n",
	  status.addr, dev->dev_pic, SB_DMA_CHAN);
  if (dsp.ver_major != EXPECTED_DSP_VER)
    {
      printf ("sb: SB Driver is MISCONFIGURED.\n"
	      "sb: You do not have a DSP version %d.x.\n", EXPECTED_DSP_VER);
      status.alive = FALSE;
    }

  status.fm_in_use = 0;
  status.dsp_in_use = 0;
  dsp_find_buffers();
  for (i = 0; i < NUM_UNITS; i++)
    status.wake[i] = NULL;

  for (i = 0; i <= SB_BIGGEST_MINOR; i++)
    {
      status.sig_proc[i] = NULL;
      status.selects[i] = NULL;
      status.sel_coll[i] = 0;
    }

  /*
   * These are default startup settings.
   * I'm wondering if I should just leave the card in the same
   * speed/stereo state that it is in.  I decided to leave the mixer
   * alone, and like that better.  Ideas?
   */
  dsp.compression = PCM_8;
  dsp.speed = INITIAL_DSP_SPEED;
  dsp_set_stereo (FALSE);
  dsp.bufsize = DSP_MAX_BUFSIZE;
  /* mixer_reset(); */
  return (ESUCCESS);
}

#endif  /* NOT BSDI */

/*
 * This gets called anytime there is an interrupt on the SB IRQ.
 * Great effort is made to service DSP interrupts immediately
 * to keep the buffers flowing and pops small.  I wish the card
 * had a bigger internal buffer so we didn't have to worry.
 */ 
int
#ifdef __bsdi__
sb_intr ( struct sb_softc *sc )
#else
sb_intr (int unit)
#endif
{
#ifdef __bsdi__
    int unit = sc->sc_dev.dv_unit;
#endif

  int i;
  DPRINTF (("sb_intr(): Got interrupt on unit %d\n", unit));

  /*
   * If the DSP is doing a transfer, and the interrupt was on the
   * DSP unit (as opposed to, say, the FM unit), then hurry up and
   * call the function to continue the DSP transfer.
   */
  if (dsp.cont_xfer && unit == DSP_UNIT)
    (*dsp.cont_xfer)();

  /*
   * If any selects are pending on any devices, wake them up
   * and have them check for data ready.  Also send out ASYNC
   * signals to anyone who asked for them.
   * Of course, this makes the blatant assumption that only one
   * interrupt-generating device will be open at any time, since
   * we can't figure out what caused the SB to interrupt.
   * (Ahhh, I love that ISA bus..)
   */
  for (i = 0; i <= SB_BIGGEST_MINOR; i++)
    {
#ifdef __bsdi__
      selwakeup(&status.sel[i]);
#else
      if (status.selects[i])
	{
	  DPRINTF (("Waking up selecters..\n"));
	  selwakeup (status.selects[i], status.sel_coll[i]);
	  status.selects[i] = 0;
	  status.sel_coll[i] = FALSE;
	}
#endif
      /*
       * I'm not sure if this is the right time to send out signals.
       * Perhaps more checking should be done to make sure the process
       * is really interested in this interrupt.  I don't use ASYNC much.
       */
      if (status.sig_proc[i])
	psignal (status.sig_proc[i], SIGIO);
    }
  
  /*
   * If this Unit is expecting an interrupt, then set the semaphore
   * and wake up the sleeper.  Otherwise, we probably weren't expecting
   * this interrupt, so just ignore it.  This also happens when two
   * interrupts are acked in a row, without sleeping in between.
   * Not really a problem, but there should be a cleaner way to fix it.
   */
  if (status.wake[unit] != NULL)
    {
      *status.wake[unit] = WOKEN_BY_INTERRUPT;
      wakeup (status.wake[unit]);
      status.wake[unit] = NULL;
    }
  else
    {
      DPRINTF (("sb_intr(): An ignored interrupt.\n"));
    }
#ifdef __bsdi__
  return 1;
#else
  return (ESUCCESS);
#endif
}

/* Multiplexes the open between all the different minor numbers. */
int
sb_open (dev_t dev, int flags
#ifdef __bsdi__
, int mode, struct proc *procp
#endif
)
{
  if (!status.alive)
    return (ENODEV);

#ifdef FULL_DEBUG
  printf("open(): dev = 0x%x, flags = 0x%x\n", dev, flags);
#endif

  /* Remember the flags for future reference */
  status.flags[minor(dev)] = flags;
  status.sig_proc[minor(dev)] = NULL;
#ifndef __bsdi__
  status.selects[minor(dev)] = NULL;
  status.sel_coll[minor(dev)] = 0;
#endif

  switch (minor (dev))
    {
    case SB_DSP_NUM:
      return (dsp_open (procp));
    case SB_FM_NUM:
      return (fm_open ());
    case SB_MIDI_NUM:
      return (midi_open ());
    case SB_MIXER_NUM:
      return (ESUCCESS);
      
    case SB_CMS_NUM:		/* No CMS support (yet) */
    default:
      return (EINVAL);
    }
}

int
#ifdef __bsdi__
sb_close ( dev_t dev, int flags, int fmt, struct proc *p )
#else
sb_close (dev_t dev, int flags)
#endif
{
#ifdef FULL_DEBUG
  printf ("close(): dev = 0x%x, flags = 0x%x\n", dev, flags);
#endif

  switch (minor (dev))
    {
    case SB_DSP_NUM:
      return (dsp_close ());
    case SB_FM_NUM:
      return (fm_close ());
    case SB_MIDI_NUM:
      return (midi_close ());
    case SB_MIXER_NUM:
      return (ESUCCESS);

    case SB_CMS_NUM:
    default:
      printf ("sb: How did you close() a device you can't open()??");
      return (ESRCH);
    }
}

/*
 * This routes all the ioctl calls to the right place.
 * In general, Mixer ioctls should be allowed on any of the devices,
 * because it makes things easier and is guaranteed hardware-safe.
 */
int
#ifdef __bsdi__
sb_ioctl ( dev_t dev, int cmd, caddr_t arg, int mode, struct proc
              *p )
#else
sb_ioctl (dev_t dev, int cmd, caddr_t arg, int mode)
#endif
{
  if (!status.alive)
    return (ENODEV);

  switch (cmd)
    {
#ifdef ALWAYS_ALLOW_MIXER_IOCTLS
    case MIXER_IOCTL_RESET:
    case MIXER_IOCTL_SET_LEVELS:
    case MIXER_IOCTL_SET_PARAMS:
    case MIXER_IOCTL_READ_LEVELS:
    case MIXER_IOCTL_READ_PARAMS:
      return (mixer_ioctl (cmd, arg));
#endif
      /*
       * No support for these yet.  Soon..
       * For now, you just have to open the device with the right flags.
       * As soon as I figure out fcntl() calls, this will work.
       */
    case F_SETFL:
      DPRINTF (("Got F_SETFL ioctl.\n"));
      return (EINVAL);
    case F_GETFL:
      DPRINTF (("Got F_GETFL ioctl.\n"));
      return (EINVAL);
      
    default:
      switch (minor (dev))
	{
	case SB_DSP_NUM:
	  return (dsp_ioctl (cmd, arg));
	case SB_FM_NUM:
	  return (fm_ioctl (cmd, arg));
	case SB_MIXER_NUM:
	  return (mixer_ioctl (cmd, arg));
	case SB_MIDI_NUM:
	  return (midi_ioctl (cmd, arg));
	case SB_CMS_NUM:
	default:
	  printf ("sb: sb_ioctl(): Invalid minor number!\n");
	  return (EINVAL);
	}
    }      
  return (ESUCCESS);
}

/* Multiplexes a read to the different devices */
int
#ifdef __bsdi__
sb_read ( dev_t dev, struct uio *uio, int flag )
#else
sb_read (int dev, struct uio *uio)
#endif
{
  if (!status.alive)
    return (ENODEV);

  switch (minor(dev))
    {
    case SB_DSP_NUM:
      return (dsp_read (uio));
    case SB_MIDI_NUM:
      return (midi_read (uio));
    case SB_MIXER_NUM:
      return (mixer_read (uio));

    case SB_FM_NUM:		/* No reading from FM, right? */
      return (ENXIO);
    case SB_CMS_NUM:
    default:
      printf ("sb: sb_read() got bad dev!\n");
      return (ENXIO);
    }
}

int
#ifdef __bsdi__
sb_write ( dev_t dev, struct uio *uio, int flag )
#else
sb_write (int dev, struct uio *uio)
#endif
{
  if (!status.alive)
    return (ENODEV);

  DPRINTF (("sb: sb_write(): dev = 0x%x\n", dev));
  switch (minor (dev))
    {
    case SB_DSP_NUM:
      return (dsp_write (uio));
    case SB_FM_NUM:
      return (fm_write (uio));
    case SB_MIXER_NUM:
      return (mixer_write (uio));
    case SB_MIDI_NUM:
      return (midi_write (uio));

    case SB_CMS_NUM:
    default:
      printf ("sb: sb_write() on bad dev.  Serious probs.\n"); /* ??? */
      return (0);
    }
}

int
sb_select (int dev, int rw
#ifdef __bsdi__
	   , struct proc *procp
#endif
	   )
{
#ifndef __bsdi__
    struct proc *procp = current_thread(); /* eeyuck */
#endif

  if (!status.alive)
    return (ENODEV);
  DPRINTF (("sb: sb_select: dev = 0x%x\n", dev));
  switch (minor (dev))
    {
    case SB_DSP_NUM:
      return (dsp_select (rw, procp));
    case SB_MIDI_NUM:
      return (midi_select (rw, procp));
    default:
#if 0
      printf ("sb: sb_select() on bad dev.\n");
#endif
      return (1);
    }
}

/*
 * Send one byte to the named Sound Blaster register.
 * The SBDK recommends 3.3 microsecs after an address write,
 * and 23 after a data write.  What they don't tell you is that
 * you also can't READ from the ports too soon, or crash! (Same timing?)
 * Anyway, 10 usecs is as close as we can get, so..
 *
 * NOTE:  This function is wicked.  It ties up the entire machine for
 * over forty microseconds.  This is unacceptable, but I'm not sure how
 * to make it better.  Look for a re-write in future versions.
 * Does 30 microsecs merit a full timeout() proceedure?
 */
static void
sb_sendb (unsigned int select_addr, sbBYTE reg,
	  unsigned int data_addr, sbBYTE value)
{
  outb (select_addr, reg);
  tenmicrosec();
  outb (data_addr, value);
  tenmicrosec();
  tenmicrosec();
  tenmicrosec();
}

#ifndef __bsdi__
/*
 * This gets called when a sleep() times out,
 * and sets the semaphore appropriately.
 */
static void
timeout_wakeup (int *arg)
{
  *arg = WOKEN_BY_TIMEOUT;
  wakeup (arg);
}
#endif

/*
 * This waits for an interrupt on the named Unit.
 * The 'patience' is how long to wait (in cycles) before a timeout
 *
 * Always have interrupts masked while calling! (like w/ splhigh())
 */
static int
wait_for_intr (int unit, int patience, int priority)
{
  static int sem[NUM_UNITS] = {0};	/* implicitly all are zero */
  int result;

  DPRINTF (("Waiting for interrupt on unit %d.\n", unit));

  /* we CANNOT have the wakeup semaphore be on our local stack,
     since the interrupt may arise in another process context.
     However, since multiple sleepers on the same unit would have collided
     in the original code (and only one would be awoken), we just make it
     static, with one entry per unit.
     */
  status.wake[unit] = &sem[unit];

#ifndef __bsdi__
  if (patience)
    timeout (timeout_wakeup, &sem[unit], patience);
  sleep (&sem[unit], priority);

  if (sem[unit] == WOKEN_BY_TIMEOUT)
#else
  result = tsleep(&sem[unit], priority, "sbintrwait", patience);
  if (result == EWOULDBLOCK)
#endif
    {
      status.wake[unit] = NULL;		/* clean up */
      printf ("sb: Interrupt time out.\n");
      if (unit == DSP_UNIT)
	dsp.error = EIO;
      result = FAIL;
    }
  else
    {
      DPRINTF (("Got it!\n"));
#ifdef __bsdi__
      if (result == ERESTART)
	  return ERESTART;
#else
      untimeout (timeout_wakeup, &sem[unit]);
#endif
      result = GOOD;
    }
  return (result);
}


/*
 * DSP functions.
 */
static int
dsp_select (int rw, struct proc *procp)
{
  int s = splhigh();
  DPRINTF (("sb: dsp_select() called with rw = %d.\n", rw));
  if ((dsp.cont_xfer == NULL)
      || ((rw == SELECT_WRITE) && (dsp.active == dsp.hi))
      || ((rw == SELECT_READ) && (dsp.full[dsp.hi] == TRUE)))
    {
      splx (s);
      return (1);
    }
#ifdef __bsdi__
  selrecord(procp, &status.sel[SB_DSP_NUM]);
#else
  if (status.selects[SB_DSP_NUM])
    status.sel_coll[SB_DSP_NUM] = TRUE;
  else
    status.selects[SB_DSP_NUM] = procp;
#endif
  
  splx (s);
  return 0;
}

static int
dsp_ioctl (int cmd, caddr_t arg)
{
  switch (cmd)
    {
    case DSP_IOCTL_RESET:
      if (dsp_reset () == GOOD)
	return (ESUCCESS);
      else
	return (EIO);
    case DSP_IOCTL_OVERRUN_ERRNO:
      dsp.overrun_errno = (*(int *)arg);
      return (ESUCCESS);
    case DSP_IOCTL_BUFSIZE:
      return (dsp_set_bufsize ((int *)arg));
    case DSP_IOCTL_SPEED:
      return (dsp_set_speed ((int *)arg));
    case DSP_IOCTL_VOICE:
      return (dsp_set_voice (*(sbFLAG *)arg));
    case DSP_IOCTL_FLUSH:
      return (dsp_flush_dac ());
    case DSP_IOCTL_COMPRESS:
      return (dsp_set_compression (*(sbBYTE *)arg));
    case DSP_IOCTL_STEREO:
      return (dsp_set_stereo (*(sbFLAG *)arg));
    default:
      return (EINVAL);
    }
}

/*
 * Reset the DSP chip, and initialize all DSP variables back
 * to square one.  This can be done at any time to abort
 * a transfer and break out of locked modes. (Like MIDI UART mode!)
 * Note that resetting the DSP puts the speed back to 8196, but
 * it shouldn't matter because we set the speed in dsp_open.
 * Keep this in mind, though, if you use DSP_IOCTL_RESET from
 * inside a program.
 */
static int
dsp_reset (void)
{
  int i, s;

  DPRINTF (("Resetting DSP.\n"));
  s = splhigh();
  dsp.used[0] = dsp.used[1] = 0; /* This is only for write; see dsp_read() */
  dsp.full[0] = dsp.full[1] = FALSE;
  dsp.hi = dsp.active = 0;
  dsp.state = NONE_READY;
  dsp.error = ESUCCESS;
  dsp.cont_xfer = NULL;
  dsp.last_xfer_size = -1;
  status.wake[DSP_UNIT] = NULL;

  /*
   * This is how you reset the DSP, according to the SBDK:
   * Send 0x01 to DSP_RESET (0x226) and wait for three microseconds.
   * Then send a 0x00 to the same port.
   * Poll until DSP_RDAVAIL's most significant bit is set, indicating
   * data ready, then read a byte from DSP_RDDATA.  It should be 0xAA.
   * Allow 100 microseconds for the reset.
   */
  tenmicrosec();		/* Lets things settle down. (necessary?) */
  outb (DSP_RESET, 0x01);
  tenmicrosec();
  outb (DSP_RESET, 0x00);

  dsp.error = EIO;
  for (i = DSP_LOOP_MAX; i; i--)
    {
      tenmicrosec();
      if ((inb (DSP_RDAVAIL) & DSP_DATA_AVAIL)
	  && ((inb (DSP_RDDATA) & 0xFF) == DSP_READY))
	{
	  dsp.error = ESUCCESS;
	  break;
	}
    }
  splx (s);

  if (dsp.error != ESUCCESS)
    return (FAIL);
  else
    return (GOOD);
}

static int
dsp_get_version (void)
{
  int i;
  
  dsp.ver_major = dsp.ver_minor = 0;
  dsp_command (GET_DSP_VERSION); /* Request DSP version number */
  for (i = 10 * DSP_LOOP_MAX; i; i--)
    {
      if (inb (DSP_RDAVAIL) & DSP_DATA_AVAIL) /* wait for Data Ready */
	{
	  if (dsp.ver_major == 0)
	    dsp.ver_major = inb (DSP_RDDATA);
	  else
	    {
	      dsp.ver_minor = inb (DSP_RDDATA);
	      break;		/* All done */
	    }
	}
      tenmicrosec ();
    }

  if (i)			/* finished in time */
    return (GOOD);
  else
    {
      printf ("sb: Error reading DSP version number.\n");
      return (FAIL);
    }
}

#ifdef __bsdi__
sb_meminit(void)
{
    dsp_find_buffers();
}
#endif

/* 
 * This finds page-safe buffers for the DSP DMA to use.  A single DMA
 * transfer can never cross a 64K page boundary, so we have to get
 * aligned buffers for DMA.  The current method is wasteful, but
 * allows any buffer size up to the full 64K (which is nice).  We grab
 * 3 * 64K in the static global memory_chunk, and find the first 64K
 * alignment in it for the first buffer.  The second buffer starts 64K
 * later, at the next alignment.  Yeah, it's gross, but it's flexible.
 * I'm certainly open to ideas!  (Using cool kernel memory alloc is tricky
 * and not real portable.)
 */
static void
dsp_find_buffers (void)
{
#ifdef __bsdi__
    char *memory_chunk;
    register char *pagecheck;
    vm_offset_t realaddr;
    /*
     * to avoid problems with static buffers causing the boot loader
     * some heartburn, we attempt to allocate the buffers here,
     * hoping that they're physically contiguous.  This is called pretty
     * early in the memory allocator's life, so there's hopefully a
     * good chance.
     */
    memory_chunk = malloc(DSP_CHUNK_SIZE, M_DEVBUF, M_WAITOK);
    /* check that it's contiguous by stepping by pages */

    for (pagecheck = memory_chunk, realaddr = kvtophys(pagecheck);
	 pagecheck - memory_chunk < DSP_CHUNK_SIZE;
	 pagecheck += (page_mask+1)) {
	if (realaddr + (pagecheck - memory_chunk) != kvtophys(pagecheck)) {
	    printf("sb: noncontiguous memory\n");
	errout:
	    free(memory_chunk, M_DEVBUF);
	    dsp.buf[0] = 0;
	    dsp.buf[1] = 0;
	    return;
	}
    }
#endif
  dsp.buf[0] = memory_chunk + 0x10000 - (kvtophys (memory_chunk)
					 - (kvtophys (memory_chunk)
					    & ~0xFFFF));
  dsp.buf[1] = dsp.buf[0] + /*0x10000*/ DSP_MAX_BUFSIZE;
  
  if (crosses_64k_boundary (kvtophys(dsp.buf[0]), DSP_MAX_BUFSIZE)
      || crosses_64k_boundary (kvtophys(dsp.buf[1]), DSP_MAX_BUFSIZE)) {
#ifdef __bsdi__
      printf ("sb: Couldn't get good DSP buffers! (v: %x, %x;p: %x, %x)\n",
	      dsp.buf[0], dsp.buf[1],
	      kvtophys(dsp.buf[0]), kvtophys(dsp.buf[1]));
      goto errout;
#else
  if (crosses_64k_boundary (dsp.buf[0], DSP_MAX_BUFSIZE)
      || crosses_64k_boundary (dsp.buf[1], DSP_MAX_BUFSIZE))
    panic ("sb: Couldn't get good DSP buffers!\n");
#endif
  }
      
  dsp.phys_buf[0] = kvtophys(dsp.buf[0]);
  dsp.phys_buf[1] = kvtophys(dsp.buf[1]);
}


/*
 * Start a single-buffer DMA transfer to/from the DSP.
 * This one needs more explaining than I would like to put in comments,
 * so look at the accompanying documentation, which, of course, hasn't
 * been written yet. :-)
 *
 * This function takes one argument, 'dir' which is either PLAY or
 * RECORD, and starts a DMA transfer of as many bytes as are in
 * dsp.buf[dsp.active].  This allows for partial-buffers.
 *
 * Always call this with interrupts masked.
 */
inline static void
dsp_dma_start (int dir)
{
  int count = dsp.used[dsp.active] - 1;

  /* Prepare the DMAC.  See sb_regs for defs and more info. */
  outb (DMA_MASK_REG, DMA_MASK);
  outb (DMA_CLEAR, 0);
  outb (DMA_MODE, (dir == RECORD) ? DMA_MODE_READ : DMA_MODE_WRITE);
  outb (DMA_PAGE, (dsp.phys_buf[dsp.active] & 0xff0000) >> 16);	/* Page */
  outb (DMA_ADDRESS, LOWBYTE (dsp.phys_buf[dsp.active]));
  outb (DMA_ADDRESS, HIBYTE (dsp.phys_buf[dsp.active]));
  outb (DMA_COUNT, LOWBYTE(count));
  outb (DMA_COUNT, HIBYTE(count));
  outb (DMA_MASK_REG, DMA_UNMASK);

  /*
   * The DMAC is ready, now send the commands to the DSP.
   * Notice that there are two entirely different operations for
   * Low and High speed DSP.  With HS, You only have to send the
   * byte-count when it changes, and that requires an extra command
   * (Checking if it's a new size is quicker than always sending it.)
   */
  if (dsp.hispeed)
    {
      DPRINTF (("Starting High-Speed DMA of %d bytes to/from buffer %d.\n",
		dsp.used[dsp.active], dsp.active));

      if (count != dsp.last_xfer_size)
	{
	  dsp_command (TRANSFER_SIZE);
	  dsp_command (LOWBYTE (count));
	  dsp_command (HIBYTE (count));
	  dsp.last_xfer_size = count;
	}
      dsp_command (dsp.start_command); /* GO! */
    }
  else				/* Low Speed transfer */
    {
      DPRINTF (("Starting Low-Speed DMA xfer of %d bytes to/from buffer %d.\n",
		dsp.used[dsp.active], dsp.active));
      dsp_command (dsp.start_command);
      dsp_command (LOWBYTE (count));
      dsp_command (HIBYTE (count)); /* GO! */
    }

  /* This sets up the function to call at the next interrupt: */
  dsp.cont_xfer = (dir == RECORD) ? dsp_next_read : dsp_next_write;
}

/*
 * This is basically the interrupt handler for Playback DMA interrupts.
 * Our first priority is to get the other buffer playing, and worry
 * about the rest after.
 */
inline static void
dsp_next_write (void)
{
  int s = sploff ();		/* Faster than splhigh */

  inb (DSP_RDAVAIL);		/* ack interrupt */
  DPRINTF (("Got interrupt.  DMA done on buffer %d.\n", dsp.active));
  dsp.full[dsp.active] = FALSE; /* Just-played buffer is not full */
  dsp.active ^= 1;
  if (dsp.full[dsp.active])
    {
      DPRINTF (("Starting next buffer off..\n"));
      dsp_dma_start (PLAY);
    }
  else
    {
      DPRINTF (("Other buffer is not full.  Clearing cont_xfer..\n"));
      dsp.cont_xfer = NULL;
    }
  splon (s);
}

/*
 * This is similar to dsp_next_write(), but for Sampling instead of playback.
 */
inline static void
dsp_next_read (void)
{
  int s = sploff ();

  inb (DSP_RDAVAIL);		/* ack interrupt */
  /* The active buffer is currently full of samples */
  dsp.full[dsp.active] = TRUE;
  dsp.used[dsp.active] = 0;

  /* Flop to the next buffer and fill it up too, unless it's already full */
  dsp.active ^= 1;
  if (dsp.full[dsp.active])
    {
      /*
       * An overrun occurs when we fill up two buffers faster than the
       * user is reading them.  Lossage has to occur.  This may or
       * may not be bad.  For recording, it's bad.  For real-time
       * FFTs and such, it's not a real big deal.
       */
      dsp.error = dsp.overrun_errno;
      dsp.cont_xfer = NULL;
    }
  else
    dsp_dma_start (RECORD);
  splon (s);
}

/*
 * This is the main recording function.  Program flow is tricky with
 * all the double-buffering and interrupts, but such are drivers.
 * To summarize, it starts filling the first buffer when the user
 * requests the first read().  (Filling on the open() call would be silly.)
 * When one buffer is done filling, we fill the other one while copying
 * the fresh data to the user.
 * If the user doesn't read fast enough for that DSP speed, we have an
 * overrun.  See above concerning overrun_errno.
 */
static int
dsp_read (struct uio *uio)
{
  int bytecount, hunk_size, s, chunk;

  if (dsp.state != READ_READY)
    {
      if (!(status.flags[SB_DSP_NUM] & FREAD))
	return (EINVAL);
      dsp_flush_dac ();		/* In case we were writing stuff */

      s = splhigh ();
      dsp_set_voice (OFF);
      dsp_set_speed (&dsp.speed); /* Set SB to the current speed. */
      dsp.state = READ_READY;
      DPRINTF (("Kicking in first DSP read..\n"));
      dsp.start_command = dsp.hispeed ? HS_ADC_8 : ADC_8;

      /* A DSP state reset, without the hardware reset. */
      dsp.used[0] = dsp.used[1] = dsp.bufsize; /* Start with both empty */
      dsp.full[0] = dsp.full[1] = FALSE;
      dsp.hi = dsp.active = 0;
      dsp.error = ESUCCESS;
      dsp.last_xfer_size = -1;
      status.wake[DSP_UNIT] = NULL;
      dsp_dma_start (RECORD);
      splx (s);
    }

  /* For each uio_iov[] entry... */
  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      bytecount = 0;

      /* While there is still data to read, and data in this chunk.. */
      while (uio->uio_resid && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  if (dsp.error != ESUCCESS)
	    return (dsp.error);
	  
	  s = splhigh ();
	  while (dsp.full[dsp.hi] == FALSE)
	    {
	      DPRINTF (("Waiting for buffer %d to fill..\n", dsp.hi));
	      wait_for_intr (DSP_UNIT, dsp.timeout, INTR_PRIORITY);
	    }
	  splx (s);

	  /* Now we give a piece of the buffer to the user */
	  hunk_size = MIN (uio->uio_iov[chunk].iov_len - bytecount,
			   dsp.bufsize - dsp.used[dsp.hi]);

	  DPRINTF (("Copying %d bytes from buffer %d.\n", hunk_size, dsp.hi));
	  if (copyout (dsp.buf[dsp.hi] + dsp.used[dsp.hi], 
		       uio->uio_iov[chunk].iov_base + bytecount, hunk_size))
	    {
	      printf ("sb: Bad copyout()!\n");
	      return (ENOTTY);
	    }
	  dsp.used[dsp.hi] += hunk_size;

	  if (dsp.used[dsp.hi] == dsp.bufsize)
	    {
	      DPRINTF (("Drained all of buffer %d.\n", dsp.hi));
	      dsp.full[dsp.hi] = FALSE;
	      dsp.hi ^= 1;
	      dsp.used[dsp.hi] = 0;
	      if (dsp.cont_xfer == NULL) /* We had an overrun recently */
		{
		  DPRINTF (("Caught up after overrun, starting DMA..\n"));
		  s = splhigh ();
		  dsp_dma_start (RECORD);
		  splx (s);
		}
	    }
	  bytecount += hunk_size;
	}			/* While there are bytes left in chunk */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (dsp.error);
}

/* 
 * Main function for DSP sampling.
 * No such thing as an overrun here, but if the user writes too
 * slowly, we have to restart the DSP buffer ping-pong manually.
 * There will then be gaps, of course.
 */
static int
dsp_write (struct uio *uio)
{
  int bytecount, hunk_size, s, chunk;

  if (dsp.state != WRITE_READY)
    {
      if (!(status.flags[SB_DSP_NUM] & FWRITE))
	return (EINVAL);
      dsp_reset ();
      dsp_set_speed (&dsp.speed); /* Set SB back to the current speed. */
      dsp_set_voice (ON);
      dsp.state = WRITE_READY;
    
      /*
       * If this is the first write() operation for this open(),
       * then figure out the DSP command to use.
       * Have to check if it is High Speed, or one of the compressed modes.
       * If this is the first block of a Compressed sound file,
       * then we have to set the "Reference Bit" in the dsp.command for the
       * first block transfer.
       */
      if (dsp.hispeed)
	{
	  dsp.start_command = HS_DAC_8;
	}
      else
	{
	  dsp.start_command = dsp.compression;
	  if (dsp.compression == ADPCM_4
	      || dsp.compression == ADPCM_3
	      || dsp.compression == ADPCM_2)
	    dsp.start_command |= 1;
	}
    }

  /* For each uio_iov[] entry... */
  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      bytecount = 0;

      /* While there is still data to write, and data in this chunk.. */
      while (uio->uio_resid && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  if (dsp.error != ESUCCESS) /* Shouldn't happen */
	    return (dsp.error);

	  hunk_size = MIN (uio->uio_iov[chunk].iov_len - bytecount,
			   dsp.bufsize - dsp.used[dsp.hi]);
	  DPRINTF (("Adding %d bytes (%d) to buffer %d.\n",
		    hunk_size, dsp.used[dsp.hi], dsp.hi));
	  if (copyin (uio->uio_iov[chunk].iov_base + bytecount,
		      dsp.buf[dsp.hi] + dsp.used[dsp.hi], hunk_size)) 
	    {
	      printf ("sb: Bad copyin()!\n");
	      return (ENOTTY);
	    }
	  dsp.used[dsp.hi] += hunk_size;

	  if (dsp.used[dsp.hi] == dsp.bufsize)
	    {
	      s = splhigh ();
	      dsp.full[dsp.hi] = TRUE;
	      DPRINTF (("Just finished filling buffer %d.\n", dsp.hi));

	      /*
	       * This is true when there are no DMA's currently
	       * active.  This is either the first block, or the
	       * user is slow about writing.  Start the chain reaction.
	       */
	      if (dsp.cont_xfer == NULL)
		{
		  DPRINTF (("Jump-Starting a fresh DMA...\n"));
		  dsp.active = dsp.hi;
		  dsp_dma_start (PLAY);
		  if (!dsp.hispeed)
		    dsp.start_command &= ~1; /* Clear reference bit */
		  status.wake[DSP_UNIT] = &dsp.semaphore;
		}
	      
	      /* If the OTHER buffer is playing, wait for it to finish. */
	      if (dsp.active == dsp.hi ^ 1)
		{
		  DPRINTF (("Waiting for buffer %d to empty.\n", dsp.active));
		  wait_for_intr (DSP_UNIT, dsp.timeout, INTR_PRIORITY);
		}
	      dsp.hi ^= 1;	/* Switch to other buffer */
	      dsp.used[dsp.hi] = 0; /* Mark it as empty */
	      splx (s);
	      DPRINTF (("Other buffer (%d) is empty, continuing..\n", dsp.hi));
	    }			/* if filled hi buffer */
	  bytecount += hunk_size;
	}			/* While there are bytes left in chunk */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (dsp.error);
}

/*
 * Play any bytes in the last waiting write() buffer and wait
 * for all buffers to finish transferring.
 * An even number of bytes is forced to keep the stereo channels
 * straight.  I don't think you'll miss one sample.
 */
static int
dsp_flush_dac (void)
{
  int s;
  
  DPRINTF (("Flushing last buffer(s).\n"));
  s = splhigh ();
  if (dsp.used[dsp.hi] != 0)
    {
      DPRINTF (("Playing the last %d bytes.\n", dsp.used[dsp.hi]));
      if (dsp.in_stereo)
	dsp.used[dsp.hi] &= ~1;	/* Have to have even number of bytes. */
      dsp.full[dsp.hi] = TRUE; 
      if (dsp.cont_xfer == NULL)
	{
	  dsp.active = dsp.hi;
	  dsp_dma_start (PLAY);
	}
    }

  /* Now wait for any transfers to finish up. */
  while (dsp.cont_xfer)
    {
      DPRINTF (("Waiting for last DMA(s) to finish.\n"));
      wait_for_intr (DSP_UNIT, dsp.timeout, INTR_PRIORITY);
    }
  dsp.used[0] = dsp.used[1] = 0;
  splx (s);
  return (ESUCCESS);
}


static int
dsp_open (dsp_procargs)
{
#ifndef __bsdi__
  struct proc *procp = u.u_procp;
#endif

  if (status.dsp_in_use)
    return (EBUSY);

  if (!dsp.buf[0])
      return(ENOMEM);			/* couldn't allocate memory;
					   don't let it be used. */

#ifndef __bsdi__
  status.selects[SB_DSP_NUM] = NULL;
  status.sel_coll[SB_DSP_NUM] = FALSE;
#endif
  status.dsp_in_use = TRUE;

  /* Take note of the process if they want asynchronous signals */
  if (status.flags[SB_DSP_NUM] & FASYNC)
    status.sig_proc[SB_DSP_NUM] = procp;
  
  dsp_reset ();			/* Resets card and inits variables */
  dsp.overrun_errno = EIO;	/* Until we are told otherwise */
  dsp.state = NONE_READY;	/* Not ready for either reading or writing */

#ifdef FULL_DEBUG
  /* Stuff buffers with loud garbage so we can hear/see leaks. */
  for (i = 0; i < dsp.bufsize; i++)
    dsp.buf[0][i] = dsp.buf[1][i] = i & 0xff;
#endif
  return (ESUCCESS);
}


static int
dsp_close (void)
{
  if (status.dsp_in_use)
    {
      /* Wait for any last write buffers to empty  */
      if (dsp.state == WRITE_READY)
	dsp_flush_dac ();
      dsp_reset ();
      status.dsp_in_use = FALSE;
      return (ESUCCESS);
    }
  else
    return (ESRCH);		/* Does this ever happen? */
}


/*
 * Set the playback/recording speed of the DSP.
 * This takes a pointer to an integer between DSP_MIN_SPEED
 * and DSP_MAX_SPEED and changes that value to the actual speed
 * you got. (Since the speed is so darn granular.)
 * This also sets the dsp.hispeed flag appropriately.
 * Note that Hi-Speed and compression are mutually exclusive!
 * I also don't check all the different range limits that
 * compression imposes.  Supposedly, the DSP can't play compressed
 * data as fast, but that's your problem.  It will just run slower.
 * Hmmm.. that could cause interrupt timeouts, I suppose.
 */
static int
dsp_set_speed (int *speed)
{
  sbBYTE time_constant;

  if (*speed == -1)		/* Requesting current speed */
    {
      *speed = dsp.speed;
      return (ESUCCESS);
    }
  
  if (*speed < DSP_MIN_SPEED)
    *speed = DSP_MIN_SPEED;
  if (*speed > DSP_MAX_SPEED)
    *speed = DSP_MAX_SPEED;

  if (*speed > MAX_LOW_SPEED)
    {
      if (dsp.compression != PCM_8)
	return (EINVAL);
      DPRINTF (("Using HiSpeed mode.\n"));
      time_constant = (sbBYTE) ((65536 - (256000000 / *speed)) >> 8);
      dsp.speed = (256000000 / (65536 - (time_constant << 8)));
      dsp.hispeed = TRUE;
    }
  else
    {
      DPRINTF (("Using LowSpeed mode.\n"));
      time_constant = (sbBYTE) (256 - (1000000 / *speed));
      dsp.speed = 1000000 / (256 - time_constant);
      dsp.hispeed = FALSE;
    }

  /* Here is where we actually set the card's speed */
  if (dsp_command (SET_TIME_CONSTANT) == FAIL
      || dsp_command (time_constant) == FAIL)
    return (EIO);
  /*
   * Replace speed with the speed we actually got.
   * Set the DSP timeout to be twice as long as a full
   * buffer should take.
   */
  *speed = dsp.speed;
  COMPUTE_DSP_TIMEOUT;
  DPRINTF (("Speed set to %d.\n", dsp.speed));
  return (ESUCCESS);
}


/*
 * Turn the DSP output speaker on and off.
 * Argument of zero turns it off, on otherwise
 */
static int
dsp_set_voice (int on)
{
  if (dsp_command (on ? SPEAKER_ON : SPEAKER_OFF) == GOOD)
    return (ESUCCESS);
  else
    return (EIO);
}

/*
 * Set the DAC hardware decompression mode.
 * No compression allowed for Hi-Speed mode, of course.
 */
static int
dsp_set_compression (int mode)
{
  switch (mode)
    {
    case ADPCM_4:
    case ADPCM_3:
    case ADPCM_2:
      if (dsp.hispeed)
	return (EINVAL);      /* Fall through.. */
    case PCM_8:
      dsp.compression = mode;
      return (ESUCCESS);
      
    default:
      return (EINVAL);
    }
}

/*
 * Send a command byte to the DSP port.
 * First poll the DSP_STATUS port until the BUSY bit clears,
 * then send the byte to the DSP_COMMAND port.
 */
static int
dsp_command (sbBYTE val)
{
  int i;

#ifdef FULL_DEBUG
  printf ("Sending DSP command 0x%x\n", val);
#endif
  for (i = DSP_LOOP_MAX; i; i--)
    {
#ifdef SB20			/* SB DSP 2.0 needs these? */
      tenmicrosec ();
      tenmicrosec ();
#endif
      if ((inb (DSP_STATUS) & DSP_BUSY) == 0)
	{
	  outb(DSP_COMMAND, val);
	  return (GOOD);
	}
      tenmicrosec ();
    }
  printf ("sb: dsp_command (%2X) failed!\n", val);
  return (FAIL);
}

/*
 * Sets the DSP buffer size.
 * Smaller sizes make the DSP more responsive,
 * but require more CPU attention.
 * Note that the size must be an even value for stereo to work.
 * We just reject odd sizes.
 * -1 is magic, it is replaced with the current buffer size.
 */
static int
dsp_set_bufsize (int *newsize)
{
  if (*newsize == -1)
    {
      *newsize = dsp.bufsize;
      return (ESUCCESS);
    }
  if (*newsize < DSP_MIN_BUFSIZE || *newsize > DSP_MAX_BUFSIZE
      || *newsize & 1)
    {
      DPRINTF (("sb: Attempt to set dsp.bufsize to %d!\n", *newsize));
      return (EINVAL);
    }
  
  dsp.bufsize = *newsize;
  COMPUTE_DSP_TIMEOUT;
  return (ESUCCESS);
}

/*
 * This turns stereo playback/recording on and off.
 * For playback, it seems to only be a bit in the mixer.
 * I don't know the secret to stereo sampling, so this may
 * need reworking.  If YOU know how to sample in stereo, send Email!
 * Maybe this should be a Mixer parameter, but I really don't think so.
 * It's a DSP Thing, isn't it?  Hmm..
 */
static int
dsp_set_stereo (sbFLAG select)
{
  dsp.in_stereo = !!select;
  /* Have to preserve DNFI bit from OUT_FILTER */
  mixer_send (CHANNELS, ((mixer_read_reg (OUT_FILTER) & ~STEREO_DAC)
			 | (dsp.in_stereo ? STEREO_DAC : MONO_DAC)));
  return (ESUCCESS);
}


/*
 * FM support functions
 */
static int
fm_ioctl (int cmd, caddr_t arg)
{
  switch (cmd)
    {
    case FM_IOCTL_RESET:
      return (fm_reset ());
    case FM_IOCTL_PLAY_NOTE:
      return (fm_play_note ((struct sb_fm_note*) arg));
    case FM_IOCTL_SET_PARAMS:
      return (fm_set_params ((struct sb_fm_params*) arg));
    default:
      return (EINVAL);
    }
}

static int
fm_open (void)
{
  if (status.fm_in_use)
    return (EBUSY);

  fm_reset();
  status.fm_in_use = 1;
  return (ESUCCESS);
}

static int
fm_close (void)
{
  if (status.fm_in_use)
    {
      status.fm_in_use = 0;
      return (fm_reset ());
    }
  else
    return (ESRCH);
}

/*
 * This really is the only way to reset the FM chips.
 * Just write zeros everywhere.
 */
static int
fm_reset (void)
{
  int reg;
  for (reg = 1; reg <= 0xf5; reg++)
    fm_send (FM_BOTH, reg, 0);

  return (ESUCCESS);;
}

/*
 * Send one byte to the FM chips.  'channel' specifies which
 * of the two SB Pro FM chip-sets to use.
 * SB (non-Pro) and Adlib owners should set both the Left and
 * Right addresses to the same (and only) FM set.
 * See sb_regs.h for more info.
 */
static int
fm_send (int channel, unsigned char reg, unsigned char val)
{
#ifdef FULL_DEBUG
  printf ("(%d) %02x: %02x\n", channel, reg, val);
#endif
  switch (channel)
    {
    case FM_LEFT:
      sb_sendb (FM_ADDR_L, reg, FM_DATA_L, val);
      break;
    case FM_RIGHT:
      sb_sendb (FM_ADDR_R, reg, FM_DATA_R, val);
      break;
    case FM_BOTH:
      sb_sendb (FM_ADDR_B, reg, FM_DATA_B, val);
      break;
    default:
      printf ("sb: Hey, fm_send to which channel?!\n");
      return (FAIL);
    }
  return (GOOD);
}

/*
 * Play one FM note, as defined by the sb_fm_note structure.
 * See sblast.h for info on the fields.
 * Now, these functions don't do any validation of the parameters,
 * but it just uses the lowest bits, so you can't hurt anything
 * with garbage.  Checking all those fields would take a while.
 */
static int
fm_play_note (struct sb_fm_note *n)
{
  int op, voice, ch;
  
  op = n->operator;		/* Only operator 0 or 1 allowed */
  if (op & ~1)
    return (EINVAL);

  voice = n->voice;		/* Voices numbered 0 through 8 */
  if (voice < 0 || voice > 8)
    return (EINVAL);

  ch = n->channel;		/* FM_LEFT, FM_RIGHT, or FM_BOTH */
  if (ch != FM_LEFT && ch != FM_RIGHT && ch != FM_BOTH)
    return (EINVAL);

  /*
   * See sb_regs.h for a detailed explanation of each of these
   * registers and their bit fields.  Here it is, packed dense.
   */
  fm_send (ch, MODULATION(voice, op),
           F(n->am) << 7
           | F(n->vibrato) << 6
           | F(n->do_sustain) << 5
           | F(n->kbd_scale) << 4
           | B4(n->harmonic));
  fm_send (ch, LEVEL(voice, op),
           B2(n->scale_level) << 6
           | B6(~n->volume));
  fm_send (ch, ENV_AD(voice, op),
           B4(n->attack) << 4
           | B4(n->decay));
  fm_send (ch, ENV_SR(voice, op),
           B4(~n->sustain) << 4 /* Make sustain level intuitive */
           | B4(n->release));
  fm_send (ch, FNUM_LO(voice), B8(n->fnum));
  fm_send (ch, FEED_ALG(voice),
           B3(n->feedback) << 1
           | F(n->indep_op));
  fm_send (ch, WAVEFORM(voice, op), B2(n->waveform));
  fm_send (ch, OCT_SET(voice),
           F(n->key_on) << 5
           | B3(n->octave) << 2
           | B2(n->fnum >> 8));
  return (ESUCCESS);
}

static int
fm_set_params (struct sb_fm_params *p)
{
  if (p->channel != FM_LEFT && p->channel != FM_RIGHT && p->channel != FM_BOTH)
    return (EINVAL);
  
  fm_send (p->channel, DEPTHS,
           F(p->am_depth) << 7
           | F(p->vib_depth) << 6
           | F(p->rhythm) << 5
           | F(p->bass) << 4
           | F(p->snare) << 3
           | F(p->tomtom) << 2
           | F(p->cymbal) << 1
           | F(p->hihat));
  fm_send (p->channel, WAVE_CTL, F(p->wave_ctl) << 5);
  fm_send (p->channel, CSM_KEYSPL, F(p->speech) << 7
	   | F(p->kbd_split) << 6);
  return (ESUCCESS);
}

/*
 * This is to write a stream of timed FM notes and parameters
 * to the FM device.  I'm not sure how useful it will be, since
 * it's totally non-standard.  Most FM should stick to the ioctls.
 * See the docs for an explanation of the expected data format.
 * Consider this experimental.
 */
static int
fm_write (struct uio *uio)
{
  int pause, bytecount, chunk;
  char type;
  char *chunk_addr;
  struct sb_fm_note note;
  struct sb_fm_params params;

  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      chunk_addr = uio->uio_iov[chunk].iov_base;
      bytecount = 0;
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  /* First read the one-character event type */
	  if (copyin (chunk_addr + bytecount, &type, sizeof (type)))
	    return (ENOTTY);
	  bytecount += sizeof (type);

	  switch (type)
	    {
	    case 'd':		/* Delay */
	    case 'D':
	      DPRINTF (("FM event: delay\n"));
	      if (copyin (chunk_addr + bytecount, &pause, sizeof (pause)))
		return (ENOTTY);
	      bytecount += sizeof (pause);
	      fm_delay (pause);
	      break;

	    case 'n':		/* Note */
	    case 'N':
	      DPRINTF (("FM event: note\n"));
	      if (copyin (chunk_addr + bytecount, &note, sizeof (note)))
		return (ENOTTY);
	      bytecount += sizeof (note);

	      if (fm_play_note (&note) != ESUCCESS)
		return (FAIL);
	      break;

	    case 'p':		/* Params */
	    case 'P':
	      DPRINTF (("FM event: params\n"));
	      if (copyin (chunk_addr + bytecount, &params, sizeof (params)))
		return (ENOTTY);
	      bytecount += sizeof (params);

	      if (fm_set_params (&params) != ESUCCESS)
		return (FAIL);
	      break;

	    default:
	      printf ("sb: Unrecognized FM event type: %c\n", type);
	      return (FAIL);	/* FAIL: Bad event type */
	    }
	}			/* while data left to write */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (ESUCCESS);
}

/*
 * I am not happy with this function!
 * The FM chips are supposed to give an interrupt when a timer
 * overflows, and they DON'T as far as I can tell.  Without
 * the interrupt, the timers are useless to Unix, since polling
 * is altogether unreasonable.
 *
 * I am still looking for information on this timer facility, so
 * if you know more about how to use it, and specifically how to
 * make it generate an interrupt, I'd love to hear from you.
 * I'm beginning to think that they can't trigger a bus interrupt,
 * and all the DOS software just polls it.
 * If this is the case, expect this function to disappear in future
 * releases.
 */
static int
fm_delay (int pause)
{
  int ticks;
#ifdef __bsdi__
  int s;
#endif
  
  DPRINTF (("fm_delay() pausing for %d microsecs... ", pause));

#ifdef __bsdi__
  s = splhigh();
#endif
#ifndef USE_BROKEN_FM_TIMER	/**** Kludge! ****/
  wait_for_intr (SB_FM_NUM, HZ * pause / 1000000, INTR_PRIORITY); 
#else

  fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
  fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
  while (pause >= 20000)	/* The 80us timer can do 20,400 usecs */
    {
      ticks = MIN((pause / 320), 255);
      fm_send (TIMER_CHANNEL, TIMER2, (256 - ticks));
      printf ("%d ticks (320 us)\n", ticks);
      fm_send (TIMER_CHANNEL, TIMER_CTL, (FM_TIMER_MASK1 | FM_TIMER_START2));
      wait_for_intr (SB_FM_NUM, TIMEOUT, INTR_PRIORITY);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
      pause -= ticks * 320;
    }

  while (pause >= 80)
    {
      ticks = MIN((pause / 80), 255);
      fm_send (TIMER_CHANNEL, TIMER1, (256 - ticks));
      printf ("%d ticks (80 us)\n", ticks);
      fm_send (TIMER_CHANNEL, TIMER_CTL, (FM_TIMER_START1 | FM_TIMER_MASK2));
      wait_for_intr (SB_FM_NUM, TIMEOUT, INTR_PRIORITY);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_MASK1 | FM_TIMER_MASK2);
      fm_send (TIMER_CHANNEL, TIMER_CTL, FM_TIMER_RESET);
      pause -= ticks * 80;
    }
  DPRINTF (("done.\n"));
#endif
#ifdef __bsdi__
  splx(s);
#endif
  return (pause);		/* Return unspent time (< 80 usecs) */
}


/*
 * Mixer functions
 */
static int
mixer_ioctl (int cmd, caddr_t arg)
{
  switch (cmd)
    {
    case DSP_IOCTL_STEREO:	/* Mixer can control DSP stereo */
      return (dsp_ioctl (cmd, arg));
    case MIXER_IOCTL_SET_LEVELS:
      return (mixer_set_levels ((struct sb_mixer_levels*) arg));
    case MIXER_IOCTL_SET_PARAMS:
      return (mixer_set_params ((struct sb_mixer_params*) arg));
    case MIXER_IOCTL_READ_LEVELS:
      return (mixer_get_levels ((struct sb_mixer_levels*) arg));
    case MIXER_IOCTL_READ_PARAMS:
      return (mixer_get_params ((struct sb_mixer_params*) arg));
    case MIXER_IOCTL_RESET:
      return (mixer_reset ());
    default:
      return (EINVAL);
    }
}

/*
 * Sets mixer volume levels.
 * All levels except mic are 0 to 15, mic is 7.  See sbinfo.doc
 * for details on granularity and such.
 * Basically, the mixer forces the lowest bit high, effectively
 * reducing the possible settings by one half.  Yes, that's right,
 * volume levels have 8 settings, and microphone has four.  Sucks.
 */
static int
mixer_set_levels (struct sb_mixer_levels *l)
{
  if (l->master.l & ~0xF || l->master.r & ~0xF
      || l->line.l & ~0xF || l->line.r & ~0xF
      || l->voc.l & ~0xF || l->voc.r & ~0xF
      || l->fm.l & ~0xF || l->fm.r & ~0xF
      || l->cd.l & ~0xF || l->cd.r & ~0xF
      || l->mic & ~0x7)
    return (EINVAL);

  mixer_send (VOL_MASTER, (l->master.l << 4) | l->master.r);
  mixer_send (VOL_LINE, (l->line.l << 4) | l->line.r);
  mixer_send (VOL_VOC, (l->voc.l << 4) | l->voc.r);
  mixer_send (VOL_FM, (l->fm.l << 4) | l->fm.r);
  mixer_send (VOL_CD, (l->cd.l << 4) | l->cd.r);
  mixer_send (VOL_MIC, l->mic);
  return (ESUCCESS);
}

/*
 * This sets aspects of the Mixer that are not volume levels.
 * (Recording source, filter level, I/O filtering, and stereo.)
 */
static int
mixer_set_params (struct sb_mixer_params *p)
{
  if (p->record_source != SRC_MIC
      && p->record_source != SRC_CD
      && p->record_source != SRC_LINE)
    return (EINVAL);

  /*
   * I'm not sure if this is The Right Thing.  Should stereo
   * be entirely under control of DSP?  I like being able to toggle
   * it while a sound is playing, so I do this... because I can.
   */
  dsp.in_stereo = !!p->dsp_stereo;

  mixer_send (RECORD_SRC, (p->record_source
			   | (p->hifreq_filter ? FREQ_HI : FREQ_LOW)
			   | (p->filter_input ? FILT_ON : FILT_OFF)));

  mixer_send (OUT_FILTER, ((dsp.in_stereo ? STEREO_DAC : MONO_DAC)
			   | (p->filter_output ? FILT_ON : FILT_OFF)));
  return (ESUCCESS);
}

/*
 * Read the current mixer level settings into the user's struct.
 */
static int
mixer_get_levels (struct sb_mixer_levels *l)
{
  sbBYTE val;

  val = mixer_read_reg (VOL_MASTER); /* Master */
  l->master.l = B4(val >> 4);
  l->master.r = B4(val);

  val = mixer_read_reg (VOL_LINE); /* FM */
  l->line.l = B4(val >> 4);
  l->line.r = B4(val);
  
  val = mixer_read_reg (VOL_VOC); /* DAC */
  l->voc.l = B4(val >> 4);
  l->voc.r = B4(val);

  val = mixer_read_reg (VOL_FM); /* FM */
  l->fm.l = B4(val >> 4);
  l->fm.r = B4(val);
  
  val = mixer_read_reg (VOL_CD); /* CD */
  l->cd.l = B4(val >> 4);
  l->cd.r = B4(val);

  val = mixer_read_reg (VOL_MIC); /* Microphone */
  l->mic = B3(val);

  return (ESUCCESS);
}

/*
 * Read the current mixer parameters into the user's struct.
 */
static int
mixer_get_params (struct sb_mixer_params *params)
{
  sbBYTE val;

  val = mixer_read_reg (RECORD_SRC);
  params->record_source = val & 0x07;
  params->hifreq_filter = !!(val & FREQ_HI);
  params->filter_input = (val & FILT_OFF) ? OFF : ON;
  params->filter_output = (mixer_read_reg (OUT_FILTER) & FILT_OFF) ? OFF : ON;
  params->dsp_stereo = dsp.in_stereo;
  return (ESUCCESS);
}

/*
 * Silly little function to read the entire mixer levels & params
 * as raw binary data from the device.  Hey, don't laugh, I set
 * my mixer with:
 * bash$ cat good-settings > /dev/sb_mixer
 * and take snapshots in the same way.
 * Of questionable utility, I admit.
 */
static int
mixer_read (struct uio *uio)
{
  int bytecount;
  struct sb_mixer_params params;
  struct sb_mixer_levels levels;

  if (uio->uio_offset != 0)
    return (ESUCCESS);

  /* There needs to be enough space for both structures. */ 
  if (uio->uio_iov->iov_len < (sizeof (params) + sizeof (levels)))
    return (ENOMEM);

  mixer_get_params (&params);
  mixer_get_levels (&levels);

  bytecount = 0;

  if (copyout (&params, uio->uio_iov->iov_base, sizeof (params)))
    return (EINVAL);
  bytecount += sizeof (params);
  if (copyout (&levels, uio->uio_iov->iov_base + bytecount, sizeof (levels)))
    return (EINVAL);
  bytecount += sizeof (levels);

  uio->uio_resid -= bytecount;
  return (ESUCCESS);
}

/*
 * Same deal as mixer_read().
 * Just read binary data for mixer params and levels and set the mixer.
 */
static int
mixer_write (struct uio *uio)
{
  int bytecount;
  struct sb_mixer_params params;
  struct sb_mixer_levels levels;

  if (uio->uio_offset != 0)
    return (ESUCCESS);
  
  if (uio->uio_iov->iov_len < (sizeof (params) + sizeof (levels)))
    return (ENOMEM);

  bytecount = 0;
  if (copyin (uio->uio_iov->iov_base, &params, sizeof (params)))
    return (EINVAL);
  bytecount += sizeof (params);
  if (copyin (uio->uio_iov->iov_base + bytecount, &levels, sizeof (levels)))
    return (EINVAL);
  bytecount += sizeof (levels);

  if (mixer_set_params (&params) == ESUCCESS
      && mixer_set_levels (&levels) == ESUCCESS)
    uio->uio_resid -= bytecount;

  return (ESUCCESS);
}

#ifdef KOLVIR
static int
mixer_reset (void)
{
    mixer_send(MIXER_RESET, MIXER_RESET);
}
#else
/*
 * This is supposed to reset the mixer.
 * Technically, a reset is performed by sending a byte to the MIXER_RESET
 * register, but I don't like the default power-up settings, so I use
 * these.  Adjust to taste, and you have your own personalized mixer_reset
 * ioctl.
 */
static int
mixer_reset (void)
{
  struct sb_mixer_levels l;
  struct sb_mixer_params p;

  p.filter_input  = OFF;
  p.filter_output = OFF;
  p.hifreq_filter = TRUE;
  p.record_source = SRC_LINE;

  l.cd.l = l.cd.r = 0;
  l.mic = 1;
  l.master.l = l.master.r = 11;
  l.line.l = l.line.r = 15;
  l.fm.l = l.fm.r = 15;
  l.voc.l = l.voc.r = 15;

  if (mixer_set_levels (&l) == ESUCCESS
      && mixer_set_params (&p) == ESUCCESS)
    return (ESUCCESS);
  else
    return (EIO);
}
#endif

/*
 * Send a byte 'val' to the Mixer register 'reg'.
 */
static void
mixer_send (sbBYTE reg, sbBYTE val)
{
#if FULL_DEBUG
  printf ("%02x: %02x\n", reg, val);
#endif
  sb_sendb (MIXER_ADDR, reg, MIXER_DATA, val);
}

/*
 * Returns the contents of the mixer register 'reg'.
 */
static sbBYTE
mixer_read_reg (sbBYTE reg)
{
  outb (MIXER_ADDR, reg);
  tenmicrosec();		/* To make sure nobody reads too soon */
  return (inb (MIXER_DATA));
}


/*
 * MIDI functions
 */
static int
midi_open (void)
{
  if (status.dsp_in_use)
    return (EBUSY);

  status.dsp_in_use = TRUE;
#ifndef __bsdi__
  status.selects[SB_MIDI_NUM] = NULL;
  status.sel_coll[SB_MIDI_NUM] = FALSE;
#endif
  dsp_reset ();			/* Resets card and inits variables */

  /*
   * This DSP command requests interrupts on MIDI bytes, but allows
   * writing to the MIDI port at the same time.  This is only
   * available in SB DSP's v2.0 or greater.
   * Any byte sent to the DSP while in this mode will be sent out the
   * MIDI OUT lines.  The only way to exit this mode is to reset the DSP.
   */
  dsp_command (MIDI_UART_READ);
  return (ESUCCESS);
}

static int
midi_close ()
{
  if (status.dsp_in_use)
    {
      status.dsp_in_use = FALSE;
      if (dsp_reset() == GOOD)	/* Exits MIDI UART mode */
	return (ESUCCESS);
      else
	return (EIO);
    }
  else
    return (ESRCH);
}

static int
midi_select (int rw, struct proc *procp)
{
  int s = splhigh();
  DPRINTF (("sb: midi_select() called with rw = %d.\n", rw));
  if (((rw == SELECT_WRITE))
      || ((rw == SELECT_READ) && (inb (DSP_RDAVAIL) & DSP_DATA_AVAIL)))
    {
      splx (s);
      return (1);		/* Give the go-ahead. */
    }

  /* It's not ready yet, so put them on the list of procs to notify */
#ifdef __bsdi__
  selrecord(procp, &status.sel[SB_MIDI_NUM]);
#else
  if (status.selects[SB_MIDI_NUM])
    status.sel_coll[SB_MIDI_NUM] = TRUE;
  else
    status.selects[SB_MIDI_NUM] = procp;
#endif
  
  splx (s);
  return 0;
}

static int
midi_read (struct uio *uio)
{
  int bytecount, s, chunk;
  sbBYTE data;

  s = splhigh ();
  /* For each uio_iov[] entry... */
  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      bytecount = 0;

      /* While there is still data to read, and data in this chunk.. */
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  while (!(inb (DSP_RDAVAIL) & DSP_DATA_AVAIL))
	    {
	      /* Maybe they don't want to wait.. */
	      if (status.flags[SB_MIDI_NUM] & FNDELAY)
		{
		  splx (s);
		  return (EWOULDBLOCK);
		}
#ifdef __bsdi__
	      if (wait_for_intr (DSP_UNIT, 0, PSLEP) == ERESTART) {
		  splx (s);
		  return EINTR;
	      }
#else
	      wait_for_intr (DSP_UNIT, 0, PSLEP); /* Maybe they do. */
#endif
	    }
	  /*
	   * In any case, data is ready for reading by this point.
	   */
	  DPRINTF (("Getting one MIDI byte.."));
	  data = inb (DSP_RDDATA);
	  if (copyout (&data, uio->uio_iov[chunk].iov_base + bytecount, 1))
	    {
	      printf ("sb: Bad copyout()!\n");
	      return (ENOTTY);
	    }
	  bytecount ++;
	  uio->uio_resid --;
	}
    }			/* for each chunk */
  splx (s);
  return (ESUCCESS);
}

static int
midi_write (struct uio *uio)
{
  int bytecount, chunk, s;
  char *chunk_addr;
  sbBYTE data;

  for (chunk = 0; chunk < uio->uio_iovcnt; chunk++)
    {
      chunk_addr = uio->uio_iov[chunk].iov_base;
      bytecount = 0;
      while (uio->uio_resid
	     && bytecount < uio->uio_iov[chunk].iov_len)
	{
	  if (copyin (chunk_addr + bytecount, &data, sizeof (data)))
	    return (ENOTTY);
	  bytecount++;
	  s = splhigh ();
	  /*
	   * In MIDI UART mode, data bytes are sent as DSP commands.
	   */
	  if (dsp_command (data) == FAIL)
	    {
	      splx (s);
	      return (EIO);
	    }
	  splx (s);
	}			/* while data left to write */
      uio->uio_resid -= bytecount;
    }				/* for each chunk */
  return (ESUCCESS);
}

/*
 * No valid ioctls for MIDI yet, but I have some ideas.
 */
static int
midi_ioctl (int cmd, caddr_t arg)
{
#if 0
  switch (cmd)
    {
    default:
      return (EINVAL);
    }
#endif
      /*
       * Shoot.  I can't figure this out, and I don't have any examples.
       * This is supposed to support the fcntl() call.
       * The only flags now are FNDELAY for non-blocking reads.
       * You can currently open the midi device with this flag, but
       * you can't set it with fcntl().
       * Stay tuned...
       */
#if 0				/* sorry */
    case TIOCGETP:
      arg->sg_flags = dsp.dont_block;
      break;
    case TIOCSETP:
      dsp.dont_block = arg->sg_flags & FNDELAY;
      break;
#endif
  return (ESUCCESS);
}

#endif NSB > 0

/* ---- Emacs kernel recompile command ----
Local Variables:
compile-command: "/bin/sh -c \"cd /sys/compile/KOLVIR; make\""
End:
*/
