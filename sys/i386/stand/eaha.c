/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eaha.c,v 1.5 1994/01/07 01:16:40 karels Exp $
 */

/*
 * Standalone Adaptec 1742A SCSI host adapter driver.
 */

#include "saio.h"
#include "scsi/scsi.h"
#include "sascsi.h"
#include "i386/isa/isa.h"
#include "i386/eisa/eisa.h"
#include "i386/eisa/eahareg.h"

#define	NADAPT			2
#define	NTARG			8

int eahaattach __P((struct iob *io));
int eahaopen __P((struct iob *io));
int eahastart __P((struct iob *io, int,
    int (*dgo) (struct iob *io, int, struct scsi_cdb *cdb),
    struct scsi_cdb *cdb));
int eahago __P((struct iob *io));

struct scsi_driver eahadriver = {
	eahaattach,
	eahastart,
	eahago
};

#ifdef SMALL
struct scsi_driver *scsi_driver[] = {
	NULL,
	&eahadriver,		/* 1 = eahadriver */
};
#endif

static int eaha;

static struct ecb ecb;
static struct estatus estatus;
static struct scsi_sense sn;

#ifndef SMALL
/*
 * Allow users to type 'eaha()bsd'.
 */
int
eahaopen(io)
	struct iob *io;
{

	io->i_dev = SD_MAJORDEV;
	io->i_adapt = 1;
	return (sdopen(io));
}
#endif

static u_long eaha_ids[] = {
	0x00009004,	/* ADP0000 */
	0x01009004,	/* ADP0001 */
	0x02009004,	/* ADP0002 */
	0x00049004	/* ADP0400 */
};

int
eahaattach(io)
	struct iob *io;
{
	char **cidp, *id;
	int slot;
	int  i, p;
	u_long compressed_id;

	if (eaha)
		/* we're already initialized */
		return (0); 
	if (bcmp((caddr_t)EISA_ID_OFFSET, EISA_ID, EISA_ID_LEN))
		return (0);

	/*
	 * Scan for 1742 cards.
	 * Count the ones we find and
	 * stop at the selected one.
	 */
	for (slot = 1; slot < EISA_NUM_PHYS_SLOT; ++slot) {
		p = EISA_PROD_ID_BASE(slot);
		outb(p, 0xff);
		outb(p, 0xff);
		if (inb(p) & 0x80 && inb(p) & 0x80)
			continue;
		compressed_id = (u_char)inb(p) |
		    (u_char)inb(p + 1) << 8 |
		    (u_char)inb(p + 2) << 16 |
		    (u_char)inb(p + 3) << 24;
		for (i = 0; i < sizeof eaha_ids / sizeof eaha_ids[0]; ++i)
			if (eaha_ids[i] == compressed_id) {
				eaha = EAHA_BASE(slot);
				goto found;
			}
	}
	return (EADAPT);

found:
	if (io->i_ctlr & ~(NTARG - 1))
		return (ECTLR);
#if 0 /* permit arbitrary logical device numbers */
	if (io->i_unit != 0)
		return (EUNIT);
#endif

#ifndef SMALL
	/* disable the BIOS, we can't use it any more */
	outb(EAHA_BIOSADDR(eaha), 0);

	/* disable interrupts, but supply an IRQ out of paranoia */
	outb(EAHA_INTDEF(eaha), EID_INTHIGH | EID_INT15);

	/* select enhanced mode operation */
	/* EP_ENHANCED has no effect here, it's just for comfort */
	outb(EAHA_PORTADDR(eaha), EP_ENHANCED | EP_ISADISABLE);
#endif

	/* put the card in a known state */
	outb(EAHA_CTRL(eaha), EC_HRDY | EC_CLRINT);

	return (0);
}

int
eahastart(io, flag, dgo, cdb)
	struct iob *io;
	int flag;
	int (*dgo) __P((struct iob *io, int, struct scsi_cdb *cdb));
	struct scsi_cdb *cdb;
{

	bzero(&ecb, sizeof ecb);
	ecb.e_cmd = ECB_CMD;
	/* add automatic request sense to foil contingent allegiance */
	ecb.e_flag1 = F1_ARS;
#ifdef notyet /* i_unit is not currently a LUN */
	ecb.e_flag2 = io->i_unit;
#endif
	ecb.e_status = (u_long)&estatus;
	ecb.e_sense = (u_long)&sn;
	ecb.e_senselen = sizeof sn;    

	ecb.e_data = (u_long)io->i_ma;
	ecb.e_datalen = io->i_cc;
	if (io->i_cc) {
		ecb.e_flag2 |= F2_DAT;
		if (flag == F_READ)
			ecb.e_flag2 |= F2_DIR;
	}
	if (cdb)
		bcopy(cdb, &ecb.e_cdb, sizeof ecb.e_cdb);

	return ((*dgo)(io, flag,
	    cdb ? (struct scsi_cdb *)0 : (struct scsi_cdb *)&ecb.e_cdb));
}

int
eahago(io)
	struct iob *io;
{
	struct ecb *in_ecb;
	int intr;

	ecb.e_cdblen = scsicmdlen[ecb.e_cdbbytes[0] >> 5];

restart:
	while ((inb(EAHA_STAT(eaha)) & (ES_EMPTY|ES_BUSY)) != ES_EMPTY)
		;
	outb(EAHA_MBOUT(eaha), (u_long)&ecb);
	outb(EAHA_MBOUT(eaha) + 1, (u_long)&ecb >> 8);
	outb(EAHA_MBOUT(eaha) + 2, (u_long)&ecb >> 16);
	outb(EAHA_MBOUT(eaha) + 3, (u_long)&ecb >> 24);
	outb(EAHA_ATTN(eaha), EA_START | io->i_ctlr);

	do {
		while ((inb(EAHA_STAT(eaha)) & ES_INTR) == 0)
			;
		intr = inb(EAHA_INTR(eaha));
		in_ecb = (struct ecb *)
		    ((((u_char)inb(EAHA_MBIN(eaha) + 3) << 8 |
		       (u_char)inb(EAHA_MBIN(eaha) + 2)) << 8 |
		       (u_char)inb(EAHA_MBIN(eaha) + 1)) << 8 |
		       (u_char)inb(EAHA_MBIN(eaha)));
		outb(EAHA_CTRL(eaha), EC_HRDY | EC_CLRINT);
	} while ((intr & EI_TARGMASK) != io->i_ctlr || in_ecb != &ecb);

	if ((intr & EI_STATMASK) != EI_SUCCESS &&
	    (intr & EI_STATMASK) != EI_RETRY) {
		if (estatus.es_flags & ESF_SNS &&
		    SENSE_ISXSENSE(&sn) && XSENSE_ISSTD(&sn) &&
		    XSENSE_KEY(&sn) == SKEY_ATTENTION)
			/* ignore unit attention reports */
			goto restart;
		printf("eaha0: error, intr = %x, flags = %x, "
		    "hastat = %x, tarstat = %x\n", intr,
		    estatus.es_flags, estatus.es_hastat, estatus.es_tarstat);
		if (estatus.es_tarstat)
			return (estatus.es_tarstat);    
		return (-1);
	}

	return (STS_GOOD);
}
