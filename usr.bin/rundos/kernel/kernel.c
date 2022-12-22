#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <machine/psl.h>
#define KERNEL
#include <machine/trap.h>
#include <machine/npx.h>
#include "kernel.h"
#include "paths.h"

/*
 * From machdep.c
 */
struct sigframe {
  int	sf_signum;
  int	sf_code;
  struct sigcontext *sf_scp;
  sig_t	sf_handler;
  int	sf_eax;	
  int	sf_edx;	
  int	sf_ecx;	
  struct save87 sf_fpu;
  struct sigcontext sf_sc;
};

/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
struct trapframe *cur_trapframe;

extern int v86_proc;
struct trapframe tf_forV86;
static struct sigframe save_sigframe;
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/

extern int no_save_video;

struct connect_area *connect_area;
int kernel_emulator = 1;

static sigset_t old_signal_state;

/*********************************************************/

static int read_connect_area_address(void)
{
  FILE *fp;
  struct connect_area ca;
  
  if ((fp = fopen(ROMBIOS_FILE_NAME, "r")) == NULL) {
    fprintf(stderr, "can't open %s\n", ROMBIOS_FILE_NAME);
    exit(1);
  }
  fread((char *)&ca, sizeof(struct connect_area), 1, fp);
  fclose(fp);
  return(MAKE_ADDR(0xf000, ca.rom_offset_inside_f000));
}

/*********************************************************/

static void load_image(char *bname, int addr)
{
  FILE *fp;
  struct stat stb;

#ifdef DEBUG
  dprintf("Load image %s, address 0x%x\n", bname, addr);
#endif
  if (stat(bname, &stb) == -1) {
    fprintf (stderr, "load_image: %s: %s\n", bname, strerror(errno));
    exit(1);
  }
  if ((fp = fopen(bname, "r")) == NULL) {
    fprintf(stderr, "can't open %s\n", bname);
    exit(1);
  }
  fread((char *)addr, stb.st_size, 1, fp);
  fclose(fp);
}

/*********************************************************/

void map_device(char *dname, int addr, int size)
{
  int fd;

  seteuid (0);
  if ((fd = open(dname, 2)) < 0) {
    seteuid (getuid ());
    fprintf(stderr, "can't open %s\n", dname);
    exit(1);
  }
  seteuid (getuid ());
  if (mmap((char *)addr, size, PROT_READ|PROT_WRITE,
	   MAP_FILE|MAP_FIXED|MAP_SHARED, fd, 0) != (char *)addr) {
    perror("map_device");
    exit(1);
  }
  close(fd);
}

/*********************************************************/

static void setsignal (int signum, void *handler)
{
  struct sigaction sa;
  sigset_t io_block;

  io_block = (signum == SIGBUS  ||  signum == SIGILL) ?
    (sigset_t) sigmask(SIGIO) : (sigset_t) 0;

  sa.sa_handler = handler;
  sa.sa_mask = io_block;
  sa.sa_flags = SA_ONSTACK;
  sigaction (signum, &sa, NULL);
}

/*********************************************************/

#ifdef DEBUG
static char * signames[] = {
  "sigNUL", "sigHUP", "sigINT", "sigQUIT",
  "sigILL", "sigTRAP", "sigABRT", "sigEMT",
  "sigFPE", "sigKILL", "sigBUS", "sigSEGV",
  "sigSYS", "sigPIPE", "sigALRM", "sigTERM",
  "sigURG", "sigSTOP", "sigTSTP", "sigCONT",
  "sigCHLD", "sigTTIN", "sigTTOU", "sigIO",
  "sigXCPU", "sigXFSZ", "sigVTALRM", "sigPROF",
  "sigWINCH", "sigINFO", "sigUSR1", "sigUSR2"
};

void Dump(char *msg, unsigned int *data, int len)
{
  int i;

  len = (len + 3) / 4;

  dprintf("%s\n", msg);
  for (i = 0; i < len; i++)
    dprintf("%X ", data[i]);
  dprintf("\n");
}

static void LogPrintSig (int beg_end,struct sigframe *sf, struct trapframe *tf)
{
  char * s1, * s2;
  int sg, ist;

  ist = (INT_STATE & PSL_I) ?  1 : 0;
  sg = sf->sf_signum;
  s1 = signames[sg];
  s2 = (beg_end == 0) ? "BEG" : "END";
  
  if ( (sf->sf_sc.sc_ps & PSL_VM) && (sg == SIGBUS || sg == SIGILL))
    dprintf ("===== %s %s  trap=%d, addr=%.4X:%.4X, int=%X\n",s1,s2,
	     tf->tf_trapno, tf->tf_cs, tf->tf_ip, INT_STATE);
  if ( !(sf->sf_sc.sc_ps & PSL_VM) && (sg == SIGBUS || sg == SIGILL))
    dprintf ("===== %s %s  trap=%d, addr=%X, int=%X\n", s1, s2,
	     tf->tf_trapno, sf->sf_scp->sc_pc, INT_STATE);
  if ( (sf->sf_sc.sc_ps & PSL_VM) && (sg != SIGBUS && sg != SIGILL))
    dprintf ("===== %s %s  addr=%.4X:%.4X, int=%X\n", s1, s2,
	     tf->tf_cs, tf->tf_ip, INT_STATE);
  if ( !(sf->sf_sc.sc_ps & PSL_VM) && (sg != SIGBUS && sg != SIGILL))
    dprintf ("===== %s %s  addr=%X, int=%X\n", s1, s2,
	     sf->sf_scp->sc_pc, INT_STATE);
/*
  Dump("Dump sf",(unsigned int *)sf, sizeof *sf);
  Dump("Dump tf",(unsigned int *)tf, sizeof *tf);
*/
  dprintf ("stack = %8.8X\n", CurrSP());
}
#endif

