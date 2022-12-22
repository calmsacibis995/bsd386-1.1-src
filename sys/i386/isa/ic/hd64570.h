/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: hd64570.h,v 1.1 1993/02/22 16:49:05 karels Exp $
 */

/*
 * Hitachi Serial Communications Adapter HD64570 chip definitions
 *
 * The register addresses are as for CPU modes 0 & 1.
 *      (R) - read-only; (W) - write-only
 */

/*
 * Program-accessible registers
 */

#define MCHAN   0x20    /* added to MSCI register address to select CHAN 1 */
#define CHAN    0x40    /* added to DMA register address to select CHAN 1 */
#define TX      0x20    /* added to DMA register address to select TX */
#define TCHAN   0x10    /* same as CHAN for timer registers */
#define TTX     0x08    /* same as TX for timer registers */

/* Low-Power Mode Control Register (common for all channels) */
#define LPR     0x00

/* Interrupt Control Registers */
#define IVR     0x1a    /* Interrupt Vector Register */
#define IMVR    0x1c    /* Interrupt Modified Vector Register */
#define ITCR    0x18    /* InTerrupt Control Register */
#define ISR0    0x10    /* Interrupt Status Register 0 (R) */
#define ISR1    0x11    /* Interrupt Status Register 1 (R) */
#define ISR2    0x12    /* Interrupt Status Register 2 (R) */
#define IER0    0x14    /* Interrupt Enable Register 0 */
#define IER1    0x15    /* Interrupt Enable Register 1 */
#define IER2    0x16    /* Interrupt Enable Register 2 */

/* Multiprotocol Serial Controller Interface Registers (+MCHAN) */
#define MD0     0x2e    /* MoDe register 0 */
#define MD1     0x2f    /* MoDe register 1 */
#define MD2     0x30    /* MoDe register 2 */
#define CTL     0x31    /* ConTroL register */
#define RXS     0x36    /* RX clock Source register */
#define TXS     0x37    /* TX clock Source register */
#define TMC     0x35    /* TiMe Constant register */
#define CMD     0x2c    /* CoMmanD register (W) */
#define ST0     0x22    /* STatus register 0 (R) */
#define ST1     0x23    /* STatus register 1 */
#define ST2     0x24    /* STatus register 2 */
#define ST3     0x25    /* STatus register 3 (R) */
#define FST     0x26    /* Frame STatus register */
#define IE0     0x28    /* Interrupt Enable register 0 */
#define IE1     0x29    /* Interrupt Enable register 1 */
#define IE2     0x2a    /* Interrupt Enable register 2 */
#define FIE     0x2b    /* Frame Interrupt Enable register */
#define SA0     0x32    /* Sync/Address register 0 */
#define SA1     0x33    /* Sync/Address register 1 */
#define IDL     0x34    /* IDLe pattern register */
#define TRBL    0x20    /* Tx/Rx Buffer register Low */
#define TRBH    0x21    /* Tx/Rx Buffer register Low */
#define RRC     0x3a    /* Rx Ready Control register */
#define TRC0    0x38    /* Tx Ready Control register 0 */
#define TRC1    0x39    /* Tx Ready Control register 1 */
#define CST0    0x3c    /* Current STatus register 0 */
#define CST1    0x3d    /* Current STatus register 1 */

/* DMA Controller Registers (common to all channels) */
#define PCR     0x08    /* dma Priority Control Register */
#define DMER    0x09    /* dma Master Enable Register */

/* DMA Controller Registers (+CHAN +TX) */
#define DARL    0x80    /* Destination Address Register Low 0-7 */
#define BARL DARL       /* Buffer Address Register Low */
#define DARH    0x81    /* Destination Address Register High 8-15 */
#define BARH DARH       /* Buffer Address Register High */
#define DARB    0x82    /* Destination Address Register Block 16-23 */
#define BARB DARB       /* Buffer Address Register Block */
#define SARL    0x84    /* Source Address Register Low 0-7 */
#define SARH    0x85    /* Source Address Register High 8-15 */
#define SARB    0x86    /* Source Address Register Block 16-23 */
#define CPB  SARB       /* Chain Pointer Base  */
#define CDAL    0x88    /* Current Descriptor Address Low */
#define CDAH    0x89    /* Current Descriptor Address High */
#define EDAL    0x8a    /* Error Descriptor Address Low */
#define EDAH    0x8b    /* Error Descriptor Address High */
#define BFLL    0x8c    /* receive BuFfer Length Low (RX only) */
#define BFLH    0x8d    /* receive BuFfer Length High (RX only) */
#define BCRL    0x8e    /* Byte Count Register Low */
#define BCRH    0x8f    /* Byte Count Register High */
#define DSR     0x90    /* Dma Status Register */
#define DMR     0x91    /* Dma Mode Regietr */
#define FCT     0x93    /* end-of-Frame interrupt CounTer */
#define DIR     0x94    /* Dma Interrupt enable Register */
#define DCR     0x95    /* Dma Command Register (W) */

