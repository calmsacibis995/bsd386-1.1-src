#include <stdio.h>
#include <sys/types.h>
#include <machine/psl.h>
#define KERNEL
#include <machine/trap.h>
#include "kernel.h"

extern struct connect_area *connect_area;
  
#define CLI	0xfa
#define STI	0xfb
#define PUSHF	0x9c
#define POPF	0x9d
#define INT3	0xcc
#define INTn	0xcd
#define INTO	0xce
#define INBn	0xe4
#define INWn	0xe5
#define INB	0xec
#define INW	0xed
#define INSB	0x6c
#define INSW	0x6d
#define OUTBn	0xe6
#define OUTWn	0xe7
#define OUTB	0xee
#define OUTW	0xef
#define OUTSB	0x6e
#define OUTSW	0x6f
#define IRET	0xcf
#define HLT	0xf4
#define LOCK	0xf0

#define LOCK_PREFIX	0xf0
#define REPN_PREFIX	0xf2
#define REP_PREFIX	0xf3
#define CS_PREFIX	0x2e		/* 0x2d in FPE kernel emul -ha-ha */
#define SS_PREFIX	0x36
#define DS_PREFIX	0x3e
#define ES_PREFIX	0x26
#define FS_PREFIX	0x64
#define GS_PREFIX	0x65
#define SIZE_PREFIX	0x66
#define ADDR_PREFIX	0x67
#define NOT_PREFIX	0x00

unsigned char prefixes [] = {
  CS_PREFIX, SS_PREFIX, DS_PREFIX, ES_PREFIX, FS_PREFIX, GS_PREFIX,
  REPN_PREFIX, REP_PREFIX,
  SIZE_PREFIX, ADDR_PREFIX,
  LOCK_PREFIX, NOT_PREFIX
  };

/***************************************************/

#ifdef DEBUG
struct instruction {
  char *i_name;
  unsigned char i_opcode;
  unsigned char i_break;
} i_table[] = {
  "CLI",  0xfa, 1,
  "STI",  0xfb, 1,
  "PUSHF",  0x9c, 1,
  "POPF",  0x9d, 1,
  "INT3",  0xcc, 1,
  "INTn",  0xcd, 1,
  "INTO",  0xce, 1,
  "INB",  0xe4, 1,
  "INW",  0xe5, 1,
  "INB.dx",  0xec, 1,
  "INW.dx",  0xed, 1,
  "INSB",  0x6c, 1,
  "INSW",  0x6d, 1,
  "OUTB",  0xe6, 1,
  "OUTW",  0xe7, 1,
  "OUTB.dx",  0xee, 1,
  "OUTW.dx",  0xef, 1,
  "OUTSB",  0x6e, 1,
  "OUTSW",  0x6f, 1,
  "IRET",  0xcf, 1,
  "HLT",  0xf4, 1,
  "LOCK",  0xf0, 1,
  NULL,    0, 0,
  "CONSOLE", 0, 1		  /* Pseudo - only for trace from keyboard */
};

static unsigned char BSD_to_386_traps [] = {
  32, /* 0 = T_RESADFLT - reserved addressing */
  6,  /* 1 = T_PRIVINFLT - privileged instruction */
  33, /* 2 = T_RESOPFLT - reserved operand */
  3,  /* 3 = T_BPTFLT - breakpoint instruction */
  128, /* 4 - not defined */
  48, /* 5 = T_SYSCALL - system call (kcall) */
  16, /* 6 = T_ARITHTRAP - arithmetic trap */
  49, /* 7 = T_ASTFLT - system forced exception */
  34, /* 8 = T_SEGFLT - segmentation (limit) fault */
  13, /* 9 = T_PROTFLT - protection fault */
  1,  /* 10 = T_TRCTRAP - trace trap */
  128, /* 11 - not defined */
  14, /* 12 = T_PAGEFLT - page fault */
  35, /* 13 = T_TABLEFLT - page table fault */
  36, /* 14 = T_ALIGNFLT - alignment fault */
  37, /* 15 = T_KSPNOTVAL - kernel stack pointer not valid */
  38, /* 16 = T_BUSERR - bus error */
  39, /* 17 = T_KDBTRAP - kernel debugger trap */
  0,  /* 18 = T_DIVIDE - integer divide fault */
  2,  /* 19 = T_NMI - non-maskable trap */
  4,  /* 20 = T_OFLOW - overflow trap */
  5,  /* 21 = T_BOUND - bound instruction fault */
  7,  /* 22 = T_DNA - device not available fault */
  8,  /* 23 = T_DOUBLEFLT - double fault */
  9,  /* 24 = T_FPOPFLT - fp coprocessor operand fetch fault */
  10, /* 25 = T_TSSFLT - invalid tss fault */
  11, /* 26 = T_SEGNPFLT - segment not present fault */
  12, /* 27 = T_STKFLT - stack fault */
  15  /* 28 = T_RESERVED - reserved fault base */
  };

