/*
 * $Id: kernel.h,v 1.2 1992/09/02 23:59:40 polk Exp $
 */

/*
 * From frame.h (slightly changed)
 */
struct trapframe {
	int	tf_es386;
	int	tf_ds386;
	ushort	tf_di;
	ushort	:16;
	ushort	tf_si;
	ushort	:16;
	ushort	tf_bp;
	ushort	:16;
	int	tf_isp386;
	ushort	tf_bx;
	ushort	:16;
	ushort	tf_dx;
	ushort	:16;
	ushort	tf_cx;
	ushort	:16;
	ushort	tf_ax;
	ushort	:16;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	ushort	tf_ip;
	ushort	:16;
	ushort	tf_cs;
	ushort	:16;
	int	tf_eflags;
	/* below only when transitting rings (e.g. user to kernel) */
	ushort	tf_sp;
	ushort	:16;
	ushort	tf_ss;
	ushort	:16;
	/* below only when transitting from vm86 */
	ushort	tf_es;
	ushort	:16;
	ushort	tf_ds;
	ushort	:16;
	ushort	tf_fs;
	ushort	:16;
	ushort	tf_gs;
	ushort	:16;
};

#define MAKE_ADDR(sel, off)	((((int)(sel))<<4) + (int)(off))
#define GETBYTE(addr)		(*(unsigned char *)(addr))
#define GETWORD(addr)		(*(unsigned short *)(addr))
#define PUTBYTE(addr, x)	((*(unsigned char *)(addr)) = (x))
#define PUTWORD(addr, x)	((*(unsigned short *)(addr)) = (x))

inline static void PUSH(ushort x, struct trapframe *frame)
{
  frame->tf_sp -= 2;
  PUTWORD(MAKE_ADDR(frame->tf_ss, frame->tf_sp), x);
}

inline static ushort POP(struct trapframe *frame)
{
  ushort x = GETWORD(MAKE_ADDR(frame->tf_ss, frame->tf_sp));
  frame->tf_sp += 2;
  return x;
}

#define DEVNAMESIZE 128

enum devtype {DEV_FLOP, DEV_HARD, DEV_LPT, DEV_MOUSE};

struct device {
  enum devtype dev_type;
  int dev_num;
  char *dev_name;
  char dev_real_name[DEVNAMESIZE];
  int dev_id;		/*  for DEV_FLOP and DEV_HARD only */
};

struct device *search_device(enum devtype dtype, int dnum);

#define MicroSoft	0
#define MouseSystems	1
#define MMSeries	2
#define Logitech	3

struct connect_area {
  int int_state;            /* enable/disable interrupt (cli/sti) state */
  int need_interrupt;       /* used by kernel to send "hardware" int */
  int disk_connect_area;
  long require_tic_count;
  int addr_for_KB;          /* not used now */
  int num_of_printers;      /* filled by monitor while reading CONFIG file */
  int num_of_COMs;          /* filled by monitor while reading CONFIG file */
  int remote_terminal;
  int menu_V86_stack;       /* filled by ROM, used by monitor */
  int menu_V86_addr;        /* filled by ROM, used by monitor */
  int pattern_of_seg0;      /* filled by ROM, used by monitor */
  int pattern_of_seg40;     /* filled by ROM, used by monitor */
  int addr_buf_lpt[3];      /* filled by ROM, used by both monitor and ROM */
  int int_10_parm;          /* reserved for int 10 parms for remote terminal */
  int reserv2[8];
  unsigned short rom_start_ip;
  unsigned short rom_start_cs;
  unsigned short rom_start_sp;
  unsigned short rom_start_ss;
  unsigned short first_page_off; /* provided by ROM to read 1st page */
  unsigned short first_page_seg; /* provided by ROM to read 1st page */
  int hard_disk_table;
  int addr_of_floppy_parm_table;
  int error_text;                /* address to place text of error message */
  int rom_offset_inside_f000;    /* provided by ROM to read all ROM image */
};

#define INT_STATE connect_area->int_state

#ifdef DEBUG
static inline int CurrSP (void)
{
  int rs;
  __asm__ volatile ("movl %%esp,%0" : "=g" (rs) );
  return rs;
}

void dprintf(const char *fmt, ...);
#endif
