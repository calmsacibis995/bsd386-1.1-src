/*	BSDI $Id: sb_regs.h,v 1.2 1994/01/30 00:58:49 karels Exp $	*/

/*======================================================================

  This original source for this file was released as part of
  SBlast-BSD-1.5 on 9/14/92 by Steve Haehnichen <shaehnic@ucsd.edu>
  under the terms of the GNU Public License.  Effective of 1/28/94, I
  (Steve Haehnichen) am replacing the GPL terms with a more
  kernel-friendly BSD-style agreement, as follows:

  *
  * Copyright (C) 1994 Steve Haehnichen.
  *
  * Permission to use, copy, modify, distribute, and sell this software
  * and its documentation for any purpose is hereby granted without
  * fee, provided that the above copyright notice appear in all copies
  * and that both that copyright notice and this permission notice
  * appear in supporting documentation.
  *
  * This software is provided `as-is'.  Steve Haehnichen disclaims all
  * warranties with regard to this software, including without
  * limitation all implied warranties of merchantability, fitness for
  * a particular purpose, or noninfringement.  In no event shall Steve
  * Haehnichen be liable for any damages whatsoever, including
  * special, incidental or consequential damages, including loss of
  * use, data, or profits, even if advised of the possibility thereof,
  * and regardless of whether in an action in contract, tort or
  * negligence, arising out of or in connection with the use or
  * performance of this software.
  *
  
======================================================================*/

/*
 * This is where you set the Sound Blaster DMA channel.
 * Possible settings are 0, 1, and 3.  The card defaults to
 * channel 1, which works just fine for me.
 *
 * The IRQ and I/O Port must be set in autoconf.c to match your card
 * settings.  See the directions in INSTALL for details.
 */
#define SB_DMA_CHAN 	1	/* 0, 1, or 3  (1 is factory default) */

/* Convenient byte masks */
#define B1(x)	((x) & 0x01)
#define B2(x)	((x) & 0x03)
#define B3(x)	((x) & 0x07)
#define B4(x)	((x) & 0x0f)
#define B5(x)	((x) & 0x1f)
#define B6(x)	((x) & 0x3f)
#define B7(x)	((x) & 0x7f)
#define B8(x)	((x) & 0xff)
#define F(x)	(!!(x))		/* 0 or 1 only */

/*
 * DMA registers & values. (Intel 8237: Direct Memory Access Controller)
 */

/* Mask register: Send DMA_MASK to this register to suspend DMA on that
 * channel, then DMA_UNMASK to allow transfers.  Generally, you
 * send DMA_MASK as the first thing in the DMA setup, then UNMASK it last */
#define DMA_MASK_REG 	0x0A	/* Mask register */
#define DMA_UNMASK	SB_DMA_CHAN
#define DMA_MASK	(DMA_UNMASK | 0x04)

/* The DMA_MODE register selects a Read or Write DMA transfer. */
#define DMA_MODE 	0x0B	/* Mode register (read/write to card) */
#define DMA_MODE_WRITE	(0x48 + SB_DMA_CHAN) /* DAC */
#define DMA_MODE_READ   (0x44 + SB_DMA_CHAN) /* ADC */

/* Use these MODE selects for auto-looping DMA transfers. */
#define DMA_AUTO_WRITE	(DMA_MODE_WRITE | 0x10)
#define DMA_AUTO_READ	(DMA_MODE_READ | 0x10)

/* This is a strange register.  Basically you just have to send
 * a byte to it before writing the address.  (It sets the address
 * state to LSB.)  Always send a 0x00 or anything else before 
 * writing the address. */
#define DMA_CLEAR	0x0C /* Send one byte before writing the address. */

/* This is where you send the address of the block of memory involved
 * in the transfer.  This block may start anywhere in the Page, but
 * the Count must not cross a 64K page boundary.   This is very important!
 * Write 2 bytes (low byte first, followed by the high byte) for the
 * least-significant 16 bits of the address. */
#define DMA_ADDRESS     (SB_DMA_CHAN << 1)

/* Write 2 bytes, low byte first for the 16 bits of the count.
   Note that one greater than this value will be copied!
   (i.e. the biggest size is 0xFFFF, and 0x0000 would transfer one byte. */
#define DMA_COUNT       (DMA_ADDRESS + 1)

/* The DMA_PAGE register gets bits 16-23 of the address.
 * Combined with the LSB and MSB sent to the address register,
 * This allows for a 16Meg addressing range.
 * Note that the Page registers are not in any logical order, so you
 * just have to pick the right one out of some table. */
