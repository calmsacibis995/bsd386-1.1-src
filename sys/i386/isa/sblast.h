/*	BSDI $Id: sblast.h,v 1.2 1994/01/30 00:59:04 karels Exp $	*/

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

#ifndef SBLAST_H
#define SBLAST_H
#include <sys/ioctl.h>

/*
 * Here is where you select which card you have.
 * Define only one of these.
 *
 * NOTE: Version 1.5 supports ONLY the SBPRO selection.
 *       Feel free to add support for the rest. :-) 
 */

/* Define this if you have a Sound Blaster Pro  (DSP v3.x) */
#define SBPRO
/* Define this if you have a Sound Blaster with a DSP version 2.x */
/* #define SB20 */
/* Define this if you have an older Sound Blaster (DSP v1.x) */
/* #define SB10 */

typedef unsigned char sbBYTE;
typedef unsigned char sbFLAG;

/*
 * Available compression modes:
 * (These are actually DSP DMA-start commands, but you don't care)
 */
enum
{
  PCM_8 	= 0x14,		/* Straight 8-bit PCM */
  ADPCM_4 	= 0x74,		/* 4-bit ADPCM compression */
  ADPCM_3	= 0x76,		/* 2.5-bit ADPCM compression */
  ADPCM_2 	= 0x16,		/* 2-bit ADPCM compression */
};

enum { FM_LEFT, FM_RIGHT, FM_BOTH };	/* Stereo channel choices */



/*   IOCTLs for FM, Mixer, and DSP control.   */

/*
 * FM: Reset chips to silence
 *     Play a note as defined by struct sb_fm_note
 *     Set global FM parameters as defined by struct sb_fm_params
 */
#define FM_IOCTL_RESET      	_IO('s', 10)
#define FM_IOCTL_PLAY_NOTE  	_IOW('s', 11, struct sb_fm_note)
#define FM_IOCTL_SET_PARAMS 	_IOW('s', 13, struct sb_fm_params)

/* Mixer: Set mixer volume levels
 *        Set mixer control parameters (non-volume stuff)
 *	  Read current mixer volume levels
 *	  Read current mixer parameters
 *	  Reset the mixer to a default state
 */	  
#define MIXER_IOCTL_SET_LEVELS 	_IOW('s', 20, struct sb_mixer_levels)
#define MIXER_IOCTL_SET_PARAMS 	_IOW('s', 21, struct sb_mixer_params)
#define MIXER_IOCTL_READ_LEVELS	_IOR('s', 22, struct sb_mixer_levels)
#define MIXER_IOCTL_READ_PARAMS _IOR('s', 23, struct sb_mixer_params)
#define MIXER_IOCTL_RESET 	_IO('s', 24)

/* DSP: Reset the DSP and terminate any transfers.
 *      Set the speed (in Hz) of DSP playback/record.
 *      (Note that the speed parameter is modified to be the actual speed.)
 *      Turn the DSP voice output on (non-zero) or off (0)
 *      Flush any pending written data.
 *      Set the DSP decompression mode to one of the above modes.
 *      Set Stereo playback/record on (non-zero) or off (0)
 *	Set the DMA transfer buffer size.
 *	Errno generated when there is recording overrun.  (ESUCCESS for ignore)
 */
#define DSP_IOCTL_RESET 	_IO('s', 0)
#define DSP_IOCTL_SPEED 	_IOWR('s', 1, int)
#define DSP_IOCTL_VOICE 	_IOW('s', 2, sbFLAG)
#define DSP_IOCTL_FLUSH 	_IO('s', 3)
#define DSP_IOCTL_COMPRESS	_IOW('s', 4, sbBYTE)
#define DSP_IOCTL_STEREO	_IOW('s', 5, sbFLAG) /* Can go to mixer too */
#define DSP_IOCTL_BUFSIZE	_IOWR('s', 6, int)
#define DSP_IOCTL_OVERRUN_ERRNO	_IOW('s', 7, int)

/*
 * DSP legal speed range:
 * For cards other than the SBPRO, there are separate limits
 * on recording and playback speed.  Future driver versions will
 * address this.  Also, compression effects the valid range.  Whew..
 */