/* Timer Registers (+TCHAN +TTX) */
#define TCNTL   0x60    /* Timer up CouNTer Low */
#define TCNTH   0x61    /* Timer up CouNTer High */
#define TCONRL  0x62    /* Timer CONstant Register Low (W) */
#define TCONRH  0x63    /* Timer CONstant Register High (W) */
#define TCSR    0x64    /* Timer Control/Status Register */
#define TEPR    0x65    /* Timer Expand Prescale Register */

/* Wait Controller Registers (common to all channels) */
#define PABR0   0x02    /* Physical Address Boundary Register 0 */
#define PABR1   0x03    /* Physical Address Boundary Register 1 */
#define WCRL    0x04    /* Wait Control Register Low */
#define WCRM    0x05    /* Wait Control Register Middle */
#define WCRH    0x06    /* Wait Control Register High */

/*
 * Low-Power Register bits
 */
#define LPR_IOSTP       0x1     /* I/O stop */

/*
 * Modified interrupt vectors
 */
#define IMVR_TYPE       0x1e    /* type */
#define  IMVR_RXRDY      0x04    /* RX ready from MSCI */
#define  IMVR_TXRDY      0x06    /* TX ready from MSCI */
#define  IMVR_RXINT      0x08    /* RX interrupt from MSCI */
#define  IMVR_TXINT      0x0a    /* TX interrupt from MSCI */
#define  IMVR_RXDA       0x14    /* DMAC A intr on RX */
#define  IMVR_RXDB       0x16    /* DMAC B intr on RX */
#define  IMVR_TXDA       0x18    /* DMAC A intr on TX */
#define  IMVR_TXDB       0x2a    /* DMAC B intr on TX */
#define  IMVR_RXTMR      0x1c    /* RX timer */
#define  IMVR_TXTMR      0x1e    /* TX timer */
#define IMVR_CHAN       0x20    /* source channel */
#define IMVR_USER       0xc0    /* user bits */

/*
 * Interrupt Control Register bits
 */
#define ITCR_VOS        0x10    /* Vector Output (1=mod vect) */
#define ITCR_IAK        0x60    /* Interrupt Acknowledge Cycle */
#define  ITCR_NIAK       0x00    /* no-acknowledge */
#define  ITCR_SIAK       0x20    /* single acknowledge */
#define  ITCR_DIAK       0x40    /* double acknowledge */
#define ITCR_IPC        0x80    /* Interrupt Priority Control */

/*
 * Interrupt Status Register 0 and
 * Interrupt Enable Register 0 bits
 */
#define ISR0_RXRDY0     0x01    /* RX ready, chan 0 */
#define ISR0_TXRDY0     0x02    /* TX ready, chan 0 */
#define ISR0_RXINT0     0x04    /* RX interrupt, chan 0 */
#define ISR0_TXINT0     0x08    /* TX interrupt, chan 0 */
#define ISR0_RXRDY1     0x10    /* RX ready, chan 1 */
#define ISR0_TXRDY1     0x20    /* TX ready, chan 1 */
#define ISR0_RXINT1     0x40    /* RX interrupt, chan 1 */
#define ISR0_TXINT1     0x80    /* TX interrupt, chan 1 */

/*
 * Interrupt Status Register 1 and
 * Interrupt Enable Register 1 bits
 */
