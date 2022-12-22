/*-
 * Copyright (c) 1991, 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ahareg.h,v 1.5 1993/04/14 20:08:06 karels Exp $
 */

/*
 * Adaptec AHA-1542[BC] definitions.
 */

/*
 * Ports
 */
#ifndef LOCORE
#define	AHA_STAT(a)		((a) + 0)
#define	AHA_DATA(a)		((a) + 1)
#define	AHA_INTR(a)		((a) + 2)
#else
#define	AHA_STAT		0x330
#define	AHA_DATA		0x331
#define	AHA_INTR		0x332
#endif
#define	AHA_NPORT		3

/*
 * Bits
 */
/* control bits (status write) */
#define	AHA_C_HRST		0x80	/* hard reset */
#define	AHA_C_SRST		0x40	/* soft reset */
#define	AHA_C_IRST		0x20	/* interrupt reset */
#define	AHA_C_SCRST		0x10	/* SCSI bus reset */
/* status bits (status read) */
#define	AHA_S_STST		0x80	/* self test in progress */
#define	AHA_S_DIAGF		0x40	/* internal diagnostic failure */
#define	AHA_S_INIT		0x20	/* mailbox initialization required */
#define	AHA_S_IDLE		0x10	/* SCSI host adapter idle */
#define	AHA_S_CDF		0x08	/* command/data out port full */
#define	AHA_S_DF		0x04	/* data in port full */
#define	AHA_S_RSV02		0X02	/* reserved bit */
#define	AHA_S_INVDCMD		0x01	/* invalid host adapter command */
/* interrupt bits (interrupt read) */
#define	AHA_I_ANY		0x80	/* any interrupt */
#define	AHA_I_RSV40		0X40	/* reserved bit */
#define	AHA_I_RSV20		0X20	/* reserved bit */
#define	AHA_I_RSV10		0X10	/* reserved bit */
#define	AHA_I_SCRD		0x08	/* SCSI reset detected */
#define	AHA_I_HACC		0x04	/* host adapter command complete */
#define	AHA_I_MBOA		0x02	/* mailbox out empty */
#define	AHA_I_MBIF		0x01	/* mailbox in full */

/*
 * Host adapter commands
 */
#define	AHA_NOP			0x00	/* no-op */
#define	AHA_MBOX_INIT		0x01	/* mailbox initialization */
#define	AHA_START_SCSI_CMD	0x02	/* start SCSI command */
#define	AHA_START_BIOS_CMD	0x03	/* start PC/AT BIOS command (diag) */
#define	AHA_INQUIRY		0x04	/* adapter inquiry */
#define	AHA_MBOX_OUT_INTR	0x05	/* enable mailbox out interrupt */
#define	AHA_SELECT_TIMEOUT	0x06	/* set selection timeout */
#define	AHA_BUS_ON_TIME		0x07	/* set bus-on time */
#define	AHA_BUS_OFF_TIME	0x08	/* set bus-off time */
#define	AHA_TRANSFER_SPEED	0x09	/* set transfer speed */
#define	AHA_INSTALLED_DEVS	0x0a	/* return installed devices */
#define	AHA_CONFIG_DATA		0x0b	/* return configuration data */
#define	AHA_TARGET_MODE		0x0c	/* enable target mode */
#define	AHA_SETUP_DATA		0x0d	/* return setup data */
#define	AHA_WRITE_CHAN2		0x1a	/* write adapter channel 2 buffer */
#define	AHA_READ_CHAN2		0x1b	/* read adapter channel 2 buffer */
#define	AHA_WRITE_FIFO		0x1c	/* write adapter FIFO buffer */
#define	AHA_READ_FIFO		0x1d	/* read adapter FIFO buffer */
#define	AHA_ECHO		0x1f	/* echo byte thru data in port */
#define	AHA_DIAG		0x20	/* run adapter diagnostic */
#define	AHA_OPTIONS		0x21	/* set adapter options */
#define	AHA_SET_EEPROM		0x22	/* program 1542C EEPROM */
#define	AHA_GET_EEPROM		0x23	/* return 1542C EEPROM data */
#define	AHA_SHADOW		0x24	/* set shadow RAM parameters */
#define	AHA_BIOS_MBOX		0x25	/* initialize BIOS mailbox */
#define	AHA_BIOS_BANK1		0x26	/* set BIOS bank 1 */
#define	AHA_BIOS_BANK2		0x27	/* set BIOS bank 2 */
#define	AHA_GET_LOCK		0x28	/* get mbox init lock code */
#define	AHA_SET_LOCK		0x29	/* set or clear mbox init lock code */
#define	AHA_BIOS_START_CMD	0x82	/* start BIOS SCSI command */

/*
 * Mailboxes
 *
 * There are two kinds of mailboxes, inboxes and outboxes.
 * CCBs are dropped into outboxes and picked up from inboxes.
 * The mailbox area consists of a number of outboxes followed
 * by an equal number of inboxes.
 */
#ifndef LOCORE
struct mbox {
	u_char	mb_cmd;		/* command/status byte */
#define	mb_status	mb_cmd
	u_char	mb_ccb[3];	/* 3-byte big-endian physical ccb address */
};
#endif

