/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: doscmd_loader.c,v 1.1 1994/01/14 23:28:03 polk Exp $ */
#include <stdio.h>
#include <a.out.h>

#define _PATH_DOS_KERNEL_DIR	"/usr/libexec/"
#define _PATH_DOS_KERNEL	"doscmd.kernel"

int load_kernel(void)
{
  FILE *fp;
  struct exec exec;
  int start_address;

  if ((fp = fopen(_PATH_DOS_KERNEL, "r")) == NULL &&
      (fp = fopen("../" _PATH_DOS_KERNEL, "r")) == NULL &&
      (fp = fopen("obj/" _PATH_DOS_KERNEL, "r")) == NULL &&
      (fp = fopen(_PATH_DOS_KERNEL_DIR _PATH_DOS_KERNEL, "r")) == NULL) {
    perror("load_kernel");
    exit(1);
  }
  if (fread(&exec, sizeof(exec), 1, fp) != 1 || exec.a_magic != OMAGIC) {
    fprintf(stderr, "bad kernel file format\n");
    exit(1);
  }
  start_address = exec.a_entry & (~(getpagesize() - 1));
  if (brk(start_address + exec.a_text + exec.a_data + exec.a_bss) < 0) {
    perror("load_kernel");
    exit(1);
  }
  fread((char *)start_address, exec.a_text + exec.a_data, 1, fp);
  bzero((char *)(start_address + exec.a_text + exec.a_data), exec.a_bss);
  fclose(fp);
  return(exec.a_entry);
}

void main(int argc, char **argv, char **environ)
{
  void (*entry_point)() = (void (*)()) load_kernel();

  (*entry_point)(argc, argv, environ);
  fprintf(stderr, "return from kernel???\n");
  exit(1);
}
