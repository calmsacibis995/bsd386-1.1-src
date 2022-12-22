#include <i386at/sblast.h>
#include <sys/file.h>
#include <stdio.h>

#define RAND(bits)		(random() & ((1 << (bits)) - 1))

#define NUM_NOTES 10
void main (void)
{
  struct sb_fm_note n;
  struct sb_fm_params p;
  int delay;
  int i;
  int fmdev;

  fmdev = open ("/dev/sb_fm", O_WRONLY, 0);
  if (fmdev < 0)
    perror ("openning fmdev");
  
/*  if (ioctl (fmdev, FM_IOCTL_RESET, 0) == -1)
    perror ("reset"); */
  
  srandom (time(0));

  p.am_depth = 0;
  p.vib_depth = 0;
  p.rhythm = 0;
  p.bass = 0;
  p.snare = 0;
  p.tomtom = 0;
  p.cymbal = 0;
  p.hihat = 0;
  p.kbd_split = 0;
  p.wave_ctl = 0;
  p.speech = 0;
  p.channel = BOTH;
      
/*  if (ioctl (fmdev, FM_IOCTL_SET_PARAMS, &p) < 0)
    perror ("set_params");  */

  for (i = 0; i < NUM_NOTES; i++)
    {
      n.channel = BOTH;
      n.operator = 0;
      n.am = 0;
      n.vibrato = 0;
      n.do_sustain = 0;
      n.kbd_scale = 0;
      n.harmonic = 2;
      n.scale_level = 0;
      n.attack = 0x0F;
      n.decay = 0x0A;
      n.sustain = 0x01;
      n.release = 0x05;
      n.fnum = 5000;
      n.octave = 3;
      n.feedback = 2;
      n.indep_op = 1;
      n.waveform = 0;
/*      n.volume = 0x3f;*/
      n.volume = 0x3f;

      n.key_on = 1;
      if (ioctl (fmdev, FM_IOCTL_PLAY_NOTE, &n) < 0)
	perror ("play_note");

      sleep (1);

      n.key_on = 0;
      if (ioctl (fmdev, FM_IOCTL_PLAY_NOTE, &n) < 0)
	perror ("play_note");
    }
  exit (0);
}

