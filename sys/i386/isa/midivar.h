/*	BSDI $Id: midivar.h,v 1.1 1992/08/21 21:15:31 karels Exp $	*/

#define NUM_TRKS	8
#define NOOP_DELAY	0x10
#define MIDI_ME		0xf9
#define MIDI_DATAEND	0xfc
#define MIDI_TOFLOW	0xf8

/*
 *	midi port offsets
 */
#define	MIDI_DATA			0
#define	MIDI_STATUS			1
#define	MIDI_COMMAND			1

/*
 *	midi data transfer status bits
 */
#define	MIDI_RDY_RCV			(1 << 6)
#define	MIDI_DATA_AVL			(1 << 7)

/*
 *	do larger transfers in write, so use water marks
 */
#define	MIDI_Q_SIZE	1024
#define	MIDI_LOW_MARK	40
#define	MIDI_HIGH_MARK	(MIDI_Q_SIZE - 40)

/*
 *	 control status bits
 */
#define	MIDI_INTELLIGENT		(1 << 0)	/* control */
#define	MIDI_EXISTS			(1 << 1)	/* control */
#define	MIDI_OPEN			(1 << 2)	/* control */
#define MIDI_ABLOCK			(1 << 3)	/* control */
#define	MIDI_PLAYING			(1 << 4)	/* control */
#define MIDI_RECORDING			(1 << 5)	/* control */
#define	MIDI_WABORT			(1 << 6)	/* control */
#define MIDI_STOP_TRAN			(1 << 7)	/* control */
#define MIDI_TIMING_ON			(1 << 8)	/* control */
#define MIDI_NEED_DATA			(1 << 9)	/* control */
#define MIDI_RABORT			(1 << 10)	/* control */
#define MIDI_RSLEEP			(1 << 11)	/* control & cond */
#define MIDI_RBLOCK			(1 << 12)	/* control & cond */
#define MIDI_BLOCKING			(1 << 13)	/* all */

#define	MIDI_WBLOCK			(1 << 0)	/* track & cond */
#define MIDI_FBLOCK			(1 << 1)	/* track & cond */
#define MIDI_WSLEEP			(1 << 2)	/* track & cond */

/*
 *	minor numbers
 */
#define CONTROL_UNIT	128
#define CONDUCTOR_UNIT	8

/*
 *	midi acknowledgement
 */
#define  MIDI_ACK			0xfe

/*
 *	time out on transfers after this many tries
 */
#define MIDI_TRIES			10000