/*********************************************************/

void sigvm86_handler (struct sigframe sf, struct trapframe tf)
{
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;

/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */

  if (v86_proc == 1) {
#ifdef DEBUG
    dprintf("------ rais SIGBUS entry, sp = %8.8X, trap = %4.4X\n",
	    CurrSP(),tf.tf_trapno);
#endif
    v86_proc = 2;

    save_sigframe = sf;

    tf.tf_eflags = 0x20202;		/* IOPL 0 !!! */

    tf.tf_cs = tf_forV86.tf_cs;
    tf.tf_ip = tf_forV86.tf_ip;
    tf.tf_ss = tf_forV86.tf_ss;
    tf.tf_sp = tf_forV86.tf_sp;
    tf.tf_ax = tf_forV86.tf_ax;
    tf.tf_bx = tf_forV86.tf_bx;
    tf.tf_cx = tf_forV86.tf_cx;
    tf.tf_dx = tf_forV86.tf_dx;
    tf.tf_es = tf_forV86.tf_es;

    sf.sf_sc.sc_ps = PSL_VM;
    sf.sf_sc.sc_onstack = 0;

    /* returns from a signal to V86 */
    return;
  }
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*******************************************************/

  if ((sf.sf_sc.sc_ps & PSL_VM) == 0)
    ErrExit (0x01, "Signal %d in protected mode\n", sf.sf_signum);
  else {
    if (emulate(&tf))
      check_interrupts(&tf);
    else
      ErrExit (0x02, "Not supported instruction\n");
  }

  PushKeyboardBySig ();

/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */

#ifdef DEBUG
  if (v86_proc)
    LogPrintSig (1,&sf,&tf);
#endif

  if (v86_proc == 3) {

    tf_forV86 = tf;  /* returned values */

    v86_proc = 4;
    sf = save_sigframe;
    sf.sf_scp = &sf.sf_sc;

    /* returns to protected mode -- to func. 'toV86()' from 'raise (SIGBUS)' */
    return;
  }
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void sigtrap_handler(struct sigframe sf, struct trapframe tf)
{
  int intnum;

#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;

  if ((sf.sf_sc.sc_ps & PSL_VM) == 0)
    ErrExit (0x03, "Sigtrap in protected mode\n");
  else {
#ifdef DEBUG
    dprintf("%s int at address %4.4X:%4.4X (%4.4X:%4.4X) %8.8X\n",
	      tf.tf_trapno == T_BPTFLT ? "breakpoint" : "trace",
	      tf.tf_cs, tf.tf_ip, tf.tf_ss, tf.tf_sp, tf.tf_eflags);
#endif
    if (tf.tf_trapno == T_BPTFLT) {
      PUSH((tf.tf_eflags & (~PSL_I)) | INT_STATE | PSL_IOPL, &tf);
      intnum = 3;
    } else {
      PUSH((tf.tf_eflags & (~PSL_I)) | INT_STATE | PSL_IOPL | PSL_T, &tf);
      intnum = 1;
    }
    PUSH(tf.tf_cs, &tf);
    PUSH(tf.tf_ip, &tf);
    tf.tf_cs = GETWORD(intnum * 4 + 2);
    tf.tf_ip = GETWORD(intnum * 4);
    INT_STATE = 0;
  }
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void sigfpe_handler(struct sigframe sf, struct trapframe tf)
{
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;

  if ((sf.sf_sc.sc_ps & PSL_VM) == 0)
    ErrExit (0x04, "Sigfpe in protected mode\n");
  else {
    if (drop_fpe(&tf))
      check_interrupts(&tf);
    else
      ErrExit (0x05, "Not supported floating point instruction\n");
  }
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void sigio_handler(struct sigframe sf, struct trapframe tf)
{
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;
  /*
   * SIGIO with trapno = T_PROTFLT may be if need_interrupt set and
   * there is transition cli -> sti. In this case we need
   * call check_interrupts() only. Else - perform keyboard input.
   */
  if (tf.tf_trapno == T_PROTFLT)
    connect_area->need_interrupt = 0;
  else 
    do_keyboard_input();

  if (sf.sf_sc.sc_ps & PSL_VM)
    check_interrupts(&tf);
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void timer_handler(struct sigframe sf, struct trapframe tf)
{
  extern unsigned short timer_counter;

#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;

  do_inputs();
  do_timer_tick();

  if (sf.sf_sc.sc_ps & PSL_VM)
    check_interrupts(&tf);

  PushKeyboardBySig ();

#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void timer2_handler(struct sigframe sf, struct trapframe tf)
{
  struct itimerval value;
  struct itimerval ovalue;

#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;

  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec = 0;
  value.it_value.tv_usec = 0;
  setitimer (ITIMER_PROF, &value, &ovalue);
  KbTimeExpired ();

  if (sf.sf_sc.sc_ps & PSL_VM)
    check_interrupts(&tf);
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (1,&sf,&tf);
#endif
#endif
}

/*********************************************************/

void random_sig_handler(struct sigframe sf, struct trapframe tf)
{
#ifdef DEBUG
#ifdef DEB_SGN
  LogPrintSig (0,&sf,&tf);
#endif
#endif

  if (v86_proc == 0  && (sf.sf_sc.sc_ps & PSL_VM))
    cur_trapframe = &tf;
  ErrExit (0x06, "Unexpected signal %d \n", sf.sf_signum);
}

/*********************************************************/

void block_all_signals()
{
  sigset_t all_block = (sigset_t) -1;

  if (sigprocmask(SIG_SETMASK, &all_block, &old_signal_state) < 0)
    return;
}

void unblock_signals()
{
  sigprocmask(SIG_SETMASK, &old_signal_state, NULL);
}

/*********************************************************/

static void switch_vm86(struct sigframe sf, struct trapframe tf)
{
  __asm__ volatile("addl $8, %esp");	/* drop garbage from stack */
  __asm__ volatile("movl $103,%eax");	/* sigreturn syscall number */
  /* enter kernel with args on stack */
  __asm__ volatile(".byte 0x9a; .long 0; .word 0x7");
  __asm__ volatile("hlt");		/* never gets here */
}

/*********************************************************/

main(int argc, char **argv)
{
  int i;
  struct sigframe sf;
  struct trapframe tf;
  struct sigstack sstack;

  seteuid (getuid ());

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0)
      no_save_video++;
    else {
      fprintf (stderr, "bad flag %s\n", argv[i]);
      exit (1);
    }
  }

  bzero(&tf, sizeof(struct trapframe));

  sstack.ss_sp = (void *)((int)&argc - 1024);
  sstack.ss_onstack = 0;
  sigstack(&sstack, NULL);

  for (i = 0; i < NSIG; i++)
    setsignal(i, random_sig_handler);

  read_config_file();

  connect_area = (struct connect_area *)read_connect_area_address();
#ifdef DEBUG
  dprintf("Connect area address: 0x%x\n", connect_area);
#endif
  load_image(ROMBIOS_FILE_NAME, (int)connect_area);
  load_image(FIRSTPAGE_FILE_NAME,
	     MAKE_ADDR(connect_area->first_page_seg,
		       connect_area->first_page_off));

  tf.tf_eflags = 0x20202;		/* IOPL 0 !!! */
  tf.tf_cs = connect_area->rom_start_cs;
  tf.tf_ip = connect_area->rom_start_ip;
  tf.tf_ss = connect_area->rom_start_ss;
  tf.tf_sp = connect_area->rom_start_sp;

  if (strcmp(ttyname(0), "/dev/console")) {
    connect_area->remote_terminal++;
    fprintf (stderr, "Current version doesn't support remote terminal\n");
    exit (1);
  }

  map_device("/dev/vga", 0xa0000, 0x50000);
  INT_STATE = 0;

  setsignal(SIGBUS, sigvm86_handler);
  setsignal(SIGILL, sigvm86_handler);
  setsignal(SIGALRM, timer_handler);
  setsignal(SIGPROF, timer2_handler);
  setsignal(SIGIO, sigio_handler);
  setsignal(SIGTRAP, sigtrap_handler);
  setsignal(SIGFPE, sigfpe_handler);
  setsignal(SIGTSTP, SIG_DFL);
  setsignal(SIGCONT, SIG_IGN);
  setsignal(SIGCHLD, SIG_IGN);  /* ignore 'lpr' exiting */

  init_ports();	/* Must be before all other equipment initializations */
  init_console(); /* should be nearly first for ErrExit 'resets' console */
  init_services();
  init_cmos();
  init_keyboard();
  init_interrupts();
  init_timer();
  init_printer();
  init_game_port();
  init_mouse();
  /* InitVideo() is called from V86 just after its initialization */

  sf.sf_eax = kernel_emulator ? (int)connect_area : -1;
  sf.sf_scp = &sf.sf_sc;
  sf.sf_sc.sc_ps = PSL_VM;
  sf.sf_sc.sc_mask = sf.sf_sc.sc_onstack = 0;
  switch_vm86(sf, tf);
}
