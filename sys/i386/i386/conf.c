/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: conf.c,v 1.22 1994/02/06 20:12:38 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
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
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/conf.h"

int	rawread		__P((dev_t, struct uio *, int));
int	rawwrite	__P((dev_t, struct uio *, int));
int	swstrategy	__P((struct buf *));
int	ttselect	__P((dev_t, int, struct proc *));

#define	dev_type_open(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_close(n)	int n __P((dev_t, int, int, struct proc *))
#define	dev_type_strategy(n)	int n __P((struct buf *))
#define	dev_type_ioctl(n) \
	int n __P((dev_t, int, caddr_t, int, struct proc *))

/* bdevsw-specific types */
#define	dev_type_dump(n)	int n __P((dev_t))
#define	dev_type_size(n)	int n __P((dev_t))

#define	dev_decl(n,t)	__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(c,n,t) \
	(c > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)

/* bdevsw-specific initializations */
#define	dev_size_init(c,n)	(c > 0 ? __CONCAT(n,size) : 0)

#define	bdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,strategy); \
	dev_decl(n,ioctl); dev_decl(n,dump); dev_decl(n,size)

#define	bdev_disk_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_init(c,n,strategy), dev_init(c,n,ioctl), \
	dev_init(c,n,dump), dev_size_init(c,n), 0 }

#define	bdev_tape_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	dev_init(c,n,strategy), dev_init(c,n,ioctl), \
	dev_init(c,n,dump), 0, B_TAPE }

#define	bdev_swap_init() { \
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, \
	swstrategy, (dev_type_ioctl((*))) enodev, \
	(dev_type_dump((*))) enodev, 0, 0 }

#define	bdev_notdef()	bdev_tape_init(0,no)
bdev_decl(no);	/* dummy declarations */

#include "wd.h"
#include "fd.h"
#include "wt.h"
#include "sd.h"
#include "st.h"
#include "mcd.h"

bdev_decl(wd);
bdev_decl(Fd);
bdev_decl(wt);
bdev_decl(sd);
bdev_decl(st);
bdev_decl(mcd);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NWD,wd),	/* 0: st506/rll/esdi/ide disk */
	bdev_swap_init(),	/* 1: swap pseudo-device */
	bdev_disk_init(NFD,Fd),	/* 2: floppy disk */
	bdev_tape_init(NWT,wt),	/* 3: QIC-24/60/120/150 cartridge tape */
	bdev_disk_init(NSD,sd),	/* 4: SCSI disk pseudo-device */
	bdev_tape_init(NST,st),	/* 5: SCSI tape pseudo-device */
	bdev_disk_init(NMCD,mcd), /* 6: Mitsumi CD-ROM */
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

/* cdevsw-specific types */
#define	dev_type_read(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_write(n)	int n __P((dev_t, struct uio *, int))
#define	dev_type_stop(n)	int n __P((struct tty *, int))
#define	dev_type_reset(n)	int n __P((int))
#define	dev_type_select(n)	int n __P((dev_t, int, struct proc *))
#define	dev_type_map(n)	int n __P(())

#define	cdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); \
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); \
	dev_decl(n,reset); dev_decl(n,select); dev_decl(n,map); \
	dev_decl(n,strategy); extern struct tty __CONCAT(n,_tty)[]

#define	dev_tty_init(c,n)	(c > 0 ? __CONCAT(n,_tty) : 0)

/* open, close, read, write, ioctl, strategy */
#define	cdev_disk_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), rawread, \
	rawwrite, dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, strategy */
#define	cdev_tape_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), rawread, \
	rawwrite, dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

/* new-style tty: open, close, read, write, ioctl, stop, select */
#define	cdev_tty_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

#define	cdev_notdef() { \
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, (dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, seltrue, \
	(dev_type_map((*))) enodev, 0 }

cdev_decl(no);			/* dummy declarations */

cdev_decl(cn);
/* open, close, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_cn_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

cdev_decl(ctty);
/* open, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_ctty_init(c,n) { \
	dev_init(c,n,open), (dev_type_close((*))) nullop, dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

dev_type_read(mmrw);
/* read/write */
#define	cdev_mm_init(c,n) { \
	(dev_type_open((*))) nullop, (dev_type_close((*))) nullop, mmrw, \
	mmrw, (dev_type_ioctl((*))) enodev, (dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_map((*))) enodev, 0 }

/* read, write, strategy */
#define	cdev_swap_init(c,n) { \
	(dev_type_open((*))) nullop, (dev_type_close((*))) nullop, rawread, \
	rawwrite, (dev_type_ioctl((*))) enodev, (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, (dev_type_select((*))) enodev, \
	(dev_type_map((*))) enodev, dev_init(c,n,strategy) }

#include "pty.h"
#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);

/* open, close, read, write, ioctl, tty, select */
#define	cdev_ptc_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

cdev_decl(log);
/* open, close, read, ioctl, select -- XXX should be a generic device */
#define	cdev_log_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, 0, \
	dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