#define ISR1_DMIA0R     0x01    /* DMAC requesting A intr, chan 0 RX */
#define ISR1_DMIB0R     0x02    /* DMAC requesting B intr, chan 0 RX */
#define ISR1_DMIA0T     0x04    /* DMAC requesting A intr, chan 0 TX */
#define ISR1_DMIB0T     0x08    /* DMAC requesting B intr, chan 0 TX */
#define ISR1_DMIA1R     0x10    /* DMAC requesting A intr, chan 1 RX */
#define ISR1_DMIB1R     0x20    /* DMAC requesting B intr, chan 1 RX */
#define ISR1_DMIA1T     0x40    /* DMAC requesting A intr, chan 1 TX */
#define ISR1_DMIB1T     0x80    /* DMAC requesting B intr, chan 1 TX */

/*
 * Interrupt Status Register 2 and
 * Interrupt Enable Register 2 bits
 */
#define ISR2_TIRQ0R     0x10    /* Timer intr requeat, chan 0 RX */
#define ISR2_TIRQ0T     0x20    /* Timer intr requeat, chan 0 TX */
#define ISR2_TIRQ1R     0x40    /* Timer intr requeat, chan 1 RX */
#define ISR2_TIRQ1T     0x80    /* Timer intr requeat, chan 1 TX */

/*
 * MSCI Mode Register 0 bits
 */
#define MD0_STOPS       0x03    /* Stop bit length (async mode) */
#define  MD0_STOP1       0x00    /* 1 stop bit */
#define  MD0_STOP15      0x01    /* 1.5 stop bits */
#define  MD0_STOP2       0x02    /* 2 stop bits */
#define MD0_CRCINIT     0x01    /* init CRC is all 1s (0 for all 0s) */
#define MD0_CRCCCITT    0x02    /* use CCITT CRC polynome (0 for CRC-16) */
#define MD0_CRCENB      0x04    /* CRC calculation enabled */
#define MD0_AUTO        0x10    /* Auto-enable (hw flow control) */
#define MD0_PRTCL       0xe0    /* Protocol mode */
#define  MD0_ASYNC       0x00    /* asynchronous mode */
#define  MD0_BYMONO      0x20    /* byte-sync mono-sync mode */
#define  MD0_BYBI        0x40    /* byte-sync bi-sync mode */
#define  MD0_BYEXT       0x60    /* byte-sync external sync mode */
#define  MD0_HDLC        0x80    /* bit-sync HDLC mode */
#define  MD0_LOOP        0xa0    /* bit-sync loop mode */

/*
 * MSCI Mode Register 1 bits
 */
#define MD1_PMPM        0x03    /* Parity/Multiprocessor mode */
#define  MD1_NOPA        0x00    /* No parity/MP bit */
#define  MD1_MP          0x01    /* Append MP bit */
#define  MD1_EVEN        0x02    /* Even parity */
#define  MD1_ODD         0x03    /* Odd parity */
#define MD1_RXCHR       0x0c    /* Receive character length */
#define  MD1_RXCHR8      0x00    /* 8 bits */
#define  MD1_RXCHR7      0x04    /* 7 bits */
#define  MD1_RXCHR6      0x08    /* 6 bits */
#define  MD1_RXCHR5      0x0c    /* 5 bits */
#define MD1_TXCHR       0x30    /* Transmit character length */
#define  MD1_TXCHR8      0x00    /* 8 bits */
#define  MD1_TXCHR7      0x10    /* 7 bits */
#define  MD1_TXCHR6      0x20    /* 6 bits */
#define  MD1_TXCHR5      0x30    /* 5 bits */
#define MD1_BRATE       0xc0    /* Bit rate (async mode) */
#define  MD1_BR1         0x00    /* 1/1 clock rate */
#define  MD1_BR16        0x40    /* 1/16 clock rate */
#define  MD1_BR32        0x80    /* 1/32 clock rate */
#define  MD1_BR64        0xc0    /* 1/64 clock rate */
#define MD1_ADDRS       0xc0    /* Address field check (bit sync mode) */
#define  MD1_NOADR       0x00    /* don't check */
#define  MD1_ADR1        0x40    /* check address byte 1 */
#define  MD1_ADR2        0x80    /* check address byte 2 */
#define  MD1_DADR        0xc0    /* check both address bytes */

/*
 * MSCI Mode Register 2 bits
 */
