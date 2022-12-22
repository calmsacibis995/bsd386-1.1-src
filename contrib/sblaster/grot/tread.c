#include <sys/file.h>
#include <i386at/sb_defs.h>
  
void main (void)
{
  int fd, res;
  struct sb_mixer_levels *l;
  struct sb_mixer_params *p;
  char buf[1000];

  fd = open ("/dev/sb_mixer", O_RDWR);
  if (fd < 0)
    {
      perror ("open");
      exit(1);
    }
 
  res = read(fd, buf, 1000);
  if (res < 0)
    {
      perror ("read < 0");
    }
  else
    {
      printf ("res = %d\n", res);
      printf ("result: %s\n", buf);
    }

  p = buf;
  l = buf + sizeof (struct sb_mixer_params);

  printf ("Params:\n"
	  "rec src: %d\n"
	  "hifreq: %d\n"
	  "filt in: %d\n"
	  "filt out: %d\n",
	  p->record_source,
	  p->hifreq_filter,
	  p->filter_input,
	  p->filter_output);

  printf ("Levels:\n"
	  "mast L: %d\n"
	  "mast R: %d\n"
	  "voc  L: %d\n"
	  "voc  R: %d\n"
	  "fm L: %d\n"
	  "fm R: %d\n"
	  "line L: %d\n"
	  "line R: %d\n"
	  "cd: %d\n"
	  "mic: %d\n",
	  l->master_l, l->master_r,
	  l->voc_l, l->voc_r,
	  l->fm_l, l->fm_r,
	  l->line_l, l->line_r,
          l->cd, l->mic);
  
  lseek (fd, 0, L_SET);
  
  res = write(fd, buf, 1000);
  if (res < 0)
    {
      perror ("write < 0");
    }
  else
    {
      printf ("res = %d\n", res);
    }
}
