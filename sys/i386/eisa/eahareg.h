/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eahareg.h,v 1.3 1992/11/19 13:08:30 karels Exp $
 */

/*
 * Adaptec AHA-1742A definitions.
 */

/* convert from slot number to mailbox address */
#define	EAHA_BASE(slot)	(((slot) << 12) + 0xcd0)

#define	EAHA_IRQMASK	(IRQ9|IRQ10|IRQ11|IRQ12|IRQ14|IRQ15)

/*
 * Mailbox registers.
 */
#ifndef LOCORE
#define	EAHA_MBOUT(e)	(e)
#define	EAHA_ATTN(e)	((e) + 4)
#define	EAHA_CTRL(e)	((e) + 5)
#define	EAHA_INTR(e)	((e) + 6)
#define	EAHA_STAT(e)	((e) + 7)
#define	EAHA_MBIN(e)	((e) + 8)
#define	EAHA_STAT2(e)	((e) + 12)
#else
#define	EAHA_MBOUT	0xcd0
#define	EAHA_ATTN	0xcd4
#define	EAHA_CTRL	0xcd5
#define	EAHA_INTR	0xcd6
#define	EAHA_STAT	0xcd7
#define	EAHA_MBIN	0xcd8
#define	EAHA_STAT2	0xcdc
#endif

#define	EA_TARGMASK	0x0f		/* SCSI ID of target */
#define	EA_IMMED	0x10		/* immediate command */
#define	EA_START	0x40		/* start CCB */
#define	EA_ABORT	0x50		/* abort CCB */

#define	IM_CMD_RESET	0x80		/* reset device or host adapter */
#define	IM_CMD_RESUME	0x90		/* resume after ECA */
#define	IM_FLAG_ADAPTER	0x00080000	/* adapter reset option */
#define	IM_FLAG_DEVICE	0x00040000	/* device reset option */

#define	EC_HRDY		0x20		/* set host ready */
#define	EC_CLRINT	0x40		/* clear pending EISA interrupt */
#define	EC_HRESET	0x80		/* hard reset */

#define	EI_TARGMASK	0x0f		/* SCSI ID of target */
#define	EI_STATMASK	0xf0
#define	EI_SUCCESS	0x10		/* CCB completed with success */
#define	EI_RETRY	0x50		/* CCB succeeded with retries */
#define	EI_FAIL		0x70		/* adapter hardware failure */
#define	EI_IMMSUCC	0xa0		/* immediate command succeeded */
#define	EI_ERROR	0xc0		/* CCB completed with error */
#define	EI_AEN		0xd0		/* asynchronous event notification */
#define	EI_IMMERR	0xe0		/* immediate command error */

#define	ES_BUSY		0x01
#define	ES_INTR		0x02		/* interrupt pending */
#define	ES_EMPTY	0x04		/* mailbox out empty */

#define	ES2_READY	0x01		/* host ready */

/*
 * Board configuration registers.
 * These are given relative to the mailbox for convenience.
 */
#ifndef LOCORE
#define	EAHA_EBCTRL(e)		((e) - 76)
#define	EAHA_PORTADDR(e)	((e) - 16)
#define	EAHA_BIOSADDR(e)	((e) - 15)
#define	EAHA_INTDEF(e)		((e) - 14)
#define	EAHA_SCSIDEF(e)		((e) - 13)
#define	EAHA_BUSDEF(e)		((e) - 12)
#else
#define	EAHA_EBCTRL		0xc84
#define	EAHA_PORTADDR		0xcc0
#define	EAHA_BIOSADDR		0xcc1
#define	EAHA_INTDEF		0xcc2
#define	EAHA_SCSIDEF		0xcc3
#define	EAHA_BUSDEF		0xcc4
#endif

#define	EE_CDEN		0x01		/* enable host adapter (w/r) */
#define	EE_HAERR	0x02		/* serious host adapter error (r) */
#define	EE_ERRST	0x04		/* clear host adapter error (w) */

#define	EP_PORTMASK	0x07		/* port address selection bits */
#define	EP_ISADISABLE	0x00		/* disable ISA port addresses */
#define	EP_CONFIGURE	0x40		/* set EEPROM from next ECB */
#define	EP_ENHANCED	0x80		/* power up in enhanced mode */

#define	EB_BIOSSEL	0x0f		/* select BIOS address */
#define	EB_RAMEN	0x20		/* make noodle soup */
#define	EB_BIOSEN	0x40		/* enable the BIOS */
#define	EB_WRTPRT	0x80		/* write protect the BIOS RAM */

#define	EID_INTSEL	0x07		/* select an interrupt channel */
#define	EID_INT9	0x00
#define	EID_INT10	0x01
#define	EID_INT11	0x02
#define	EID_INT12	0x03
#define	EID_INT14	0x05
#define	EID_INT15	0x06
#define	EID_INTHIGH	0x08		/* high true vs. low true */
#define	EID_INTEN	0x10		/* enable interrupts */