# define DSP_MIN_SPEED	3906
#ifdef SBPRO
#  define DSP_MAX_SPEED	47619
#endif
#ifdef SB20
#  define DSP_MAX_SPEED 43478
#endif
#ifdef SB10
#  define DSP_MAX_SPEED 22222
#endif

/* The smallest and largest allowable DSP transfer buffer size.
   Note that the size must also be divisible by two */
#define DSP_MIN_BUFSIZE 256
#define DSP_MAX_BUFSIZE (64 * 1024)

struct stereo_vol
{
  sbBYTE l;			/* Left volume */
  sbBYTE r;			/* Right volume */
};


/*
 * Mixer volume levels for MIXER_IOCTL_SET_VOL & MIXER_IOCTL_READ_VOL
 */
struct sb_mixer_levels
{
  struct stereo_vol master;	/* Master volume */
  struct stereo_vol voc;	/* DSP Voice volume */
  struct stereo_vol fm;		/* FM volume */
  struct stereo_vol line;	/* Line-in volume */
  struct stereo_vol cd;		/* CD audio */
  sbBYTE mic;			/* Microphone level */
};

/*
 * Mixer parameters for MIXER_IOCTL_SET_PARAMS & MIXER_IOCTL_READ_PARAMS
 */
struct sb_mixer_params
{
  sbBYTE record_source;		/* Recording source (See SRC_xxx below) */
  sbFLAG hifreq_filter;		/* Filter frequency (hi/low) */
  sbFLAG filter_input;		/* ANFI input filter */
  sbFLAG filter_output;		/* DNFI output filter */
  sbFLAG dsp_stereo;		/* 1 if DSP is in Stereo mode */
};
#define SRC_MIC         1	/* Select Microphone recording source */
#define SRC_CD          3	/* Select CD recording source */
#define SRC_LINE        7	/* Use Line-in for recording source */


/*
 * Data structure composing an FM "note" or sound event.
 */
struct sb_fm_note
{
  sbBYTE channel;			/* LEFT, RIGHT, or BOTH */
  sbBYTE operator;		/* Operator cell (0 or 1) */
  sbBYTE voice;			/* FM voice (0 to 8) */

  /* Per operator: */
  sbBYTE waveform;		/* 2 bits: Select wave shape (see WAVEFORM) */
  sbFLAG am;			/* Amplitude modulation */
  sbFLAG vibrato;			/* Vibrato effect */
  sbFLAG do_sustain;		/* Do sustain phase, or skip it */
  sbFLAG kbd_scale;		/* Keyboard scaling? */

  sbBYTE harmonic;		/* 4 bits: Which harmonic to generate */
  sbBYTE scale_level;		/* 2 bits: Decrease output as freq rises*/
  sbBYTE volume;			/* 6 bits: Intuitive volume */

  sbBYTE attack;			/* 4 bits: Attack rate */
  sbBYTE decay;			/* 4 bits: Decay rate */
  sbBYTE sustain;			/* 4 bits: Sustain level */
  sbBYTE release;			/* 4 bits: Release rate */

  /* Apply to both operators of one voice: */
  sbBYTE feedback;		/* 3 bits: How much feedback for op0 */
  sbFLAG key_on;			/* Set for active, Clear for silent */
  sbFLAG indep_op;		/* Clear for op0 to modulate op1,
				   Set for parallel tones. */

  /* Freq = 50000 * Fnumber * 2^(octave - 20) */
  sbBYTE octave;			/* 3 bits: What octave to play (0 = low) */
  unsigned int fnum;		/* 10 bits: Frequency "number" */
};

/*
 * FM parameters that apply globally to all voices, and thus are not "notes"
 */
struct sb_fm_params
{
  sbBYTE channel;			/* LEFT, RIGHT, or BOTH, as defined above */

  sbFLAG am_depth;		/* Amplitude Modulation depth (1=hi) */
  sbFLAG vib_depth;		/* Vibrato depth (1=hi) */
  sbFLAG wave_ctl;		/* Let voices select their waveform */
  sbFLAG speech;			/* Composite "Speech" mode (?) */
  sbFLAG kbd_split;		/* Keyboard split */
  sbFLAG rhythm;			/* Percussion mode select */

  /* Percussion instruments */
  sbFLAG bass;
  sbFLAG snare;
  sbFLAG tomtom;
  sbFLAG cymbal;
  sbFLAG hihat;
};

#endif				/* !def SBLAST_H */
