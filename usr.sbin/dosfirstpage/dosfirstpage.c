/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: dosfirstpage.c,v 1.2 1992/09/01 16:03:05 polk Exp $
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>

#define VIDEO_BASE 0xa0000
#define VIDEO_LEN  0x20000

#define FIRSTPAGEFILE "/var/run/dos.firstpage"

void main(void)
{
  int dev_vga;
  FILE *fp;

  if ((dev_vga = open("/dev/vga", 2)) == -1) {
    fprintf(stderr, "dosfirstpage: can't open /dev/vga\n");
    exit(1);
  }
  if ((fp = fopen(FIRSTPAGEFILE, "w")) == NULL) {
    fprintf(stderr, "dosfirstpage: can't create %s\n", FIRSTPAGEFILE);
    exit(1);
  }
  if (mmap((char *)VIDEO_BASE, VIDEO_LEN,
		 PROT_READ|PROT_WRITE,
		 MAP_FILE|MAP_FIXED, dev_vga,
		 0) != (char *)VIDEO_BASE) {
    fprintf(stderr, "dosfirstpage: can't map video memory\n");
    exit(1);
  }
  fwrite((char *)(0xb8000+(4*1024)), 8 * 1024, 1, fp);
  munmap((char *)VIDEO_BASE, VIDEO_LEN);
  fclose(fp);
  close(dev_vga);
  exit(0);
}
