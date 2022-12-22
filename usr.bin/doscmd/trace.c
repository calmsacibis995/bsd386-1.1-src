/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: trace.c,v 1.1 1994/01/14 23:37:15 polk Exp $ */
#include "doscmd.h"
#include "trap.h"

int tmode = 0;

static u_short inst;
static u_short jinst;
static u_long addr = 0;
static u_long jaddr = 0;

extern unsigned long jump_pc;
extern unsigned long jump_regs;

#define	aget(x)		(((x & 0xffff0000) >> 12) + (x & 0xffff))
#define	get(x)		*(u_short *)(((x & 0xffff0000) >> 12) + (x & 0xffff))

#define	TTRAP	0x03cd

void
resettrace()
{
    if (addr) {
	get(addr) = inst;
	addr = 0;
    }
    if (jaddr) {
	get(jaddr) = jinst;
	jaddr = 0;
    }
}

void
tracetrap(struct trapframe *tf)
{
    char buf[100];
    char buf2[100];
    u_char opcode;

    addr = (tf->tf_cs << 16) | tf->tf_ip;

    opcode = get(addr) & 0xff;
    switch (opcode) {
    case 0xc2: /* Near ret */
    case 0xc3: /* Near ret */
	i386dis(tf->tf_cs, tf->tf_ip, aget(addr), buf, 0);
    	addr &= ~0xffff;
	addr |= PEEK(tf, 0);
	break;
    case 0xca: /* Far ret */
    case 0xcb: /* Far ret */
	i386dis(tf->tf_cs, tf->tf_ip, aget(addr), buf, 0);
	addr = PEEK(tf, 0);
	addr |= PEEK(tf, 1) << 16;
	break;
    default:
	addr += i386dis(tf->tf_cs, tf->tf_ip, aget(addr), buf, 0);
    }

    printtrace(tf, buf);

#if 0
if (tf->tf_cs == 0x0070 && tf->tf_ip == 0x1dd9) {
    u_short ip = tf->tf_ip;
    u_long ad = (tf->tf_cs << 16) | tf->tf_ip;
    int i;
    i = i386dis(tf->tf_cs, ip, aget(ad), buf, 0);
    printtrace(tf, buf);
    ad += i;
    tf->tf_ip += i;
    i = i386dis(tf->tf_cs, tf->tf_ip, aget(ad), buf, 0);
    printtrace(tf, buf);
    ad += i;
    tf->tf_ip += i;
    i = i386dis(tf->tf_cs, tf->tf_ip, aget(ad), buf, 0);
    printtrace(tf, buf);
    ad += i;
    tf->tf_ip += i;
    i = i386dis(tf->tf_cs, tf->tf_ip, aget(ad), buf, 0);
    printtrace(tf, buf);
    ad += i;
    tf->tf_ip += i;
    i = i386dis(tf->tf_cs, tf->tf_ip, aget(ad), buf, 0);
    printtrace(tf, buf);

    tf->tf_ip = ip;
}
#endif

    if (jump_regs && !jump_pc) {
    	unsigned short offset = 0;
    	switch (jump_regs >> 24) {
	case es_reg:
	    jump_pc = tf->tf_es << 16;
	    break;
	case cs_reg:
	    jump_pc = tf->tf_cs << 16;
	    break;
	case ss_reg:
	    jump_pc = tf->tf_ss << 16;
	    break;
	case ds_reg:
	    jump_pc = tf->tf_ds << 16;
	    break;
	case fs_reg:
	    jump_pc = tf->tf_fs << 16;
	    break;
	case gs_reg:
	    jump_pc = tf->tf_gs << 16;
	    break;
    	}

    	switch (((jump_regs >> 16) & 0xff) - 1) {
    	case bx_si_reg:
    	    offset = (tf->tf_bx + tf->tf_si) & 0xffff;
	    break;
    	case bx_di_reg:
    	    offset = (tf->tf_bx + tf->tf_di) & 0xffff;
	    break;
    	case bp_si_reg:
    	    offset = (tf->tf_bp + tf->tf_si) & 0xffff;
	    break;
    	case bp_di_reg:
    	    offset = (tf->tf_bp + tf->tf_di) & 0xffff;
	    break;
    	case si_reg:
    	    offset = tf->tf_si;
	    break;
    	case di_reg:
    	    offset = tf->tf_di;
	    break;
    	case bp_reg:
    	    offset = tf->tf_bp;
	    break;
    	case bx_reg:
    	    offset = tf->tf_bx;
	    break;
    	}

    	offset += jump_regs & 0xffff;
	offset &= 0xffff;

    	jump_pc |= offset; 
    	switch (opcode) {
	case 0xff:
	    jump_pc = (get(jump_pc) << 16) | get((jump_pc + 2));
	    break;
    	default:
	    jump_pc = (tf->tf_cs << 16) | get(jump_pc);
	    break;
    	}
    }

    inst = get(addr);
    get(addr) = TTRAP;
    if (jaddr = jump_pc) {
	jinst = get(jaddr);
	get(jaddr) = TTRAP;
    }
#if 0
if (jump_regs) {
    int i;
    u_short *cs = (u_short *)(tf->tf_cs << 4);
    u_char *ccs = (u_char *)(tf->tf_cs << 4);
    u_short save[4];
    memcpy(save, (tf->tf_cs << 4) + tf->tf_ip, 8);
printf("Jump_regs\n");
    for (i = 0; i < 32*1024; ++i)
	cs[i] = 0xffcd;
    ccs[0x0e] = 0x00;
    ccs[0x0f] = 0x70;
    ccs[0x10] = 0x01;
    ccs[0x11] = 0x02;
    memcpy((tf->tf_cs << 4) + tf->tf_ip, save, 8);
}
#endif
}

