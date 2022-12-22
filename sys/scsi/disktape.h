/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: disktape.h,v 1.3 1992/05/14 20:10:13 karels Exp $
 */

/*-
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory and its contributors.
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
 * from disktape.h,v 1.1 91/05/10 08:21:37 torek Exp   (LBL)
 */

/*
 * Commands common to disk and tape devices, but not other SCSI devices.
 */

/*
 * Structure of a MODE SENSE command (i.e., the cdb).
 */
struct scsi_cdb_modesense {
	u_char	cdb_cmd,	/* command */
		cdb_lun:3,	/* logical unit number */
		cdb_xxx0:5,	/* reserved */
		cdb_xxx1,	/* reserved */
		cdb_xxx2,	/* reserved */
		cdb_len,	/* allocation length */
		cdb_ctrl;	/* control byte */
};

/*
 * Structure of returned mode sense data.
 */
struct scsi_modesense {
	u_char	ms_len,		/* total sense data length */
		ms_mt,		/* medium type */
		ms_wbs,		/* write protect, buffered mode, & speed */
		ms_bdl;		/* block descriptor length */
};

#if 0	/* not supported in CCS */
#define	SCSI_CMD_MS_DBD		0x8
#endif

#define	PC_CURRENT	0
#define	PC_CHANGEABLE	1
#define	PC_DEFAULT	2
#define	PC_SAVED	3

/*
 * Block descriptors follow the mode sense header.
 */
struct scsi_ms_bdesc {
	u_char	dc,	/* density code */
		nbh,	/* number of blocks (MSB) */
		nbm,	/* number of blocks */
		nbl,	/* number of blocks (LSB) */
		xxx,	/* reserved */
		blh,	/* block length (MSB) */
		blm,	/* block length */
		bll;	/* block length (LSB) */
};

/*
 * Mode pages follow the block descriptors.
 */
struct scsi_ms_page {
	u_char	code,		/* page code */
		length,		/* page length */
		parm[1];	/* mode parameters */
};

/*
 * Structure of a PREVENT/ALLOW MEDIUM REMOVAL command.
 */
struct scsi_cdb_pamr {
	u_char	cdb_cmd,	/* 0x1e */
		cdb_lun:3,	/* logical unit number */
		cdb_xxx0:5,	/* reserved */
		cdb_xxx1,	/* reserved */
		cdb_xxx2,	/* reserved */
		cdb_prevent,	/* 1=prevent, 0=allow */
		cdb_ctrl;
};
