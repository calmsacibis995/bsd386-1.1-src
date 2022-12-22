/*	BSDI $Id: if_tnreg.h,v 1.2 1994/01/12 18:02:04 karels Exp $	*/

/*
 * i386/isa/if_tnreg.h by South Coast Computing Services, Inc.
 * Released without restrictions.
 */


/*
 *	As explained in i386/isa/ic/am79c960.h, the chip
 *	initialization logic really wants the buffer descriptor
 *	rings to have power-of-2 sizes between 1 and 128 entires,
 *	inclusive.  These macros are used in i386/isa/if_tn.c
 *	to set things up and control loops.  Don't even think
 *	about using different numbers for the N?RING and ?RING_CODE
 *	macros for the same choice of ?.  The literal numbers
 *	appear twice here for insuperable lexical reasons.
 */


/*
 * To be useful, Rx buffers need a Kbyte (MCLBYTES) of memory behind
 * them, so we only allocate "enough".  Input errors may indicate you
 * need to increase this (MISS), or they may indicate a DMA timing
 * (MERR) problem.
 *
 * If you try to drop this below 2 you will have to add some #if stuff
 * to whack the MTU down to MCLBYTES.
 */

#if MCLBYTES > 1024 && !defined(GATEWAY)
#define TN_NRRING	16	/* number of Rx descriptors to use */
#define TN_RRING_CODE	TNIB_LEN(16)
#else
#define TN_NRRING	32	/* number of Rx descriptors to use */
#define TN_RRING_CODE	TNIB_LEN(32)
#endif

/*
 * Tx descriptors are free -- allocate max unless you want less
 * for debugging or something.  Note that mbuf fragmentation
 * can cause a single packet to require five or more slots,
 * so don't lightly go below 8.
 */

#define TN_NTRING	128	/* number of Tx descriptors to use */
#define TN_TRING_CODE	TNIB_LEN(128)

#define TN_NREG		24	/* no VSW on the TNIC-1500 */