inline
showstate(long flags, long flag, char f)
{
    putc((flags & flag) ? f : ' ', stderr);
}

printtrace(struct trapframe *tf, char *buf)
{
    static int first = 1;
    u_char *addr = (u_char *)MAKE_ADDR(tf->tf_cs, tf->tf_ip);
    char *bigfmt = "%04x:%04x "
#if BIG_DEBUG
	   	   "%02x %02x %02x %02x %02x %02x "
#endif
	   	   "%-30s "
           	   "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x ";
    char *smlfmt = "%04x:%04x "
#if BIG_DEBUG
	   	   "%02x %02x %02x %02x %02x %02x "
#endif
	   	   "%s\n";
    if (first) {
	fprintf(stderr, "%4s:%4s "
#if BIG_DEBUG
	       ".. .. .. .. .. .. "
#endif
	       "%-30s "
	       "%4s %4s %4s %4s %4s %4s %4s %4s %4s %4s %4s\n",
		"CS", "IP", "instruction",
		"AX", "BX", "CX", "DX",
		"DI", "SI", "SP", "BP",
		"SS", "DS", "ES");
	first = 0;
    }

    fprintf(stderr, bigfmt,
	    tf->tf_cs, tf->tf_ip,
#if BIG_DEBUG
	    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
#endif
	    buf,
	    tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx,
	    tf->tf_di, tf->tf_si, tf->tf_sp, tf->tf_bp,
	    tf->tf_ss, tf->tf_ds, tf->tf_es);
    showstate(tf->tf_eflags, PSL_C, 'C');
    showstate(tf->tf_eflags, PSL_PF, 'P');
    showstate(tf->tf_eflags, PSL_AF, 'c');
    showstate(tf->tf_eflags, PSL_Z, 'Z');
    showstate(tf->tf_eflags, PSL_N, 'N');
    showstate(tf->tf_eflags, PSL_T, 'T');
    showstate(tf->tf_eflags, PSL_I, 'I');
    showstate(tf->tf_eflags, PSL_D, 'D');
    showstate(tf->tf_eflags, PSL_V, 'V');
    showstate(tf->tf_eflags, PSL_NT, 'n');
    showstate(tf->tf_eflags, PSL_RF, 'r');
    showstate(tf->tf_eflags, PSL_VM, 'v');
    showstate(tf->tf_eflags, PSL_AC, 'a');
    putc('\n', stderr);
}
