#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "/usr/src/sys/i386/isa/pcconsioctl.h"
#include "kernel.h"

/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
extern int v86_proc;
int monitor_initialized = 0;
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/

void init_services(void)
{
}

void system_service(struct trapframe *tf)
{
  switch (tf->tf_ax & 0xff) {
  case 0x00:       /* then goes to case 0x13 !!!!!!! */
    init_disk_service();
  case 0x13:
    (void)v86_do_int13();
    break;
  case 0x10:       /* temp.: remote terminal service in future */
#ifdef DEBUG
    ErrorTo86 ("Obsolete CGA video call");
#endif
    break;
  case 0x17:
    PrinterService ((u_char)(tf->tf_ax >> 8), tf->tf_bx);
    break;
  case 0x18:
    ErrorTo86 ("ERROR: ROM BASIC not present and boot fails");
    break;
  case 0x40:
    ioctl (0, PCCONIOCBEEP);
    break;
  case 0x64:
    xms_service (tf);
    break;
/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
  case 0x6D:
  case 0xF0:
    v86_proc = 3;
    break;
  case 0xF1:
    if (! monitor_initialized) {
      InitVideo(tf);
      monitor_initialized = 1;
    }
    break;
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/
  default:
    ErrExit (0x09, "Exit caused by DOS' program\n");
  }
}