#define MD2_CNCT        0x03    /* Channel connection */
#define  MD2_DUPLEX      0x00    /* full duplex */
#define  MD2_ECHO        0x01    /* auto echo */
#define  MD2_LOOPBACK    0x03    /* local loop back */
#define MD2_DRATE       0x18    /* ADPLL operating clock/bit rate (sync) */
#define  MD2_DR8         0x00    /* x 8 */
#define  MD2_DR16        0x08    /* x 16 */
#define  MD2_DR32        0x10    /* x 32 */
#define MD2_CODE        0xe0    /* Transmission code type (sync) */
#define  MD2_NRZ         0x00    /* Non-Return to Zero */
#define  MD2_NRZI        0x20    /* NRZ with Inversion */
#define  MD2_MCH         0x80    /* Manchester code */
#define  MD2_FM1         0xa0    /* Frequency modulation (hi freq = 1) */
#define  MD2_FM0         0xc0    /* Frequency modulation (hi freq = 0) */

/*
 * MSCI Control Register bits
 */
#define CTL_RTS         0x01    /* 0 = RTS on */
#define CTL_GOP         0x02    /* Go active on poll (bit sync loop mode) */
#define CTL_SYNCLD      0x04    /* Sync char load enable (byte sync mode) */
#define CTL_BRK         0x08    /* Send break (async mode) */
#define CTL_IDLC        0x10    /* 0 - mark on idle; 1 - idle pattern on idle (sync) */
#define CTL_UNDRC       0x20    /* Underrun state control (1-add CRC/FCS) (sync) */

/*
 * MSCI RX Clock Source Register bits
 */
#define RXS_BR          0x0f    /* Clock division ratio = 2**n (0<=n<=9) */
#define RXS_SRC         0x70    /* Clock source */
#define  RXS_LINE        0x00    /* RXC line input */
#define  RXS_LNS         0x20    /* RXC line input with noise suppression */
#define  RXS_BRG         0x40    /* iternal baud rate generator */
#define  RXS_ALINE       0x60    /* from ADPLL from RXC line */
#define  RXS_ABRG        0x70    /* from ADPLL from internal BRG */

/*
 * MSCI TX Clock Source Register bits
 */
#define TXS_BR          0x0f    /* Clock division ratio = 2**n (0<=n<=9) */
#define TXS_SRC         0x70    /* Clock source */
#define  TXS_LINE        0x00    /* TXC line input */
#define  TXS_BRG         0x40    /* iternal baud rate generator */
#define  TXS_RXC         0x60    /* receive clock */

/*
 * MSCI Command Register values (commands)
 */
#define CMD_NOP         0x00    /* No operation */
#define CMD_TXRST       0x01    /* TX reset */
#define CMD_TXENB       0x02    /* TX enable */
#define CMD_TXDIS       0x03    /* TX disable */
#define CMD_TXCRCINI    0x04    /* TX CRC initialization */
#define CMD_TXCRCEXCL   0x05    /* TX CRC exclusion */
#define CMD_EOM         0x06    /* End-of-message */
#define CMD_ABORT       0x07    /* Abort transmission */
#define CMD_MP          0x08    /* MP bit on */
#define CMD_TXCLR       0x09    /* TX buffer clear */
#define CMD_RXRST       0x11    /* RX reset */
#define CMD_RXENB       0x12    /* RX enable */
#define CMD_RXDIS       0x13    /* RX disable */
#define CMD_RXCRCINI    0x14    /* RX CRC initialization */
#define CMD_MREJ        0x15    /* Messaje reject */
#define CMD_SMP         0x16    /* Search MP bit */
#define CMD_RXCRCEXCL   0x17    /* RX CRC exclusion */
#define CMD_RXCRCFRC    0x18    /* Forcing RX CRC calculation */
#define CMD_CHRST       0x21    /* Channel reset */
#define CMD_ESM         0x31    /* Enter search mode */

/*
 * MSCI Status Register 0 and
 * Interrupt Enable register 0 bits
 */
#define ST0_RXRDY       0x01    /* RX buffer full */
#define ST0_TXRDY       0x02    /* TX is possible */
#define ST0_RXINT       0x40    /* RX exception */
#define ST0_TXINT       0x80    /* TX exception */

/*
 * MSCI Status Register 1 and
 * Interrupt Enable register 1 bits
 */
#define ST1_BRKE        0x01    /* Break end (async) */
#define ST1_IDLD        0x01    /* Idle detected (bit sync) */
#define ST1_BRKD        0x02    /* Break detected (async) */
#define ST1_ABTD        0x02    /* Abort seq start detected (bit sync) */
#define ST1_CDCD        0x04    /* Change on DCD */
#define ST1_CCTS        0x08    /* Change on CTS */
#define ST1_SYNCD       0x10    /* SYN detected (byte sync) */
#define ST1_FLGD        0x10    /* Flag detected (bit sync) */
#define ST1_IDL         0x40    /* Transmitter idle status */
#define ST1_UDRN        0x80    /* Underrun error (sync) */