#ifndef SB_DMA_CHAN
#  error "You must define SB_DMA_CHAN to be 0, 1, or 3!"
#endif

#if SB_DMA_CHAN == 0
#  define	DMA_PAGE	0x87
#elif SB_DMA_CHAN == 1
#  define	DMA_PAGE	0x83
#elif SB_DMA_CHAN == 2
#  define	DMA_PAGE	0x81 /* Not available to SB! */
#elif SB_DMA_CHAN == 3
#  define	DMA_PAGE	0x82
#else
#  error "You must define SB_DMA_CHAN to be 0, 1, or 3!"
#endif

#ifdef SBPRO
#  define EXPECTED_DSP_VER	3
#endif
#ifdef SB20
#  define EXPECTED_DSP_VER	2
#endif
#ifdef SB10
#  define EXPECTED_DSP_VER	1
#endif

#define	SB_NPORT	16		/* 16 I/O ports used */

  
/*  ---- Sound Blaster DSP values ---
 *
 * These are the card addresses where data may be read or written.
 * status.addr is the base address of the card.
 * The typical settings are 0x220 or 0x240  (0x220 is the default)
 * See the docs for an explanation of each register.
 */
#define DSP_RESET   		(status.addr + 0x06) /* Pulse to reset DSP */
#define DSP_RDDATA  		(status.addr + 0x0A) /* Read data */
#define DSP_WRDATA  		(status.addr + 0x0C) /* Write data */
#define DSP_COMMAND 		(status.addr + 0x0C) /* Send command */
#define DSP_STATUS  		(status.addr + 0x0C) /* Read status */
#define DSP_RDAVAIL 		(status.addr + 0x0E) /* Data available */

/* Status bits:
 * These are bits within the named readable registers that mean something. */
#define DSP_BUSY		(1 << 7) /* STATUS flag indicates not ready */
#define DSP_DATA_AVAIL		(1 << 7) /* RDAVAIL flag for data ready */
#define DSP_READY		0xAA     /* RDDATA generated by a good reset */

/* DSP commands:
 * These are the possible commands to send to the DSP via the DSP_COMMAND
 * register.  See the docs for more detailed explanations. */
#define SET_TIME_CONSTANT	0x40 /* Set the sampling rate (1 byte) */
#define SPEAKER_ON		0xD1 /* Turn on DSP sound */
#define SPEAKER_OFF		0xD3 /* Mute DSP voice output */
#define READ_SPEAKER		0xD8 /* Read speaker status (00:OFF FF:ON) */
#define HALT_DMA		0xD0 /* Pause the DMA transfer */
#define CONT_DMA		0xD4 /* Continue paused DMA transfer */
#define GET_DSP_VERSION		0xE1 /* Get the DSP major & minor versions */

#define TRANSFER_SIZE		0x48 /* Set the HS block size. (LSB & MSB) */
#define HS_DAC_8		0x91 /* Start High-Speed 8-bit DMA DAC */
#define HS_ADC_8		0x99 /* Start High-Speed 8-bit DMA ADC */
#define AUTO_HS_DAC_8		0x90 /* Start High-Speed auto-repeat DAC */
#define AUTO_HS_ADC_8		0x98 /* Start High-Speed auto-repeat ADC */

#define SILENCE			0x80 /* Send a block of silence */
#define DIRECT_DAC_8		0x10 /* For sending a single sample byte */
#define DIRECT_ADC_8		0x20 /* For sampling a single byte */

/* Commands to start Low-Speed DMA transfers.
   For the decompression modes, add one to get the command for
   data with reference byte (first block of sample) */
#define ADC_8			0x24 /* Start 8-bit DMA ADC */
#define DAC_8			0x14 /* Start 8-bit DMA DAC */
#define DAC_4			0x74 /* Start 4-bit DMA DAC */
#define DAC_3			0x76 /* Start 2.6-bit DMA DAC */
#define DAC_2			0x16 /* Start 2-bit DMA DAC */

#define DMA_SUSPEND		0x04 /* ??? */
#define AUTO_DAC_2BIT 		0x1e
#define AUTO_DAC_3BIT 		0x7e
#define AUTO_DAC_4BIT 		0x7c
#define AUTO_DAC_8BIT		0x1c /* Start low-speed auto-repeat DAC */
#define AUTO_ADC_8BIT		0x2c /* Start low-speed auto-repeat ADC */