#define	ESD_HSCSIID	0x0f		/* host's SCSI ID */
#define	ESD_RSTPWR	0x10		/* SCSI reset on power-up */

#define	EBD_BUSON	0x03		/* bus on time after preemption */
#define	EBD_BUS0	0x00		/* 0 microseconds */
#define	EBD_BUS4	0x01		/* 4 microseconds */
#define	EBD_BUS8	0x02		/* 8 microseconds */
#define	EBD_DMACHAN	0x0c		/* DMA channel selection */
#define	EBD_DMA0	0x00
#define	EBD_DMA5	0x04
#define	EBD_DMA6	0x08
#define	EBD_DMA7	0x0c

/*
 * Enhanced Control Block (ECB)
 */
#ifndef LOCORE
struct ecb {
	u_short		e_cmd;
	u_short		e_flag1;
	u_short		e_flag2;
	u_short		e_pad1;
	vm_offset_t	e_data;
	vm_size_t	e_datalen;
	vm_offset_t	e_status;
	vm_offset_t	e_chain;
	u_long		e_pad2;
	vm_offset_t	e_sense;
	u_char		e_senselen;
	u_char		e_cdblen;
	u_short		e_cksum;
	union {
		struct	scsi_cdb6 e_ucdb6;
		struct	scsi_cdb10 e_ucdb10;
		u_char	e_ucdbbytes[16];
	} e_ucdb;
};
#define	e_cdb6		e_ucdb.e_ucdb6
#define	e_cdb10		e_ucdb.e_ucdb10
#define	e_cdb		e_ucdb.e_ucdb6
#define	e_cdbbytes	e_ucdb.e_ucdbbytes
#else
#define	E_CMD		0x00
#define	E_FLAG1		0x02
#define	E_FLAG2		0x04
#define	E_DATA		0x08
#define	E_DATALEN	0x0c
#define	E_STATUS	0x10
#define	E_CHAIN		0x14
#define	E_SENSE		0x1c
#define	E_SENSELEN	0x20
#define	E_CDBLEN	0x21
#define	E_CKSUM		0x22
#define	E_CDB		0x24
#endif

#define	ECB_FREE	0xff
#define	ECB_NOP		0x00
#define	ECB_CMD		0x01
#define	ECB_DIAG	0x05
#define	ECB_INIT	0x06
#define	ECB_SENSE	0x08
#define	ECB_DOWNLOAD	0x09
#define	ECB_INQUIRY	0x0a
#define	ECB_TARGET	0x10

#define	F1_CNE		0x0001		/* chain no error */
#define	F1_DI		0x0080		/* disable interrupts */
#define	F1_SES		0x0400		/* suppress error on underrun */
#define	F1_SG		0x1000		/* scatter/gather */
#define	F1_DSB		0x4000		/* disable status block */
#define	F1_ARS		0x8000		/* automatic request sense */

#define	F2_LUNMASK	0x0007
#define	F2_TAG		0x0008		/* tagged queuing */
#define	F2_TTMASK	0x0030		/* tag type */
#define	F2_ND		0x0040		/* no disconnect */
#define	F2_DAT		0x0100		/* check direction of data transfer */
#define	F2_DIR		0x0200		/* direction of transfer */
#define	F2_ST		0x0400		/* suppress transfer to host memory */
#define	F2_CHK		0x0800		/* calculate checksum on data */
#define	F2_REC		0x4000		/* error recovery */
#define	F2_NRB		0x8000		/* no retry on busy status */

/*
 * Enhanced Status block
 */
#ifndef LOCORE
struct estatus {
	u_short		es_flags;
	u_char		es_hastat;
	u_char		es_tarstat;
	u_long		es_resid;
	u_long		es_residbuf;
	u_short		es_addlen;
	u_char		es_senselen;
	u_char		es_pad[9];
	u_char		es_targ[8];
};
#endif

#define	ESF_DONE	0x0001		/* command done -- no error */
#define	ESF_DU		0x0002		/* data underrun */
#define	ESF_QF		0x0008		/* host adapter queue full */
#define	ESF_SC		0x0010		/* specification check */
#define	ESF_DO		0x0020		/* data overrun */
#define	ESF_CH		0x0040		/* chaining halted */
#define	ESF_INT		0x0080		/* interrupt issued for CCB */
#define	ESF_ASA		0x0100		/* additional status available */
#define	ESF_SNS		0x0200		/* sense information stored */
#define	ESF_INI		0x0800		/* initialization required */
#define	ESF_ME		0x1000		/* major error or exception */
#define	ESF_ECA		0x4000		/* extended contingent allegiance */
#define	ESF_HAERROR(es) \
	((es) & (ESF_SC|ESF_INI|ESF_ME))
#define	ESF_BAD(es) \
	((es) & (ESF_QF|ESF_SC|ESF_CH|ESF_INI|ESF_ME|ESF_ECA))
