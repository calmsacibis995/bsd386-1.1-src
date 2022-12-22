#ifdef TRACE
#include <stdio.h>
#include <signal.h>
#include "kernel.h"
#include <termios.h>
#include <sys/ioctl.h>

int tracing = 0;
static FILE *fp_trace;
static struct termios tios;

void init_trace(char num)
{
  char tty[64];

  strcpy(tty, "/dev/ttypx");

  tty[strlen(tty) - 1] = num;

  if ((fp_trace = fopen(tty, "r+")) == NULL) {
#ifdef DEBUG
    dprintf("Cannot open trace terminal %s\n", tty);
#endif
    exit(1);
  }
  setbuf(fp_trace, NULL);
  ioctl(fileno(fp_trace), TIOCGETA, &tios);
  cfmakeraw(&tios);
  ioctl(fileno(fp_trace), TIOCSETA, &tios);
  tracing = 1;
#ifdef DEBUG
  dprintf("Open trace terminal %s\n", tty);
#endif
}

void print_regs(struct trapframe *tf)
{
  fprintf(fp_trace, "ax = %x\n", tf->tf_ax);
  fprintf(fp_trace, "bx = %x\n", tf->tf_bx);
  fprintf(fp_trace, "cx = %x\n", tf->tf_cx);
  fprintf(fp_trace, "dx = %x\n", tf->tf_dx);
  fprintf(fp_trace, "bp = %x\n", tf->tf_bp);
  fprintf(fp_trace, "sp = %x\n", tf->tf_sp);
  fprintf(fp_trace, "si = %x\n", tf->tf_si);
  fprintf(fp_trace, "di = %x\n", tf->tf_di);
  fprintf(fp_trace, "ip = %x\n", tf->tf_ip);
  fprintf(fp_trace, "cs = %x\n", tf->tf_cs);
  fprintf(fp_trace, "ss = %x\n", tf->tf_ss);
  fprintf(fp_trace, "ds = %x\n", tf->tf_ds);
  fprintf(fp_trace, "fs = %x\n", tf->tf_fs);
  fprintf(fp_trace, "gs = %x\n", tf->tf_gs);
  fprintf(fp_trace, "es = %x\n", tf->tf_es);
  fprintf(fp_trace, "eflags = %x\n", tf->tf_eflags);
}

extern struct instruction {
  char *i_name;
  unsigned char i_opcode;
  unsigned char i_break;
} i_table[];

static char command[BUFSIZ];

#define syntax_error()	\
  do { fprintf(fp_trace, "Syntax error\n"); cmd = 'c'; goto loop; } while(0)

