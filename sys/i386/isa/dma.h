/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: dma.h,v 1.6 1993/02/21 02:12:27 karels Exp $
 */

/*
 * Direct memory access (PC/AT  8237A-5)
 */

/* DMA controller 1 (channels 0-3) -- byte i/o */
/* Ports */
#define DMA1_ADDR(chan)  (0x0 + (chan) * 2)	/* Current address */
#define DMA1_COUNT(chan) (0x1 + (chan) * 2)	/* Current byte count */
#define DMA1_STATUS	0x8			/* Status register (r) */
#define DMA1_COMMAND	0x8			/* Command register (w) */
#define DMA1_MASK	0xa			/* DMA request masks */
#define DMA1_MODE	0xb			/* Mode register */
#define DMA1_CLEARFF	0xc			/* Clear flip-flop */
#define DMA1_RESET	0xd			/* Reset DMA controller (w) */
#define DMA1_CLRMASK	0xe			/* Clear mask register */
#define DMA1_ALLMASK	0xf			/* All mask register bits */

/* page register */
#define DMA1_PAGEREG(chan) (((unsigned char *)"\207\203\201\202")[chan])

/*
 * DMA controller 2 (channels 4-7) -- word i/o
 * Use argument chan decremented by 4.
 */
#define DMA2_ADDR(chan)  (0xc0 + (chan) * 4)	/* Current address */
#define DMA2_COUNT(chan) (0xc2 + (chan) * 4)	/* Current byte count */
#define DMA2_STATUS	0xd0			/* Status register (r) */
#define DMA2_COMMAND	0xd0			/* Command register (w) */
#define DMA2_MASK	0xd4			/* DMA request masks */
#define DMA2_MODE	0xd6			/* Mode register */
#define DMA2_CLEARFF	0xd8			/* Clear flip-flop */
#define DMA2_RESET	0xda			/* Reset DMA controller (w) */
#define DMA2_CLRMASK	0xdc			/* Clear mask register */
#define DMA2_ALLMASK	0xde			/* All mask register bits */

/* page register */
#define DMA2_PAGEREG(chan) (((unsigned char *)"\217\213\211\212")[chan])

/*
 * Bits in various registers 
 */
/* Command register */
#define DMA_DISABLE	4			/* Disable DMA */
#define DMA_ENABLE	0			/* Enable DMA */

/* Status register bits */
#define DMA_TERM(chan)		(1 << (chan))	/* Terminal count on chan */
#define DMA_REQPEND(chan)	(0x10 << (chan)) /* Request pending on chan */

/* Mask register commands */
#define DMA_ECHAN(chan)		(chan)		/* Enable DMA on channel */
#define DMA_DCHAN(chan)		(DMA_DISABLE + (chan))	/* Disable DMA chan */

/* Allmask register bits */
#define DMA_MBIT(chan)		(1 << (chan))	/* Channel enabled */

/* Mode register bits */
#define DMA_CNUM(chan)		(chan)		/* Channel # */
#define DMA_WRITE		0x48		/* Set write mode */
#define DMA_READ		0x44		/* Set read mode */
#define DMA_AUTOINIT		0x10		/* Autoinitialization */
#define DMA_DEC			0x20		/* Decrement address */
#define DMA_CASCADE		0xc0		/* Set cascade mode */

/*
 * DMA state per channel
 */
struct at_dma_state {
	caddr_t	bounce;		/* copy buffer */
	int	maxsz;		/* size of copy buffer */
	int	bphys;		/* physical adr of the copy buffer */
	int	nbytes;		/* number of bytes to copy */
	caddr_t	addr;		/* address to copy */
};

extern struct at_dma_state at_dma_state[8];

/* Setup DMA on a channel */
extern void at_setup_dmachan __P((int chan, int maxsz));

/* Start DMA on a channel */
extern void at_dma __P((int isread, caddr_t addr, int nbytes, int chan));

/* Abort a DMA operation */
extern void at_dma_abort __P((int chan));

/* Terminate DMA */
#define at_dma_terminate(chan) \
	if (at_dma_state[chan].addr != NULL) { \
		bcopy(at_dma_state[chan].bounce, at_dma_state[chan].addr, \
		      at_dma_state[chan].nbytes); \
		at_dma_state[chan].addr = NULL; \
	}

/* Set DMA cascade mode */
extern void at_dma_cascade __P((int chan));