/* MIDI DSP commands. */
#define MIDI_POLL		0x30 /* Poll a byte from the MIDI port */
#define MIDI_READ		0x31 /* Initiate a MIDI read with interrupts */
#define MIDI_UART_POLL		0x34 /* Poll for a byte while allowing write */
#define MIDI_UART_READ		0x35 /* Initiate a read while allowing write */
#define MIDI_WRITE		0x38 /* Write a single MIDI byte */ 


/*
 * Sound Blaster on-board Mixer chip addresses:
 */
#define MIXER_ADDR		(status.addr + 0x04)
#define MIXER_DATA		(status.addr + 0x05)

/* To reset the Mixer to power-up state, send a 0x00 to both the
 * ADDR and DATA ports.  (Actually, you can send any value to DATA after
 * sending 0x00 to the ADDR port.) */
#define MIXER_RESET		0x00 /* Send this to both ADDR & DATA */

/*
 * Mixer chip registers:
 */
#define RECORD_SRC      0x0C
/*  bit:  7 6 5 4 3 2 1 0           F=frequency (0=low, 1=high)
          x x T x F S S x           SS=source (00=MIC, 01=CD, 11=LINE)
	                            T=input filter switch (ANFI) */

/* Recording sources (SRC_MIC, SRC_CD, SRC_LINE)
   are defined in i386at/sblast.h */

#define IN_FILTER       0x0C	/* "ANFI" analog input filter reg */
#define OUT_FILTER      0x0E	/* "DNFI" output filter reg */
#define FREQ_HI         (1 << 3) /* Use High-frequency ANFI filters */
#define FREQ_LOW        0	/* Use Low-frequency ANFI filters */
#define FILT_ON         0	/* Yes, 0 to turn it on, 1 for off */
#define FILT_OFF        (1 << 5)

#define CHANNELS	0x0E	/* Stereo/Mono output select */
#define MONO_DAC	0	/* Send to OUT_FILTER for mono output */
#define STEREO_DAC	2	/* Send to OUT_FILTER for stereo output */

#define VOL_MASTER      0x22	/* High nibble is left, low is right */
#define VOL_FM          0x26
#define VOL_CD          0x28
#define VOL_LINE        0x2E
#define VOL_VOC         0x04
#define VOL_MIC         0x0A	/* Only the lowest three bits. (0-7) */


/*
 * Soundblaster FM sythesizer definitions
 */

/* FM synthesizer chip addresses for both, left, and right channels. */
#define FM_ADDR_B		0x388 /* Adlib addresses */
#define FM_DATA_B		0x389 /* Could also use addr + 0x08 & 0x09 */

#ifdef SBPRO
/* The Sound Blaster Pro has stereo FM channels */
#define FM_ADDR_L		(status.addr + 0x00) /* Left only */
#define FM_DATA_L		(status.addr + 0x01)
#define FM_ADDR_R		(status.addr + 0x02) /* Right only */
#define FM_DATA_R		(status.addr + 0x03)
#else
/* Original Sound Blaster cards do not have stereo */
#define FM_ADDR_L		FM_ADDR_B
#define FM_DATA_L		FM_DATA_B
#define FM_ADDR_R		FM_ADDR_B
#define FM_DATA_R		FM_DATA_B
#endif

#define FM_STATUS_L		FM_ADDR_L /* Status Read Register (left) */
#define FM_STATUS_R		FM_ADDR_R /* Status Read Register (right) */
/* Bit 7: IRQ flag.  FM synth has raised an interrupt.
 * Bit 6: Timer 1 overflow flag.  Set when Timer1 expires.
 * Bit 5: Timer 2 overflow flag.  Set when Timer2 expires. */

/*
 * Offset from register for a given voice (0 to 8) and operator cell (0 or 1)
 * (see also const char fm_op_order[] in sb_driver.c)
 */
#define FM_OFFSET(voice, op)		(fm_op_order[voice] + ((op) ? 3 : 0))
extern const char fm_op_order[];
/* = { 0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12 }; */

/*
 * Parameters applied to all voices
 */
#define WAVE_CTL	0x01
/* Bit 5 set allows FM chips to "control the waveform" of each operator. */

#define CSM_KEYSPL	0x08
/* Bit 7: Set for composite sine-wave speech mode.
          All KEY-ON bits must be clear first.
   Bit 6: Selects keyboard split point (with F-Number data??)  */

