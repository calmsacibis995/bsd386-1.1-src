#include <sys/ioctl.h>
#include <sys/file.h>
#include <i386at/sblast.h>

void main (void)
{
  struct sb_mixer_levels l;
  int fd;

  fd = open ("/dev/sb_mixer", O_RDWR);
  
  l.master_l = 15;
  l.master_r = 15;
  l.line_l = 15;
  l.line_l = 15;
  l.voc_l = 15;
  l.voc_r = 15;
  l.fm_l = 0;
  l.fm_r = 0;
  l.cd = 0;
  l.mic = 0;

  if (ioctl (fd, MIXER_IOCTL_SET_VOL, &l) == -1)
    perror ("ioctl");
}