#endif   DEBUG

/***************************************************/

/* 
 * Fake input/output ports
 */

static void outb_nullport(int port, unsigned char byte)
{
}

static unsigned char inb_nullport(int port)
{
  return 0xff;
}

#define MAXPORT 0x400

/*
 * configuration table for ports' emulators
 */

struct portsw {
  unsigned char		(*p_inb)(int portnum);
  void			(*p_outb)(int portnum, unsigned char byte);
} portsw[MAXPORT];

void init_ports(void)
{
  int i;

  for (i=0; i < MAXPORT; i++) {
    portsw[i].p_inb = inb_nullport;
    portsw[i].p_outb = outb_nullport;
  }
}

void define_in_port(int port, unsigned char (*p_inb)(int portnum))
{
  if (port < 0 || port >= MAXPORT)
    ErrExit (0x11, "Attempt to handle invalid port %X\n", port);
  portsw[port].p_inb = p_inb;
}

void define_out_port(int port, void (*p_outb)(int portnum, unsigned char byte))
{
  if (port < 0 || port >= MAXPORT)
    ErrExit (0x11, "Attempt to handle invalid port %X\n", port);
  portsw[port].p_outb = p_outb;
}

/************************************************************/

static void PrepareForV86Int (int intno, struct trapframe *tf)
{
  PUSH((tf->tf_eflags & (~PSL_I)) | INT_STATE | PSL_IOPL, tf);
  PUSH(tf->tf_cs, tf);
  PUSH(tf->tf_ip, tf);
  INT_STATE = 0;
  tf->tf_eflags &= ~PSL_T;
  tf->tf_cs = GETWORD(intno * 4 + 2);
  tf->tf_ip = GETWORD(intno * 4);
}

/************************************************************/

