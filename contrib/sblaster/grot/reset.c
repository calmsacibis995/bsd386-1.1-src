#include <i386at/sblast.h>
#include <sys/file.h>

void main (int argc, char **argv)
{
  int fd;

  fd = open ("/dev/sb_mixer", O_RDONLY, 0);
  if (fd < 0)
    perror ("open");

  if (ioctl (fd, DSP_IOCTL_RESET, 0) < 0)
    perror ("ioctl");
  exit (0);
}
