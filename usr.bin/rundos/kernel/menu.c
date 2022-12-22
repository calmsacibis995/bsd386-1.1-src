#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include "kernel.h"

extern FILE *logfile;

#ifdef DEBUG
struct sigframe {
int	sf_signum;
int	sf_code;
struct sigcontext *sf_scp;
sig_t	sf_handler;
int	sf_eax;	
int	sf_edx;	
int	sf_ecx;	
struct sigcontext sf_sc;
};
#endif

#ifndef BYTE
typedef unsigned char byte;
typedef unsigned short word;
#define BYS (byte *)
#endif
#define BYTE

#define MAKE_DD_ADDR(dd_adr)  BYS((dd_adr & 0xFFFF) + ((dd_adr >> 12) & 0xFFFF0))

/*********************************************************/

extern void sigvm86_handler ();

extern struct trapframe tf_forV86;

extern struct trapframe *cur_trapframe;
extern struct connect_area *connect_area;
extern int supercli;
extern word columns;

static struct trapframe save_trapframe;
static byte save_curr_seg0[0x400];
static byte save_curr_seg40[0x200];
static char stck[0x8000];

int v86_proc = 0;

void toV86_msgs (word ax_for86);

/*
*  386->vm86 transmission loses registers. We need to save it, but there is
*  no a way to do it via GCC. Therefore we need to write this crazy function.
*/
static void raise_sigbus()
{
  __asm__ volatile ("pushal");
  __asm__ volatile ("pushl $10");	/* SIGBUS signal number */
  __asm__ volatile ("call _raise");
  __asm__ volatile ("popl %eax");
  __asm__ volatile ("popal");
}

void ErrExit (int exit_status, char *msg, int x)
{
  FILE *f;
  
  reset_console ();

  if (! logfile)
    f = stderr;
  else
    f = logfile;
  if (exit_status != 0)
    fprintf (f, "Error: ");
  fprintf (f, msg, x);

  exit(exit_status);
}

/*********************************************************/

void MenuEnter (int errcod, char * msg)
{
  int user_answ;

#ifdef DEBUG  
  dprintf("***************** MENU ENTER *****************\n");
#endif

  select_cook_keyboard();
  vga_save_current_mode();

  if (errcod)
    strcpy ((char *)MAKE_DD_ADDR(connect_area->error_text), msg);

  toV86_msgs (errcod);   /* action: menu only or error */

#ifdef DEBUG  
  dprintf("***************** INSIDE MENU *****************\n");
#endif

  while ( (user_answ = getchar()) != 0x1B  &&  user_answ != 0x0D ) {
    switch ( user_answ & 0x5F ) {
    case 'Q':
      ErrExit (0, "Normal exit ...\n", 0);  /* dummy second 0 */
    case 'S':
      vga_restore_initial_mode();
      select_cook_console();
      normal_console();
      munmap((caddr_t)0xa0000, 0x50000);

#ifdef DEBUG  
      dprintf("------ before SIGTSTP\n");
#endif

      raise(SIGTSTP);
      /* Time passed... */

#ifdef DEBUG  
      dprintf("------ after SIGTSTP\n");
#endif

      map_device("/dev/vga", 0xa0000, 0x50000);
      vga_save_initial_mode();
      redirect_console();
      select_raw_console();
      goto Menu_exit;
    case 'R':
      cur_trapframe->tf_ip = 0x0000;
      cur_trapframe->tf_cs = 0xFFFF;
      cur_trapframe->tf_sp = 0xF000;
      cur_trapframe->tf_ss = 0x0000;
      cur_trapframe->tf_eflags &= ~(PSL_T | PSL_D);
      INT_STATE = 0;
      xms_reset ();
      goto Menu_exit_1;
    case 'P':
      PrintBufFlash ();
      break;
    case 'T':
      while ( (user_answ = getchar()) == EOF  ||
	     user_answ < 0x30  ||  user_answ > 0x31 )
	;
      change_tick_mode (user_answ - 0x30);
      goto Menu_exit;
      break;
    case 'M':
      change_sound_mode ();
      goto Menu_exit;
      break;
    default:
      break;
    }
  }
 Menu_exit:
  vga_restore_current_mode();
 Menu_exit_1:
  init_tick ();
  select_raw_keyboard();

#ifdef DEBUG  
  dprintf("***************** MENU EXIT *****************\n");
#endif

}

/****************************************************************/

void ErrorTo86 (char *msg)
{
  if (! supercli)
    MenuEnter (1, msg);
  else
    ErrExit (0x10, "Unable to display the menu !\n", 0);
} 

/***************************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */

void toV86 (void)
{
  struct sigaction sa;
  struct sigaction old_bus_sa;
  struct sigaction old_ill_sa;
  struct sigstack sst, oss;
  sigset_t all_block = (sigset_t) -1;
  sigset_t old_signal_state;

  v86_proc = 1;
  supercli = 1;

  all_block &= ~(sigmask(SIGILL) | sigmask(SIGBUS));
  sigprocmask (SIG_SETMASK, &all_block, &old_signal_state);

  sst.ss_sp = stck + 0x7000;
  sst.ss_onstack = 0;
  sigstack (&sst, &oss);

#ifdef DEBUG  
  dprintf("oss = %X %X\n", oss.ss_sp, oss.ss_onstack);
#endif

  sa.sa_handler = sigvm86_handler;
  sa.sa_mask = all_block;
  sa.sa_flags = SA_ONSTACK;
  sigaction (SIGBUS, &sa, &old_bus_sa);
  sa.sa_handler = sigvm86_handler;
  sa.sa_mask = all_block;
  sa.sa_flags = SA_ONSTACK;
  sigaction (SIGILL, &sa, &old_ill_sa);

#ifdef DEBUG  
  dprintf("------ before raising, sp = %8.8X\n", CurrSP());
#endif

  raise_sigbus();

#ifdef DEBUG  
  dprintf("------ after raising, sp = %8.8X\n", CurrSP());
#endif

#ifdef DEBUG  
  dprintf("oss = %X %X\n", oss.ss_sp, oss.ss_onstack);
#endif

  sigstack (&oss, NULL);
  sigaction (SIGBUS, &old_bus_sa, NULL);
  sigaction (SIGILL, &old_ill_sa, NULL);
  sigprocmask (SIG_SETMASK, &old_signal_state, NULL);
  v86_proc = 0;
  supercli = 0;
}

void toV86_msgs (word ax_for86)
{
  memcpy (save_curr_seg0, (char *)0x00, 0x400);
  memcpy (save_curr_seg40, (char *)0x400, 0x200);
  memcpy (BYS 0x00, MAKE_DD_ADDR(connect_area->pattern_of_seg0),
	  0x43 * 4);   /* up to 43th vector */
  memcpy (BYS (0x44*4), MAKE_DD_ADDR(connect_area->pattern_of_seg0) + 0x44*4,
	  0x400 - 0x44*4); /* starting from 44th vector */
   /* for future: don't copy video data area !!! */

  tf_forV86.tf_ip = (word)connect_area->menu_V86_addr;
  tf_forV86.tf_cs = (word)(connect_area->menu_V86_addr >> 16);
  tf_forV86.tf_sp = (word)connect_area->menu_V86_stack;
  tf_forV86.tf_ss = (word)(connect_area->menu_V86_stack >> 16);
  tf_forV86.tf_ax = ax_for86;   /* action: menu only or error */

  toV86 ();

  memcpy ((char *)0x00, save_curr_seg0, 0x400);
  memcpy ((char *)0x400, save_curr_seg40, 0x200);
}