/* out mbox command bytes */
#define	MBOX_O_FREE		0x0
#define	MBOX_O_START		0x1	/* request to start processing a ccb */
#define	MBOX_O_ABORT		0x2	/* request to abort a ccb */
/* in mbox status bytes */
#define	MBOX_I_FREE		0x0
#define	MBOX_I_COMPLETED	0x1	/* ccb completed without error */
#define	MBOX_I_ABORTED		0x2	/* ccb aborted by host */
#define	MBOX_I_ABORT_FAILED	0x3	/* couldn't find ccb to abort */
#define	MBOX_I_ERROR		0x4	/* ccb completed with error */

/*
 * Command control block
 */
#ifndef LOCORE
struct ccb {
	u_char	ccb_opcode;
	u_char	ccb_control;	/* address and control byte */
	u_char	ccb_cdblen;
	u_char	ccb_rqslen;	/* request sense allocation in bytes */
	u_char	ccb_datalen[3];
	u_char	ccb_data[3];	/* 3-byte big-endian physical data address */
	u_char	ccb_link[3];
	u_char	ccb_linkid;
	u_char	ccb_hastat;	/* host adapter status */
	u_char	ccb_tarstat;	/* target device status */
	u_char	ccb_rsv1;
	u_char	ccb_rsv2;
	union {
		struct	scsi_cdb6 ccb_ucdb6;
		struct	scsi_cdb10 ccb_ucdb10;
		/* size 12 CDBs? */
		u_char	ccb_ucdbbytes[16];
	} ccb_ucdb;
};
#define	ccb_cdb6	ccb_ucdb.ccb_ucdb6
#define	ccb_cdb10	ccb_ucdb.ccb_ucdb10
#define	ccb_cdb		ccb_cdb10
#define	ccb_cdbbytes	ccb_ucdb.ccb_ucdbbytes
#else
#define	CCB_OPCODE	0
#define	CCB_CTRL	1
#define	CCB_CDBLEN	2
#define	CCB_RQSLEN	3
#define	CCB_DATALEN	4
#define	CCB_DATA	7
#define	CCB_LINK	10
#define	CCB_LINKID	13
#define	CCB_HASTAT	14
#define	CCB_TARSTAT	15
#define	CCB_CDB		18
#endif

#define	CCB_FREE	0xff	/* unused ccb */
#define	CCB_CMD		0x00	/* SCSI initiator ccb */
#define	CCB_TARGET	0x01	/* SCSI target ccb */
#define	CCB_CMD_SG	0x02	/* SCSI initiator ccb with scatter/gather */
#define	CCB_CMD_RDL	0x03	/* SCSI initiator ccb with residual length */
#define	CCB_CMD_SG_RDL	0x04	/* scatter/gather and residual length */
#define	CCB_RESET	0x81	/* SCSI bus device reset */

#define	CCB_C_TARGET(c)	(((c)>>5) & 7)	/* SCSI ID of target */
#define	CCB_C_OUT	0x10		/* outbound data */
#define	CCB_C_IN	0x08		/* inbound data */
#define	CCB_C_LUN(c)	((c) & 7)	/* logical unit number on target */
#define	CCB_CONTROL(t, w, r, l) \
	(((t) & 7) << 5 | ((w) & 1) << 4 | ((r) & 1) << 3 | ((l) & 7))

#define	CCB_H_NORMAL	0x00	/* no host adapter detected error */
#define	CCB_H_TIMEOUT	0x11	/* target selection timeout */
#define	CCB_H_OVERRUN	0x12	/* data overrun or underrun */
#define	CCB_H_BUSFREE	0x13	/* target unexpectedly freed the SCSI bus */
#define	CCB_H_PHASE	0x14	/* target bus phase sequence error (reset!) */
#define	CCB_H_INVCCB	0x16	/* invalid ccb opcode */
#define	CCB_H_LINKLUN	0x17	/* linked ccb doesn't have same LUN */
#define	CCB_H_INVTDIR	0x18	/* invalid target direction in target mode */
#define	CCB_H_DUPCCB	0x19	/* duplicate ccb received in target mode */
#define	CCB_H_INVPARM	0x1a	/* invalid ccb or segment list parameter */

#ifndef LOCORE
/*
 * Scatter/gather maps
 */
struct sg {
	u_char sg_len[3];		/* contiguous length */
	u_char sg_addr[3];		/* physical address of a buffer page */
};

/*
 * Software CCB
 */

#define	MAXSG		17

struct soft_ccb {
	struct	ccb sccb_ccb;
	scintr_fn sccb_intr;		/* base interrupt handler */
	struct	device *sccb_intrdev;	/* link back to associated unit */
	time_t	sccb_stamp;		/* timestamp (for use by watchdog) */
	int	sccb_bounces;		/* number of bounce-ins needed */
	struct	sg sccb_sg[MAXSG];	/* scatter/gather map */
};

/*
 * DMA bounce support
 */
struct aha_bounce {
	vm_offset_t	bo_p;		/* physical address of bounce page */
	vm_offset_t	bo_v;		/* virtual address of bounce page */
	vm_offset_t	bo_dst;		/* virtual address of destination */
	vm_size_t	bo_len;		/* amount of data in the page */
};
#endif