void trace(int number, struct trapframe *tf)
{
  int i, c;
  int cmd = 0, repeate;
  sigset_t new, old;
  unsigned seg, offset;
  char reg[10];

  new = (sigset_t) -1;
  if (sigprocmask(SIG_SETMASK, &new, &old) < 0)
    return;
  
  if (number >= 0)
    fprintf(fp_trace, "%s ", i_table[number].i_name);
  else
    fprintf(fp_trace, "CONSOLE ");    

loop:
  if (number >= 0)
    fprintf(fp_trace, "%x:%x > ", tf->tf_cs, tf->tf_ip);
  i = 0;
  while (((c = fgetc(fp_trace)) != '\n') && (c != EOF))
    command[i++]  = c;
  if (i == 0 && c == '\n') {		/* repeate previous command */
    command[0] = cmd;
    repeate = 1;
  } else {
    command[i] = 0;
    repeate = 0;
  }
  
  switch(cmd = command[0]) {
  case EOF:
  case '\0':
  case 'c':					/* continue */
    break;
  case 'u':					/* remove breakpoint */
    if (sscanf(command, "%*s %s", reg) != 1)
      syntax_error();
    if (!strcmp(reg, "all")) {
      for (i = 0; i_table[i].i_name != NULL; i++)
	i_table[i].i_break = 0;
    } else {
      for (i = 0; i_table[i].i_name != NULL; i++)
	if (!strcmp(reg, i_table[i].i_name)) {
	  i_table[i].i_break = 0;
	  break;
	}
      if (i_table[i].i_name == NULL)
	syntax_error();
    }
    goto loop;
  case 'b':					/* set breakpoint on inst */
    if (sscanf(command, "%*s %s", reg) != 1)
      syntax_error();
    if (!strcmp(reg, "pr")) {
      for (i = 0; i_table[i].i_name != NULL; i++)
	if (i_table[i].i_break)
	  fprintf(fp_trace, "break %s on\n", i_table[i].i_name);
	else
	  fprintf(fp_trace, "break %s off\n", i_table[i].i_name);
      goto loop;
    } else {
      for (i = 0; i_table[i].i_name != NULL; i++)
	if (!strcmp(reg, i_table[i].i_name)) {
	  i_table[i].i_break = 1;
	  break;
	}
      if (i_table[i].i_name == NULL)
	syntax_error();
    }
    goto loop;
  case 'w':					/* write byte */
    if (repeate)
      offset++;
    else if (sscanf(command, "%*s %x:%x %x", &seg, &offset, &c) != 3)
      syntax_error();

    i = GETBYTE(MAKE_ADDR(seg, offset));
    PUTBYTE(MAKE_ADDR(seg, offset), c);
    fprintf(fp_trace, "%x:%x  %x = %x\n", seg, offset, i, c);
    goto loop;
  case 'x':					/* read byte */
    if (repeate)
      offset++;
    else if (sscanf(command, "%*s %x:%x", &seg, &offset) != 2)
      syntax_error();

    c = GETBYTE(MAKE_ADDR(seg, offset));
    fprintf(fp_trace, "%x:%x = %x\n", seg, offset, c);
    goto loop;
  case 'r':					/* print registers */
    if (number < 0) {
      syntax_error();
      goto loop;
    }
    if (!strcmp(command+2, "all"))
      print_regs(tf);
    else if (!strcmp(command+2, "ax")) 
      fprintf(fp_trace, "ax = %x\n", tf->tf_ax);
    else if (!strcmp(command+2, "bx")) 
      fprintf(fp_trace, "bx = %x\n", tf->tf_bx);
    else if (!strcmp(command+2, "cx")) 
      fprintf(fp_trace, "cx = %x\n", tf->tf_cx);
    else if (!strcmp(command+2, "dx")) 
      fprintf(fp_trace, "dx = %x\n", tf->tf_dx);
    else if (!strcmp(command+2, "bp")) 
      fprintf(fp_trace, "bp = %x\n", tf->tf_bp);
    else if (!strcmp(command+2, "sp")) 
      fprintf(fp_trace, "sp = %x\n", tf->tf_sp);
    else if (!strcmp(command+2, "si")) 
      fprintf(fp_trace, "si = %x\n", tf->tf_si);
    else if (!strcmp(command+2, "di")) 
      fprintf(fp_trace, "di = %x\n", tf->tf_di);
    else if (!strcmp(command+2, "ip")) 
      fprintf(fp_trace, "ip = %x\n", tf->tf_ip);
    else if (!strcmp(command+2, "cs")) 
      fprintf(fp_trace, "cs = %x\n", tf->tf_cs);
    else if (!strcmp(command+2, "ss")) 
      fprintf(fp_trace, "ss = %x\n", tf->tf_ss);
    else if (!strcmp(command+2, "ds")) 
      fprintf(fp_trace, "ds = %x\n", tf->tf_ds);
    else if (!strcmp(command+2, "fs")) 
      fprintf(fp_trace, "fs = %x\n", tf->tf_fs);
    else if (!strcmp(command+2, "gs")) 
      fprintf(fp_trace, "gs = %x\n", tf->tf_gs);
    else if (!strcmp(command+2, "es")) 
      fprintf(fp_trace, "es = %x\n", tf->tf_es);
    else if (!strcmp(command+2, "eflags")) 
      fprintf(fp_trace, "eflags = %x\n", tf->tf_eflags);
    else
      syntax_error();
    goto loop;
  case 's':					/* set register */
    if (number < 0) {
      syntax_error();
      goto loop;
    }
    if (sscanf(command, "%*s %s %x", reg, &c) != 2)
      syntax_error();
    if (!strcmp(reg, "ax")) {
      fprintf(fp_trace, "ax %x = %x\n", tf->tf_ax, c);
      tf->tf_ax = c;
    } else if (!strcmp(reg, "bx")) {
      fprintf(fp_trace, "bx %x = %x\n", tf->tf_bx, c);
      tf->tf_bx = c;
    } else if (!strcmp(reg, "cx")) {
      fprintf(fp_trace, "cx %x = %x\n", tf->tf_cx, c);
      tf->tf_cx = c;
    } else if (!strcmp(reg, "dx")) {
      fprintf(fp_trace, "dx %x = %x\n", tf->tf_dx, c);
      tf->tf_dx = c;
    } else if (!strcmp(reg, "bp")) {
      fprintf(fp_trace, "bp %x = %x\n", tf->tf_bp, c);
      tf->tf_bp = c;
    } else if (!strcmp(reg, "sp")) {
      fprintf(fp_trace, "sp %x = %x\n", tf->tf_sp, c);
      tf->tf_sp = c;
    } else if (!strcmp(reg, "si")) {
      fprintf(fp_trace, "si %x = %x\n", tf->tf_si, c);
      tf->tf_si = c;
    } else if (!strcmp(reg, "di")) {
      fprintf(fp_trace, "di %x = %x\n", tf->tf_di, c);
      tf->tf_di = c;
    } else if (!strcmp(reg, "ip")) {
      fprintf(fp_trace, "ip %x = %x\n", tf->tf_ip, c);
      tf->tf_ip = c;
    } else if (!strcmp(reg, "cs")) {
      fprintf(fp_trace, "cs %x = %x\n", tf->tf_cs, c);
      tf->tf_cs = c;
    } else if (!strcmp(reg, "ss")) {
      fprintf(fp_trace, "ss %x = %x\n", tf->tf_ss, c);
      tf->tf_ss = c;
    } else if (!strcmp(reg, "ds")) {
      fprintf(fp_trace, "ds %x = %x\n", tf->tf_ds, c);
      tf->tf_ds = c;
    } else if (!strcmp(reg, "fs")) {
      fprintf(fp_trace, "fs %x = %x\n", tf->tf_fs, c);
      tf->tf_fs = c;
    } else if (!strcmp(reg, "gs")) {
      fprintf(fp_trace, "gs %x = %x\n", tf->tf_gs, c);
      tf->tf_gs = c;
    } else if (!strcmp(reg, "es")) {
      fprintf(fp_trace, "es %x = %x\n", tf->tf_es, c);
      tf->tf_es = c;
    } else if (!strcmp(reg, "eflags")) {
      fprintf(fp_trace, "eflags %x = %x\n", tf->tf_eflags);
      tf->tf_eflags = c;
    } else
      syntax_error();
    goto loop;
  case 'q':					/* quit */
    reset_console();
    exit(1);
  default:
    syntax_error();
  }
  sigprocmask(SIG_SETMASK, &old, NULL);
}

int console_trace(void)
{
  if (!tracing)
    return 0;

  trace(-1, NULL);
  return 1;
}
#endif		/* TRACE */