#define DEPTHS	     	0xbd
/* Bit 7: 1 - AM depth is 4.8 dB
          0 - AM depth is 1 dB
   Bit 6: 1 - Vibrato depth is 14 cent
          0 - Vibrato depth is 7 cent
   Bit 5: 1 - Rhythm enabled  (w/ 6 music voices)
          0 - Rhtthm disabled (w/ 9 music voices)
   Bit 4: Bass drum on/off
   Bit 3: Snare drum on/off
   Bit 2: Tom-tom on/off
   Bit 1: Cymbal on/off
   Bit 0: Hi-hat on/off

   Note: KEY-ON bits for voices 0x06, 0x07, and 0x08 must be
   OFF to use rhythm  */

/*
 * Parameters applied to individual voices
 */
#define WAVEFORM(voice, op)	(0xe0 + FM_OFFSET((voice), (op)))
/* When Bit 5 of address 0x01 is set, output will be distorted
   according to Bits 0-1 of this register.
   00 - Normal sine wave
   01 - Sine with all negative values removed
   10 - abs(Sine wave)  i.e. two positive half-cycles
   11 - sortof sawtooth.  (like 10, with second half of each hum removed) */

#define MODULATION(voice, op)	(0x20 + FM_OFFSET((voice), (op)))
/* Bit 7: Set for amplitude modulation (depth controlled by DEPTH)
   Bit 6: Set for vibrato (depth controlled by DEPTH)
   Bit 5: Set to have a Sustain in the envelope.  Clear for decay
          immediately after hitting Sustain phase.
   Bit 4: Keyboard scaling rate. ???
   Bit 3-0: Which harmonic of the specified frequency to generate.
            0: -1 8ve
            1: at specified freq.
	    2: +1 8ve
	    3: +1 8ve and a fifth
	    4: +2 8ve
	    etc.. */

#define LEVEL(voice, op)	(0x40 + FM_OFFSET ((voice), (op)))
/* Bits 7-6: Output decrease as freq rises.
             00 - none
	     10 - 1.5 dB/8ve
	     01 - 3 dB/8ve
	     11 - 6 dB/8ve
   Bits 5-0: Volume attenuation
             All bits 0 is the LOUDEST, all bits 1 for softest.
	     (Attenuation = value * .75 dB) */

#define ENV_AD(voice, op)	(0x60 + FM_OFFSET ((voice), (op)))
/* Bits 7-4: Attack rate  (0 slowest, F fastest)
   Bits 3-0: Decay rate   (0 slowest, F fastest) */

#define ENV_SR(voice, op)	(0x80 + FM_OFFSET ((voice), (op)))
/* Bits 7-4: Sustain level (0 loudest, F softest)
   Bits 3-0: Release rate  (0 slowest, F fastest) */

#define FNUM_LO(voice)		(0xa0 + (voice))
/* Least significant byte of frequency control.
   8 bits of the 10-bit F-Number.
   Freq = 50000 * FNUM * 2^(octave - 20) */

#define OCT_SET(voice)		(0xb0 + (voice))
/* Bit 5:    Channel on when set, silent when clear.
   Bits 4-2: Octave (0 lowest, 7 highest)
   Bits 1-0: Most significant bytes of frequency (FNUM_HI)*/

#define FEED_ALG(voice)	(0xc0 + (voice))
/* Bits 3-1: Feedback strength.  0 = none, 7 = max
   Bit 0:    If Set, both operators output directly.
             If Clear, operator 1 modulates operator 2.  */

/*
 * Sound Blaster dual-Timer controls
 */
#define TIMER1		0x02
/* If Timer1 is enabled, this register will ++ every 80 uSec until
   it overflows.  Upon overflow, card will signal a Timer interrupt (INT 08)
   and set bits 7 and 6 in status byte.  */

#define TIMER2		0x03
/* Second timer same as Timer1 except it sets status bits 7 and 5
   on overflow, and increments every 320 uSec.  */

#define TIMER_CTL	0x04
/* Bit 7: Resets flags for Timer 1 & 2.  (All other bits ignored)
   Bit 6: Masks Timer1.  (Bit 0 is ignored)
   Bit 5: Masks Timer2.  (Bit 1 is ignored)
   Bit 1: When clear, Timer2 not running.
          When set, value from TIMER2 is loaded into Timer2 and incr begins.
   Bit 0: Same as Bit 1, but for Timer1.  */

/* Timer control bits as explained in TIMER_CTL */
#define FM_TIMER_RESET		(1 << 7)
#define FM_TIMER_MASK1		(1 << 6)
#define FM_TIMER_MASK2		(1 << 5)
#define FM_TIMER_START2  	(1 << 1)
#define FM_TIMER_START1  	(1 << 0)