/*
 * MSCI Status Register 2, Current Status Registers 0/1,
 * Interrupt Enable register 2 and chain buffer descriptor status field bits
 *
 * Bit-synchronous mode only:
 * Those bits are also present in Frame Status Register (FST),
 * ST2_EOM is also used in Frame Interrupt Enable (FIE).
 */
#define CST_DE          0x01    /* Data Exists (CST0/1 only) */
#define ST2_EOT         0x01    /* End of Transfer (chain buf descr. only) */
#define ST2_CRCE        0x04    /* CRC error (sync) */
#define ST2_OVRN        0x08    /* Overrun error */
#define ST2_FRME        0x10    /* Framing error */
#define ST2_PE          0x20    /* Parity error (async) */
#define ST2_ABT         0x20    /* Abort end frame (bit sync) */
#define ST2_PMP         0x40    /* Parity/MP bit (async) */
#define ST2_SHRT        0x40    /* Short frame (bit sync) */
#define ST2_EOM         0x80    /* Receive end of message (bit sync) */

/*
 * MSCI Status Register 3 bits
 */
#define ST3_RXENBL      0x01    /* RX enabled */
#define ST3_TXENBL      0x02    /* TX enabled */
#define ST3_DCD         0x04    /* DCD line state */
#define ST3_CTS         0x08    /* CTS line state */
#define ST3_SRCH        0x10    /* ADPLL search mode (sync) */
#define ST3_SLOOP       0x20    /* Sending on loop (bit sync) */

/*
 * DMA Status Register and
 * DMA Interrupt enable Register bits
 */
#define DSR_DWD         0x01    /* DE bit write disable (DSR only) */
#define DSR_DE          0x02    /* DMA enable (DSR only) */
#define DSR_COF         0x10    /* Counter overflow (chain mode only) */
#define DSR_BOF         0x20    /* Buffer over/underflow (chain mode only) */
#define DSR_EOM         0x40    /* End of Frame transfer (chain mode only) */
#define DSR_EOT         0x80    /* End of transfer */

/*
 * DMA Mode Register bits
 */
#define DMR_CNTE        0x02    /* Frame-end intr counter (FCT) enable */
#define DMR_NF          0x04    /* Multi-frame transfer (chain mode only) */
#define DMR_TMOD        0x10    /* Chained-block transfer mode */

/*
 * DMA Command Register values (commands)
 */
#define DCR_ABORT       0x01    /* Software abort */
#define DCR_FCTCLR      0x02    /* Clear frame-end intr counter (FCT) */

/*
 * DMA Priority Register bits
 */
#define PCR_PRI         0x07    /* Channel Priority Policy */
#define  PCR_01RT        0x00    /* Chan 0 > Chan1, then RX > TX */
#define  PCR_10RT        0x01    /* Chan 1 > Chan0, then RX > TX */
#define  PCR_RT01        0x02    /* RX > TX, then Chan 0 > Chan 1 */
#define  PCR_TR01        0x03    /* TX > RX, then Chan 0 > Chan 1 */
#define  PCR_ROT         0x04    /* Rotating priority */
#define PCR_CCC         0x08    /* Channel change condition */
#define PCR_BRC         0x10    /* Bus release condition */

/*
 * DMA Master Enable Register bits
 */
#define DMER_DME        0x80    /* DMA master enable */

/*
 * Timer Control/Status Register bits
 */
#define TCSR_TME        0x10    /* Timer enable */
#define TCSR_ECMI       0x40    /* CMF interrupt enable */
#define TCSR_CMF        0x80    /* Compare match flag */


/*
 * The Chain Buffer Descriptor structure
 */
struct hd_cbd {
	u_short cbd_next;       /* chain pointer */
	u_short cbd_bp0;        /* buffer pointer -- lower 16 bits */
	u_short cbd_bp1;        /* buffer pointer -- higer 16 bits */
	u_short cbd_len;        /* data length */
	u_char  cbd_stat;       /* status byte, bits as in ST2 */
	u_char  cbd_unujsed;
};