/* open, close, read, write, ioctl, strategy */
#define	cdev_wt_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_map((*))) enodev, \
	dev_init(c,n,strategy) }

cdev_decl(wd);
cdev_decl(Fd);
cdev_decl(wt);
cdev_decl(sd);
cdev_decl(st);
cdev_decl(mcd);

#include "com.h"

cdev_decl(com);

#include "pcaux.h"

cdev_decl(pcx);

/* tty except for d_stop */
#define	cdev_pcx_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

#include "pccons.h"

cdev_decl(pc);

#include "rc.h"

cdev_decl(rc);

#include "midi.h"

cdev_decl(midi);

/* open, close, read, write, ioctl */
#define cdev_midi_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, 0, \
	dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

#include "sb.h"
cdev_decl(sb);

#define cdev_sb_init(c,n) { \
        dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
        dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev,\
        (dev_type_reset((*))) nullop, 0, dev_init(c,n,select), \
        (dev_type_map((*))) enodev, 0 }

#include "bpfilter.h"
cdev_decl(bpf);

/* open, close, read, write, ioctl, select -- XXX should be generic device */
#define	cdev_bpf_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, 0, dev_init(c,n,select), \
	(dev_type_map((*))) enodev, 0 }

cdev_decl(fd);
#define	cdev_fd_init(c,n) { \
	dev_init(c,n,open), (dev_type_close((*))) enodev, \
	(dev_type_read((*))) enodev, (dev_type_write((*))) enodev, \
	(dev_type_ioctl((*))) enodev, (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) enodev, 0, (dev_type_select((*))) enodev, \
	(dev_type_map((*))) enodev, 0 }

#include "vga.h"
cdev_decl(vga);
#define	cdev_vga_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), \
	(dev_type_read((*))) enodev, (dev_type_write((*))) enodev, \
	dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	(dev_type_reset((*))) nullop, 0, seltrue, \
	dev_init(c,n,map), 0 }

cdev_decl(kbd);
extern struct tty pckbd;
#define		kbdclose	pcclose
#define		kbdread		pcread
#define		kbdioctl	pcioctl
#define	cdev_kbd_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) nullop, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, \
	(dev_type_reset((*))) nullop, &pckbd, ttselect, \
	(dev_type_map((*))) nullop, 0 }

#include "lp.h"
cdev_decl(lp);
#define cdev_lp_init(c,n) {\
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, \
	0, dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

#include "bms.h"
cdev_decl(bms);
#define cdev_bms_init(c,n) {\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, \
	0, dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

#include "lms.h"
cdev_decl(lms);
#define cdev_lms_init(c,n) {\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, \
	0, dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

#include "ms.h"
cdev_decl(ms);

#include "digi.h"
cdev_decl(digi);

#include "si.h"
cdev_decl(si);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),		/* 3: st506/rll/esdi/ide disk */
	cdev_swap_init(1,sw),		/* 4: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 5: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 6: pseudo-tty master */
	cdev_log_init(1,log),		/* 7: /dev/klog */
	cdev_tty_init(NCOM,com),	/* 8: serial communications ports */
	cdev_disk_init(NFD,Fd),		/* 9: floppy disk */
	cdev_wt_init(NWT,wt),		/* 10: QIC-02/36 cartridge tape */
	cdev_tty_init(NRC,rc),		/* 11: RISCom/N8 async mux */
	cdev_tty_init(NPCCONS,pc),	/* 12: vga console */
	cdev_pcx_init(NPCAUX,pcx),	/* 13: console/keyboard aux port */
	cdev_bpf_init(NBPFILTER,bpf),	/* 14: berkeley packet filter */
	cdev_fd_init(1,fd),		/* 15: file descriptor devices */
	cdev_vga_init(NVGA,vga),	/* 16: VGA display for X */
	cdev_kbd_init(NPCCONS,kbd),	/* 17: Keyboard device (excl from cn)*/
	cdev_disk_init(NSD,sd),		/* 18: SCSI disk pseudo-device */
	cdev_tape_init(NST,st),		/* 19: SCSI tape pseudo-device */
	cdev_lp_init(NLP,lp),		/* 20: printer on a parallel port */
	cdev_bms_init(NBMS,bms),	/* 21: Microsoft Bus Mouse */
	cdev_midi_init(NMIDI,midi),	/* 22: Midi device */
	cdev_disk_init(NMCD,mcd),       /* 23: Mitsumi CD-ROM */
	cdev_tty_init(NMS,ms),		/* 24: Maxpeed Async Mux */
	cdev_lms_init(NLMS,lms),	/* 25: Logitec Bus Mouse */
	cdev_tty_init(NDIGI,digi),	/* 26: DigiBoard PC/X[ei] */
	cdev_tty_init(NSI,si),		/* 27: Specialix multiplexor */
	cdev_sb_init(NSB,sb),		/* 28: SoundBlaster Pro */
};

int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);