int emulate(struct trapframe *tf)
{
  int addr, portnum;
  int word_width = 0;
  void (*out_handler)(int, unsigned char);
  unsigned char (*in_handler)(int);
  int sel, step;
  int i = 0;
  int prefix = 0;

#define seg_cs_prefix (prefix & 0x01)
#define seg_ss_prefix (prefix & 0x02)
#define seg_ds_prefix (prefix & 0x04)
#define seg_es_prefix (prefix & 0x08)
#define seg_fs_prefix (prefix & 0x10)
#define seg_gs_prefix (prefix & 0x20)
#define all_segm_mask 0x3F
#define segm_prefix (prefix & all_segm_mask)
#define repn_prefix (prefix & 0x40)
#define rep_prefix (prefix & 0x80)
#define ins_prefix (prefix & 0xC0)
#define op_size_prefix (prefix & 0x100)
#define addr_size_prefix (prefix & 0x200)
#define lock_prefix (prefix & 0x400)

  addr = MAKE_ADDR(tf->tf_cs, tf->tf_ip);

  while ( prefixes[i] != NOT_PREFIX ) {
    if (prefixes[i] == GETBYTE(addr)) {
      if ( ((1<<i) & all_segm_mask) && segm_prefix )
	prefix &= ~all_segm_mask;
      prefix |= (1<<i);
      addr++;
      tf->tf_ip++;
      i = 0;
    }
    else
      ++i;
  }

  if (seg_cs_prefix)
    sel = tf->tf_cs;
  if (seg_ss_prefix)
    sel = tf->tf_ss;
  if (seg_ds_prefix)
    sel = tf->tf_ds;
  if (seg_es_prefix)
    sel = tf->tf_es;
  if (seg_fs_prefix)
    sel = tf->tf_fs;
  if (seg_gs_prefix)
    sel = tf->tf_gs;


#ifdef DEBUG
  {
    int i;
#ifdef TRACE
    extern tracing;
#endif
    unsigned char opcode = GETBYTE(addr);
    
    dprintf("cs:ip=%4.4X:%4.4X (ss:sp=%4.4X:%4.4X %4.4X %.4X %.4X) ",
	    tf->tf_cs, tf->tf_ip, tf->tf_ss, tf->tf_sp,
	    GETWORD(MAKE_ADDR(tf->tf_ss, tf->tf_sp+6)),
	    GETWORD(MAKE_ADDR(tf->tf_ss, tf->tf_sp+8)),
	    GETWORD(MAKE_ADDR(tf->tf_ss, tf->tf_sp+10))
	    );

    if (prefix)
      dprintf("prefix=%.4X ", prefix);

    for (i = 0; i_table[i].i_name != NULL; i++) {
      if (opcode == i_table[i].i_opcode) {
	dprintf("%s ", i_table[i].i_name);
	goto found;
      }
    }
    dprintf("opcode %X %X %X %X %X %X %X %X\n",
	    GETBYTE(addr),GETBYTE(addr+1),GETBYTE(addr+2),GETBYTE(addr+3),
	    GETBYTE(addr+4),GETBYTE(addr+5),GETBYTE(addr+6),GETBYTE(addr+7));
  found:
#ifdef TRACE
    if (tracing && i_table[i].i_break) {
      trace(i, tf);
    }
#endif
    ;
  }
#endif
  
  switch (GETBYTE(addr)) {

  case CLI:
    INT_STATE = 0;
    tf->tf_ip++;
    return 1;

  case STI:
    INT_STATE = PSL_I;
    tf->tf_ip++;
    return 1;

  case PUSHF:
    if (op_size_prefix)
      PUSH(PSL_VM>>16, tf);   /* high word of eflags: only V86 bit is set ! */
    PUSH((tf->tf_eflags & (~PSL_I)) | INT_STATE | PSL_IOPL, tf);
    tf->tf_ip++;
    return 1;

  case POPF:
    INT_STATE = POP(tf);
    if (op_size_prefix)
      (void) POP(tf);   /* bits V86 and RF really are not affected by POPFD */
    tf->tf_eflags = (tf->tf_eflags & 0xffff0000) |
      (INT_STATE & ~(PSL_IOPL | PSL_T)) | PSL_I;
    INT_STATE &= PSL_I;
    tf->tf_ip++;
    return 1;

  case INTn:
    tf->tf_ip += 2;
    PrepareForV86Int ( GETBYTE(addr + 1), tf);
    return 1;

  case INT3:
    /* trap (#3 !) currently goes to sigTRAP */

  case INTO:
    /* trap (#13 !) goes to sigBUS ... */
    tf->tf_ip += 1;
    PrepareForV86Int (4, tf);
    return 1;

  case IRET:
    {
      unsigned short tmp;

      tf->tf_ip= POP(tf);
      if (op_size_prefix) {
#ifdef DEBUG
	ErrorTo86 (" instruction IRETD ");
#endif
	if ( (tmp = POP(tf)) != 0 )
	  ErrorTo86 (" instruction IRETD:  EIP out of range ");
	*(&(tf->tf_ip) + 1) = tmp;   /* high portion of EIP */
      }
      tf->tf_cs = POP(tf);
      if (op_size_prefix)
	(void) POP(tf);   /* CS has no high portion ! */
      INT_STATE = POP(tf);
      if (op_size_prefix)
	(void) POP(tf);   /* throw away high portion of EFLAGS ! */
      tf->tf_eflags = (tf->tf_eflags & 0xffff0000) |
	(INT_STATE & (~PSL_IOPL)) | PSL_I;
      INT_STATE &= PSL_I;
      return 1;
    }

  case HLT:
    /* trap (#13 !) currently goes to sigBUS ... */
    tf->tf_ip++;
    return 1;

  case INW:
    word_width = 1;
  case INB:
    portnum = tf->tf_dx;
    tf->tf_ip++;
    goto inputb;

  case INWn:
    word_width = 1;
  case INBn:
    portnum = GETBYTE(addr+1);
    tf->tf_ip += 2;

inputb:
    in_handler = inb_nullport;
    if (portnum >= 0 && portnum < MAXPORT)
      in_handler = portsw[portnum].p_inb;
    tf->tf_ax = ((*in_handler)(portnum)&0xff)
      			|((tf->tf_ax)&0xffffff00);
    if (word_width)
      tf->tf_ax = ((((*in_handler)(portnum + 1))&0xff) << 8)
			|((tf->tf_ax)&0xffff00ff);
#ifdef DEBUG
    if (word_width)
      dprintf("port=%X val=%4.4X\n", portnum, tf->tf_ax);
    else
      dprintf("port=%X val=%2.2X\n", portnum, (tf->tf_ax & 0x0FF));
#endif
    return 1;

  case INSW:
    word_width = 1;
  case INSB:
    if (tf->tf_eflags & PSL_D)
      step = -1;
    else
      step = 1;

    in_handler = inb_nullport;
    if (tf->tf_dx >= 0 && tf->tf_dx < MAXPORT)
      in_handler = portsw[tf->tf_dx].p_inb;

    if (ins_prefix) {
      i = tf->tf_cx;
      tf->tf_cx = 0;
    }
    else
      i = 1;

    if (!segm_prefix)
      sel = tf->tf_es;

    while (--i >= 0) {
      PUTBYTE(MAKE_ADDR(sel, tf->tf_di), (*in_handler)(tf->tf_dx));
      if (word_width) {
	PUTBYTE(MAKE_ADDR(sel, tf->tf_di + 1), (*in_handler)(tf->tf_dx + 1));
	tf->tf_di += step;
      }
      tf->tf_di += step;
    }
    tf->tf_ip++;
    return 1;

  case OUTW:
    word_width = 1;
  case OUTB:
    tf->tf_ip++;
    if (tf->tf_dx == 0xffff)
      ErrExit (0, "Exit caused by DOS' program\n");
    if (tf->tf_dx == 0x8000) {
#ifdef DEBUG
      dprintf("sys service #%2.2X\n",(tf->tf_ax & 0xFF));
#endif
      system_service(tf);
      return 1;
    }
    else {
      portnum = tf->tf_dx;
      goto output;
    }

  case OUTWn:
    word_width = 1;
  case OUTBn:
    portnum = GETBYTE(addr+1);
    tf->tf_ip += 2;

output:

#ifdef DEBUG
    if (word_width)
      dprintf("port=%X val=%4.4X\n", portnum, tf->tf_ax);
    else
      dprintf("port=%X val=%2.2X\n", portnum, (tf->tf_ax & 0x0FF));
#endif
    out_handler = outb_nullport;
    if (portnum >= 0 && portnum < MAXPORT)
      out_handler = portsw[portnum].p_outb;
    (*out_handler)(portnum, (tf->tf_ax)&0xff);
    if (word_width)
      (*out_handler)(portnum + 1, ((tf->tf_ax)&0xff00) >> 8);
    return 1;

  case OUTSW:
    word_width = 1;
  case OUTSB:
    if (tf->tf_eflags & PSL_D)
      step = -1;
    else
      step = 1;

    out_handler = outb_nullport;
    if (tf->tf_dx >= 0 && tf->tf_dx < MAXPORT)
      out_handler = portsw[tf->tf_dx].p_outb;

    if (ins_prefix) {
      i = tf->tf_cx;
      tf->tf_cx = 0;
    }
    else
      i = 1;

    if (!segm_prefix)
      sel = tf->tf_ds;

    while (--i >= 0) {
      (*out_handler)(tf->tf_dx, GETBYTE(MAKE_ADDR(sel, tf->tf_si)));
      if (word_width) {
	(*out_handler)(tf->tf_dx + 1, GETBYTE(MAKE_ADDR(sel, tf->tf_si + 1)));
	tf->tf_si += step;
      }
      tf->tf_si += step;
    }
    tf->tf_ip++;
    return 1;

  default:
    if ( tf->tf_cs == 0xFFFF  &&  tf->tf_ip == 0x0000 )
      /* sigBUS interrupted by Ctrl+Alt+Del */
      return 1;
    else {
      char err_buf [100];

#ifdef DEBUG
      sprintf (err_buf," trap No %.2X ", BSD_to_386_traps[tf->tf_trapno]);
      sprintf (err_buf+12,
	       "code %.2X %.2X %.2X %.2X %.2X %.2X at addr %.4X:%.4X %c",
	       GETBYTE(addr),GETBYTE(addr+1),GETBYTE(addr+2),GETBYTE(addr+3),
	       GETBYTE(addr+4),GETBYTE(addr+5),tf->tf_cs,tf->tf_ip,'\0');
      ErrorTo86 (err_buf);
#endif
      switch (tf->tf_trapno) {

      case T_PRIVINFLT: /* fault #6 */
	sprintf (err_buf," ERROR: invalid opcode %c",'\0');
	ErrorTo86 (err_buf);
	PrepareForV86Int (6, tf);
	return 1;
      case T_STKFLT:  /* fault #12 - currently doesn't come ! (kernel error) */
	sprintf (err_buf," ERROR: stack fault %c",'\0');
	ErrorTo86 (err_buf);
	PrepareForV86Int (12, tf);
	return 1;
      case T_PROTFLT:   /* fault #13 */
	sprintf (err_buf," ERROR: general protection fault %c",'\0');
	ErrorTo86 (err_buf);
	PrepareForV86Int (13, tf);
	return 1;
      default:
	return 0;
      }
    }
  }
}

