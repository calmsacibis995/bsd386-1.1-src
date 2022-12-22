/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: doscmd.h,v 1.2 1994/01/14 23:27:50 polk Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <machine/psl.h>
#include <machine/npx.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dos.h"
#include "cwd.h"
#include "config.h"

#define	MAX_AVAIL_SEG 0xa000
char *dosmem;
char cmdname[];
int dosmem_size;

int init_cs;
int init_ip;
int init_ss;
int init_sp;
int init_ds;
int init_es;

int pspseg;                                     /* segment # of PSP */
int curpsp;

struct timeval boot_time;
 
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

/*
 * From frame.h (slightly changed)
 * SHOULD USE trapframe_vm86 from <machine/frame.h>
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

struct byteregs {
	u_char tf_bl;
	u_char tf_bh;
	u_short :16;
	u_char tf_dl;
	u_char tf_dh;
	u_short :16;
	u_char tf_cl;
	u_char tf_ch;
	u_short :16;
	u_char tf_al;
	u_char tf_ah;
	u_short :16;
};

int vflag;
int tmode;

inline static char *
MAKE_ADDR(int sel, int off)
{

	return (char *)((sel << 4) + off);
}

inline static void
PUSH(u_short x, struct trapframe *frame)
{
	frame->tf_sp -= 2;
	*(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp) = x;
}

inline static u_short
POP(struct trapframe *frame)
{
	u_short x = *(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp);

	frame->tf_sp += 2;
	return (x);
}

inline static u_short
PEEK(struct trapframe *frame, int off)
{
	u_short x = *(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp + off * 2);
	return (x);
}

#define cond_switch_vm86(sf, tf) \
{ \
        if ((sf.sf_sc.sc_ps & PSL_VM) && (tf.tf_eflags & PSL_VM)) \
                switch_vm86(sf, tf); \
}       


char *disk_transfer_addr;
int	disk_transfer_seg;
int	disk_transfer_offset;

u_char	*VREG;

#define	BIOSDATA	((u_char *)0x400)

unsigned long rom_config;

void dump_regs ();

int intnum;

void fake_int(struct trapframe *tf, int);

void mem_init (void);
int mem_alloc (int, int, int *);
int mem_adjust (int, int, int *);

void fatal (char *fmt, ...);
void debug (int flags, char *fmt, ...);

typedef void (*int_func_t)();
char *translate_filename(char *);

extern int_func_t int_func_table[256];
extern int	  int_table[512];

int debug_flags;
int exec_level;

/* Lower 8 bits are int number */
#define D_ALWAYS 	0x00100
#define D_TRAPS 	0x00200
#define D_FILE_OPS	0x00400
#define D_MEMORY	0x00800
#define D_HALF		0x01000 /* for "half-implemented" system calls */
#define	D_FLOAT		0x02000
#define	D_DISK		0x04000
#define	D_TRAPS2	0x08000
#define	D_PORT		0x10000
#define	D_EXEC		0x20000
#define	D_ITRAPS	0x40000
#define	D_REDIR		0x80000

#define	TTYF_ECHO	0x00000001
#define	TTYF_ECHONL	0x00000003
#define	TTYF_CTRL	0x00000004
#define	TTYF_BLOCK	0x00000008
#define	TTYF_POLL	0x00000010
#define	TTYF_REDIRECT	0x00010000	/* Cannot have 0xffff bits set */

#define	TTYF_ALL	(TTYF_ECHO | TTYF_CTRL | TTYF_REDIRECT)
#define	TTYF_BLOCKALL	(TTYF_ECHO | TTYF_CTRL | TTYF_REDIRECT | TTYF_BLOCK)

/*
 * Must match i386-pinsn.c
 */
#define es_reg		100
#define cs_reg		101
#define ss_reg		102
#define ds_reg		103
#define fs_reg		104
#define gs_reg		105

#define	bx_si_reg	0
#define	bx_di_reg	1
#define	bp_si_reg	2
#define	bp_di_reg	3
#define	si_reg		4
#define	di_reg		5
#define	bp_reg		6
#define	bx_reg		7

extern int redirect0;
extern int redirect1; 
extern int redirect2;
extern int xmode;
extern int dosmode;
extern int use_fork;
extern unsigned long *ivec;

extern int nfloppies;
extern int ndisks;
extern int nserial;
extern int nparallel;

void switch_vm86(struct sigframe, struct trapframe);
int init_hdisk(int drive, int cyl, int head, int tracksize,
	       char *file, char *boot_sector);
int init_floppy(int drive, int type, char *file);
int disk_fd(int drive);
void put_dosenv(char *value);

int tty_eread(struct trapframe *, int, int);
void tty_write(int, int);
void tty_rwrite(int, int);
void tty_move(int, int);
void tty_report(int *, int *);
void tty_flush();
void tty_index();
void tty_pause();
int tty_peek(struct trapframe *, int);
int tty_state();
void tty_scroll(int, int, int, int, int, int);
void tty_rscroll(int, int, int, int, int, int);
int tty_char(int, int);
void video_setborder(int);

void unknown_int2(int, int, struct trapframe *);
void unknown_int3(int, int, int, struct trapframe *);
void unknown_int4(int, int, int, int, struct trapframe *);
