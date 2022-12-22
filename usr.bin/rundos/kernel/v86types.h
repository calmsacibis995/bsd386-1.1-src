/*
 * Types definitions for V86 mode emulation.
 */

/*
 * Alexander Kolbasov.                   Started Wed Mar 11 1992.
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/rundos/kernel/v86types.h,v 1.2 1992/09/03 00:04:02 polk Exp $
 */

#pragma once
#ifndef __V86TYPES_H_
#define __V86TYPES_H_

/*
 * This file may be shared by V86 monitor programs and DOS-based BIOS
 * programs. Thus it includes some parts for correct compilation under
 * MS-DOS.
 */

/*
 * Names BYTE, WORD and DWORD (and their u_ forms) are for more easy 
 * ports between platforms. They describe size of elements in bytes
 * that is essential for V86 mode programs. On compilers with different
 * sizes of char, int, long you should typedef them for the appropriate
 * type.
 */

typedef char             BYTE;
typedef unsigned char  u_BYTE;
typedef long             DWORD;
typedef unsigned long  u_DWORD;
typedef short            WORD;
typedef unsigned short u_WORD;

#define V86_WORD_SIZE 16

/*
 * I don't want to use flags and corresponding bits from system include
 * files because the fact that V86 flags bits are the same as flags in
 * host computer is virtualy random fact. Logically they are completely
 * different!
 */

typedef struct
{
  u_WORD  fl_cf : 1,                 /* carry/borrow */
                : 1,                 /* reserved     */
          fl_pf : 1,                 /* parity       */
                : 1,                 /* reserved     */
          fl_af : 1,                 /* aux carry    */
                : 1,                 /* reserved     */
          fl_zf : 1,                 /* zero         */
          fl_sf : 1,                 /* sign         */
          fl_tf : 1,                 /* trap         */
          fl_if : 1,                 /* interrupt enable */
          fl_df : 1,                 /* direction    */
          fl_of : 1,                 /* overflow     */
          fl_reserved : 4;
} v86_flags_t;


/* Types to use 16 and 8-bit registers */

typedef struct {
    u_WORD x;
} V86_WORDREG;

typedef struct {
    u_BYTE low, high;
} V86_BYTEREG;

typedef union {
  V86_WORDREG wr;
  V86_BYTEREG br;
} V86_REG;

/*
 * This is the conventional structure of common area for parameter
 * passing between V86 task and its monitor. It is used by monitor
 * services.
 */

typedef struct V86_REGPACK {
  u_WORD ret_code;                      /* For emulation return codes */
  V86_REG r_ax, r_bx, r_cx, r_dx;
  V86_REG r_bp, r_si, r_di, r_ds, r_es;
  v86_flags_t  r_flags;
} *V86_REGPACK_t;

/* Macros for extracting different registers from V86_REGPACK_t */

#define get_AX(regs) ((regs)->r_ax.wr.x)
#define get_BX(regs) ((regs)->r_bx.wr.x)
#define get_CX(regs) ((regs)->r_cx.wr.x)
#define get_DX(regs) ((regs)->r_dx.wr.x)

#define get_AH(regs) ((regs)->r_ax.br.high)
#define get_AL(regs) ((regs)->r_ax.br.low)
#define get_BH(regs) ((regs)->r_bx.br.high)
#define get_BL(regs) ((regs)->r_bx.br.low)
#define get_CH(regs) ((regs)->r_cx.br.high)
#define get_CL(regs) ((regs)->r_cx.br.low)
#define get_DH(regs) ((regs)->r_dx.br.high)
#define get_DL(regs) ((regs)->r_dx.br.low)

#define get_ES(regs) ((regs)->r_es.wr.x)
#define get_DI(regs) ((regs)->r_di.wr.x)

/* Small stuff for manipulating carry flag */

#define get_v86_flags(regs)  ((regs)->r_flags)

#define clear_CF(flags)      ((flags).fl_cf = 0)
#define raise_CF(flags)      ((flags).fl_cf = 1)
#define get_CF(flags)        ((flags).fl_cf)

/*
 * Macro MK_FP(seg, ofs) creates 32-bit linear address from 16
 * bit segment and offset using v86 mode address translation method
 */

#define MK_FP(seg, ofs) ((caddr_t)((u_DWORD)(((u_DWORD)(seg) << 4) + (ofs))))

/* Macros to extract segment and offset part from pointers */

#if !defined(GET_SEG)
#  define GET_SEG(x) ((u_WORD)(((u_DWORD)(x)) >> V86_WORD_SIZE))
#endif

#if !defined(GET_OFS)
#  define GET_OFS(x) ((u_WORD)(((u_DWORD)(x)) & 0xFFFF))
#endif

/*
 * Macro MK_32_ADDR translates V86 segment-offset address into flat
 * 32-bit address.  
 */

#if !defined(MK_32_ADDR)
#  define MK_32_ADDR(x) (MK_FP(GET_SEG(x), GET_OFS(x)))
#endif

/*
 * This is the structure of the connection area in V86 address space.
 * All parameters are accesses via this structure.
 */

typedef struct connection_block
{
  DWORD cli_flag;          /* Virtual CLI flag. */
  DWORD pending_flag;      /* Interrupts are waiting */
  DWORD addr_link_fi13;    /* Adres of parameter block for int13 interface */
  WORD  skipped_ticks_lo;
  WORD  skipped_ticks_hi;
  DWORD addr_for_KB;
  DWORD	num_printers;
  DWORD	num_COMs;
  DWORD	remote_terminal;
  DWORD	menue_stack;
  DWORD	menue_fun;
  DWORD aimseg0;
  DWORD aimseg40;
  DWORD adr_bufLPT1;
  DWORD adr_bufLPT2;
  DWORD adr_bufLPT3;
  DWORD reserv;
  DWORD com1_com4 [8];                  /* com1 - com4 reserved now */
  DWORD cs_ip;
  DWORD ss_sp;
  DWORD pg_0;
  DWORD _hdisk_table;                  /* Hard disks parameters table */
  DWORD addr_ADDR_fl_PARAMS;           /* Indirect address of floppy */
				       /* parameter table. */
} *connection_block_t;

#endif   /* __V86TYPES_H_ */