/***************************************************************/

/*
 * Commands: recognition, prefixes,
 */
struct cmd {
	unsigned c_opa:3;
	unsigned c_esc:5;
	unsigned c_rm:3;
	unsigned c_opb:3;
	unsigned c_mod:2;
};

#define	FWAIT		0x9B
#define C_ESC		0x001B

#define ush(cmd) (*(unsigned short*)&cmd)

int drop_fpe(struct trapframe *tf)
{
  int rm, mod;
  int cmd_len;
  int addr16;
  struct cmd cmd;

  switch (tf->tf_trapno) {

  case T_DIVIDE:
    /* fault (#0 !) currently goes to sigFPE ... */
    PrepareForV86Int (0, tf);
    return 1;
  case T_BOUND:
    /* fault (#5 !) currently goes to sigFPE ... */
    PrepareForV86Int (5, tf);
    return 1;
  case T_ARITHTRAP:
    /* trap (#16 !) must never occure but if does then goes to sigFPE ... */
    PrepareForV86Int (16, tf);
    return 1;
  case T_DNA:
    break;
  default:
    return 0;
  }

/*
 * skip 80x87 instruction if T_DNA (Device Not Available)
 */

  ush(cmd) = GETWORD(MAKE_ADDR(tf->tf_cs, tf->tf_ip));
#ifdef DEBUG
  dprintf("fp: 0x%x:0x%x cmd %x\n", tf->tf_cs, tf->tf_ip, ush(cmd));
#endif
  addr16 = 1;
  cmd_len = 0;
  while (cmd.c_esc != C_ESC) {
    switch ((unsigned char)ush(cmd)) {
    case FWAIT: 
#ifdef DEBUG
      dprintf("fp:FWAIT\n");
#endif 
      tf->tf_ip += cmd_len + 1;
      return 1;
    case ADDR_PREFIX:
#ifdef DEBUG
      dprintf("fp:ADDR_PREFIX\n");
#endif 
      addr16 = 0;
      break;
    case SIZE_PREFIX:
    case CS_PREFIX:
    case DS_PREFIX:
    case SS_PREFIX:
    case ES_PREFIX:
    case FS_PREFIX:
    case GS_PREFIX:
    case LOCK_PREFIX:
#ifdef DEBUG
      dprintf("fp: skip prefix\n");
#endif
      break;
    default:
      /*
       * 80387 call on not 80387 instruction	
       */
#ifdef DEBUG
      dprintf("fp: unknown instruction\n");
#endif
      return 0;
    }
    ++cmd_len;
    ush(cmd) = GETWORD(MAKE_ADDR(tf->tf_cs, tf->tf_ip + cmd_len));
  }

  rm = cmd.c_rm;
  mod = cmd.c_mod;
  cmd_len += 2;	/* 2 bytes for fp command */

  if ((mod != 3) && addr16) {
    if (mod == 0 && rm == 6)
      cmd_len += 2;
    else
      cmd_len += mod;
  } else if (mod != 3) {
    if (rm == 4) {
      struct sib {
	unsigned base:3;
	unsigned index:3;
	unsigned ss:2;
      } sib;

      *(unsigned char *)&sib = 
	GETWORD(MAKE_ADDR(tf->tf_cs, tf->tf_ip+cmd_len));
      
      if (sib.base == 5 && mod == 0) {
	cmd_len +=5;
      } else if (mod == 0)
	cmd_len++;
      else if (mod == 1) {
	cmd_len += 2;
      } else { /* mod == 2 */
	cmd_len += 5;
      }
    } else {
      switch (mod) {
      case 0:
	if (rm == 5)
	  cmd_len +=4;
	break;
      case 1:
	cmd_len += 1;
	break;
      case 2:
	cmd_len += 4;
	break;
      }
    }
  }
#ifdef DEBUG
  dprintf("fp: skip instruction (%d bytes)\n", cmd_len);
#endif
  tf->tf_ip += cmd_len;
  return(1);
}