#define	ESF_BITS \
	"\20\17ECA\15ME\14INI\12SNS\11ASA\10INT\7CH\6DO\5SC\4QF\2DU\1DON"

#define	ESH_GOOD	0x00
#define	ESH_HOST	0x04		/* command aborted by host */
#define	ESH_ADAPTER	0x05		/* command aborted by adapter */
#define	ESH_DOWNLOAD	0x08		/* firmware not downloaded */
#define	ESH_TARGET	0x0a		/* invalid target */
#define	ESH_TIMEOUT	0x11		/* selection timeout */
#define	ESH_LENGTH	0x12		/* data overrun or underrun */
#define	ESH_BUSFREE	0x13		/* unexpected bus free occurred */
#define	ESH_BUSPHASE	0x14		/* invalid bus phase detected */
#define	ESH_OPCODE	0x16		/* invalid operation code */
#define	ESH_LINK	0x17		/* invalid SCSI linking operation */
#define	ESH_PARAM	0x18		/* invalid control block parameter */
#define	ESH_DUPTARG	0x19		/* duplicate target CDB received */
#define	ESH_SCATTER	0x1a		/* invalid scatter/gather list */
#define	ESH_SENSE	0x1b		/* request sense command failed */
#define	ESH_TAG		0x1c		/* tagged queuing message rejected */
#define	ESH_HARDWARE	0x20		/* host adapter hardware error */
#define	ESH_ATTN	0x21		/* target didn't respond to attn */
#define	ESH_HARESET	0x22		/* SCSI bus reset by host adapter */
#define	ESH_RESET	0x23		/* SCSI bus reset by other device */
#define	ESH_CKSUM	0x80		/* program checksum failure */

/*
 * Scatter/gather maps
 */
#ifndef LOCORE
struct esg {
	vm_offset_t	esg_data;
	vm_size_t	esg_datalen;
};
#endif

/*
 * Software Enhanced Control Block
 */
#ifndef LOCORE
struct soft_ecb {
	struct	ecb se_ecb;
	int	se_targ;		/* the target (for attn register) */
	u_long	se_len;			/* total length of the transfer */
	scintr_fn se_intr;		/* base interrupt handler */
	struct	device *se_intrdev;	/* link back to associated unit */
	struct	esg se_sg[17];		/* scatter/gather map */
	struct	estatus se_est;		/* returned command status */
};
#endif

/*
 * Host adapter inquiry data
 */
#ifndef LOCORE
struct ehinq {
	u_short		eh_type;	/* always indicates a processor */
	u_short		eh_support;	/* SCSI-2 support level */
	u_char		eh_addlen;	/* data available */
	u_char		eh_targluns;	/* LUNs reserved for target mode */
	u_char		eh_ecbs;	/* max number of ECBs supported */
	u_char		eh_flags;	/* random bits */
	char		eh_vendor[8];	/* name of vendor */
	char		eh_product[8];	/* name of product */
	char		eh_firmware[8];	/* firmware type */
	char		eh_rev[4];	/* firmware revision level */
	char		eh_date[8];	/* release date for firmware */
	char		eh_time[8];	/* release time for firmware */
	u_short		eh_cksum;	/* firmware checksum */
};
#endif

#define	EHT_TMD		0x0020		/* target mode disabled */
#define	EHT_TMS		0x0040		/* target mode unsupported */

#define	EHS_AEN		0x8000		/* AENs supported */

#define	EHF_DIF		0x04		/* differential SCSI */
#define	EHF_LNK		0x08		/* linked commands supported */
#define	EHF_SYN		0x10		/* synchronous transfers supported */
#define	EHF_WID		0x20		/* wide (16-bit) data transfers */

/*
 * SCSI configuration data
 */
#ifndef LOCORE
struct escd {
	u_short	ec_target[16];
};
#endif

#define	ESC_DISCONN	0x0004		/* enable disconnection */
#define	ESC_SYNCH	0x0008		/* enable synch transfer negotiation */
#define	ESC_PARITY	0x0020		/* enable parity checking */
#define	ESC_XFER_10_0	0x0000		/* 10.0 MB/s synch transfer rate */
#define	ESC_XFER_6_7	0x0100		/* 6.7 MB/s synch transfer rate */
#define	ESC_XFER_5_0	0x0200		/* 5.0 MB/s synch transfer rate */
#define	ESC_XFER_4_0	0x0300		/* 4.0 MB/s synch transfer rate */
#define	ESC_XFER_3_3	0x0400		/* 3.3 MB/s synch transfer rate */
#define	ESC_XFER_2_9	0x0500		/* 2.9 MB/s synch transfer rate */
#define	ESC_XFER_2_5	0x0600		/* 2.5 MB/s synch transfer rate */
#define	ESC_XFER_2_2	0x0700		/* 2.2 MB/s synch transfer rate */
