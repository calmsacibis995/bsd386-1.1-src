/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_exreg.h,v 1.2 1994/01/06 00:09:48 karels Exp $
 */

/*
 * Intel EtherExpress 16 Ethernet Adapter Driver
 *
 * This code is derived from the Intel EtherExpress driver
 * donated to BSDI by Korotin D.O. (mitia@pczz.msk.su).
 */

#define EX_NPORTS       16

/*
 * Valid IRQs -- 3, 4, 5, 9, 10, 11
 */
#define EX_IRQS 0x0e38
#define EX_IRQVALID(i)  (EX_IRQS & (1<<(i)))

/*
 * Valid base addresses
 * (200, 210, ... 270, 300, 310, ... 370)
 */
#define EX_IOBASEVALID(b)       (((b) & ~0x170) == 0x200)

#define DX_REG  0x0
#define WR_REG  0x2
#define RD_REG  0x4
#define CA_REG  0x6
#define IS_REG  0x7
#define SM_REG  0x8
#define MA_REG  0xA
#define MM_REG  0xB
#define PC_REG  0xC
#define CO_REG  0xD
#define EP_REG  0xE
#define ID_REG  0xF
#define MC_REG  0xF

/* shadow registers */
#define ECR1_REG	0x300E	/* on second generation only */
#define SHADOW_ID_REG	0x300F

/* values in ECR1 */
#define	ECR1_NOTAUI	0x80	/* 1 if not AUI connector */
#define	ECR1_TP		0x02	/* 1 if TP interface, 0 for BNC */

#define	EX_ID		0xBABA	/* in EF_ID */
#define	EX_ID_GEN1	0xBABA	/* in SHADOW_EF_ID: isa, first generation */
#define	EX_ID_GEN2	0xBABB	/* in SHADOW_EF_ID: isa, second generation */
#define	EX_ID_MCA	0xBABC	/* in SHADOW_EF_ID: mca */

#define OFFSET_SCP      0xFFF6
#define OFFSET_ISCP     (OFFSET_SCP-0x8)
#define OFFSET_SCB      (OFFSET_ISCP-0x10)
#define OFFSET_CU       0xF814
#define OFFSET_TBD      (OFFSET_CU+0x12)
#define OFFSET_TXB      (OFFSET_TBD+0x8)
#define OFFSET_RXB      (0x8000+0x60A)

#define N_FD    20
#define N_RBD   20

#define SCP_SYSBUS      (OFFSET_SCP)
#define SCP_ISCP        (OFFSET_SCP+0x6)
#define SCP_ISCP_BASE   (OFFSET_SCP+0x8)

#define ISCP_BUSY       (OFFSET_ISCP)
#define ISCP_SCB_OFFSET (OFFSET_ISCP+0x2)
#define ISCP_SCB        (OFFSET_ISCP+0x4)
#define ISCP_SCB_BASE   (OFFSET_ISCP+0x6)

#define SCB_STATUS      (OFFSET_SCB)
#define SCB_COMMAND     (OFFSET_SCB+0x2)
#define SCB_CBL_OFFSET  (OFFSET_SCB+0x4)
#define SCB_RFA_OFFSET  (OFFSET_SCB+0x6)
#define SCB_CRCERRS     (OFFSET_SCB+0x8)
#define SCB_ALNERRS     (OFFSET_SCB+0xA)
#define SCB_RSCERRS     (OFFSET_SCB+0xC)
#define SCB_OVRNERRS    (OFFSET_SCB+0xE)

#define CU_STATUS       (OFFSET_CU)
#define CU_COMMAND      (OFFSET_CU+0x2)
#define CU_LINK_OFFSET  (OFFSET_CU+0x4)

#define CU_TR_TBD_OFFSET (OFFSET_CU+0x6)
#define CU_TR_DSR_ADDR  (OFFSET_CU+0x8)
#define CU_TR_LENGTH    (OFFSET_CU+0xE)

#define CU_DATA         (OFFSET_CU+0x6)

#define SCB_SW_INT      0xF000
#define SCB_SW_RNR      0x1000
#define SCB_SW_CNA      0x2000
#define SCB_SW_FR       0x4000
#define SCB_SW_CX       0x8000

#define SCB_RUS_READY   0x0040
#define SCB_CU_STRT     0x0100
#define SCB_RESET       0x0080

#define AC_IASETUP      0x0001
#define AC_CONFIG       0x0002
#define AC_TRANSMIT     0x0004
#define AC_SW_OK        0x2000
#define AC_SW_EL        0x8000
#define AC_SW_C         0x8000
#define AC_SW_I         0x2000

#define RBD_SW_EOF      0x8000
#define RBD_SW_COUNT    0x3FFF

#define CMD_NULL        0xFFFF

/*
 * TBD Structure.
 */
#define TBD_COUNT(x)            (x)
#define TBD_NEXT_TBD(x)         ((x)+0x2)
#define TBD_BUFFER_ADDR(x)      ((x)+0x4)
#define TBD_BUFFER_BASE(x)      ((x)+0x6)

/*
 * FD Structure.
 */
#define FD_STATUS(x)    (x)
#define FD_COMMAND(x)   ((x)+0x2)
#define FD_LINK(x)      ((x)+0x4)
#define FD_RBD(x)       ((x)+0x6)
#define FD_DSR_ADDR(x)  ((x)+0x8)
#define FD_SRC_ADDR(x)  ((x)+0xE)
#define FD_LENGTH(x)    ((x)+0x14)

/*
 * RBD Structure.
 */
#define RBD_STATUS(x)           (x)
#define RBD_NEXT_RBD(x)         ((x)+0x2)
#define RBD_BUFFER_ADDR(x)      ((x)+0x4)
#define RBD_BUFFER_BASE(x)      ((x)+0x6)
#define RBD_SIZE(x)             ((x)+0x8)

#define EE_DI           0x04
#define EE_SK           0x01
#define EE_DO           0x08
#define EE_CS           0x02
#define EEXP_EPROM_READ 0x06

/*
 * EEPROM offsets and values; not sure what to name the words.
 * This is mostly incomplete.
 */
#define	EE_W0		0
#define	  EE0_NOTAUI	0x1000	/* 1 if not configured for AUI connector */

#define	EE_W1		1
#define	  EE1_AUTOSENS	0x0080	/* autosense the active connector? */

#define	EE_W5		5
#define	  EE5_TP	0x0001	/* 1 if configured for TP connector */
