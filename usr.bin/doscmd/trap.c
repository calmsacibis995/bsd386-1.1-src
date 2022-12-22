/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: trap.c,v 1.2 1994/01/15 21:19:17 polk Exp $ */
#include "doscmd.h"
#include "trap.h"

int int_state = 0;

void
fake_int(struct trapframe *tf, int intnum)
{
    	if (tf->tf_cs == 0xF000 || (ivec[intnum] >> 16) == 0xF000) {
		debug (D_ITRAPS|intnum, "int%x:%x ", intnum, tf->tf_ax >> 8);
		switch(intnum) {
		case 0x00: printf("Divide by 0!\n"); break;
		case 0x10: int10(tf); break;
		case 0x11: int11(tf); break;
		case 0x12: int12(tf); break;
		case 0x13: int13(tf); break;
		case 0x14: int14(tf); break;
		case 0x15: int15(tf); break;
		case 0x16: int16(tf); break;
		case 0x17: int17(tf); break;
		case 0x08: if ((ivec[0x1c] >> 16) != 0xF000) {
			       intnum = 0x1c;
			       goto user_int;
			   }
		    	   break;
		case 0x1c: break;
		case 0x1a: int1a(tf); break;
		case 0x20: int20(tf); break;
		case 0x21: int21(tf); break;
		case 0x2f: int2f(tf); break;
		case 0x33: int33(tf); break;
		case 0xff: intff(tf); break;
		default:
		    if (vflag) dump_regs(tf);
		    fatal("no interrupt set up for 0x%02x\n", intnum);
		}
		debug (D_ITRAPS|intnum, "\n");
		tf->tf_ip += 2;
		return;
	}

user_int:
	if (ivec[intnum] == 0) {
		if (intnum == 0) {
		    printf("Divide by 0!\n");
		    return;
		}

		if (vflag) dump_regs(tf);
		fatal("No interrupt set up for 0x%02x\n", intnum);
	}

	/*
	 * This is really ugly, but when DOS boots, it seems to loop
	 * for a while on INT 16:11 INT 21:3E INT 2A:82
	 * INT 21:3E is a close(), which seems like something one would
	 * not sit on for ever, so we will allow it to reset our POLL count.
	 */
    	if (intnum == 0x21 && (tf->tf_ax >> 8) == 0x3E) {
	    	reset_poll();
	}

	debug (D_TRAPS|intnum, "INT%x:%x [%04x:%04x]\n",
	      intnum, tf->tf_ax >> 8,
	      ivec[intnum] >> 16, ivec[intnum] & 0xffff);

	PUSH(tf->tf_eflags, tf);
	PUSH(tf->tf_cs, tf);
	PUSH(tf->tf_ip + 2, tf);

	tf->tf_cs = ivec[intnum] >> 16;
	tf->tf_ip = ivec[intnum] & 0xffff;
}

static struct trapframe sda_trapframe;
static void (*sdafunc)(struct trapframe *, u_long);
static gettingsda = 0;

void
getsda(struct trapframe *tf, void (*func)(struct trapframe *, u_long))
{
    gettingsda = 1;
    sda_trapframe = *tf;
    sdafunc = func;
    tf->tf_ax = 0x5d06;
    tf->tf_bx = 0x0000;
    tf->tf_es = 0x0000;
    tf->tf_cs = ivec[0x21] >> 16;
    tf->tf_ip = ivec[0x21] & 0xffff;
    tf->tf_ip -= 2;
}

#define	MOD_REG_RM(w) \
	mod = ((w) >> 6) & 0x3; \
	reg = ((w) >> 3) & 0x7; \
	rm =   (w)  & 0x7

#define	READ_DATA \
	switch (mod) { \
	case 0: \
	    switch(rm) { \
	    case 0: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_si); break; \
	    case 1: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_di); break; \
	    case 2: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_si); break; \
	    case 3: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_di); break; \
	    case 4: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_si); break; \
	    case 5: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_di); break; \
	    case 6: daddr = MAKE_ADDR(tf.tf_ds, *(u_short *)addr); \
		    addr += 2; \
		    tf.tf_ip += 2; \
		    break; \
	    case 7: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx); break; \
	    } \
	    data = *(u_short *)daddr; \
	    break; \
	case 1: \
	    switch(rm) { \
	    case 0: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_si); break; \
	    case 1: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_di); break; \
	    case 2: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_si); break; \
	    case 3: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_di); break; \
	    case 4: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_si); break; \
	    case 5: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_di); break; \
	    case 6: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp); \
	    case 7: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx); break; \
	    } \
	    daddr += *(u_char *)addr; \
	    ++addr; \
	    ++tf.tf_ip; \
	    data = *(u_short *)daddr; \
	    break; \
	case 2: \
	    switch(rm) { \
	    case 0: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_si); break; \
	    case 1: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx + tf.tf_di); break; \
	    case 2: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_si); break; \
	    case 3: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp + tf.tf_di); break; \
	    case 4: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_si); break; \
	    case 5: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_di); break; \
	    case 6: daddr = MAKE_ADDR(tf.tf_ss, tf.tf_bp); \
	    case 7: daddr = MAKE_ADDR(tf.tf_ds, tf.tf_bx); break; \
	    } \
	    daddr += *(u_short *)addr; \
	    addr += 2; \
	    tf.tf_ip += 2; \
	    data = *(u_short *)daddr; \
	    break; \
	case 3: \
	    switch(rm) { \
	    case 0: data = tf.tf_ax; break; \
	    case 1: data = tf.tf_bx; break; \
	    case 2: data = tf.tf_cx; break; \
	    case 3: data = tf.tf_dx; break; \
	    case 4: data = tf.tf_sp; break; \
	    case 5: data = tf.tf_bp; break; \
	    case 6: data = tf.tf_si; break; \
	    case 7: data = tf.tf_di; break; \
	    } \
	    break; \
	}
	
void
trap(struct sigframe sf, struct trapframe tf)
{
    	char *daddr;
	int port;
	int addr;
	int okflags;
	int mod;
	int reg;
	int rm;
	int data;
	int din;
    	int rep;
    	int seg;
	int old_int_state = int_state;
	struct byteregs *b = (struct byteregs *)&tf.tf_bx;

	addr = (int)MAKE_ADDR(tf.tf_cs, tf.tf_ip);

    	if (tmode) {
		if (*(u_short *)addr != 0x03cd)
			resettrace();
	}

    	seg = tf.tf_ds;
    	rep = 1;

	debug (D_TRAPS2, "%04x:%04x ", tf.tf_cs, tf.tf_ip);
	switch (*(u_char *)addr) {
	case CLI:
		debug (D_TRAPS2, "cli\n");
		int_state = 0;
		tf.tf_ip++;
		break;
	case STI:
		debug (D_TRAPS2, "sti\n");
		int_state = PSL_I;
		tf.tf_ip++;
		break;
	case PUSHF:
		debug (D_TRAPS2, "pushf\n");
		PUSH((tf.tf_eflags & ~PSL_I) | int_state | PSL_IOPL, &tf);
		tf.tf_ip++;
		break;
	case POPF:
		debug (D_TRAPS2, "popf\n");

		int_state = POP(&tf);
		okflags = PSL_ALLCC | PSL_T | PSL_D | PSL_V;

		tf.tf_eflags = (tf.tf_eflags & ~okflags)
			| (int_state & okflags);

		tf.tf_eflags &= ~PSL_IOPL;
		tf.tf_eflags |= PSL_I;

		int_state &= PSL_I;
		tf.tf_ip++;
		break;
	tracetrap:
    	case TRACETRAP:
		if (!tmode) {
			if (vflag) dump_regs(&tf);
			fatal ("Executing a trace trap\n");
		}
		resettrace();
		break;
	case INTn:
		intnum = *(u_char *)(addr + 1);

		if (gettingsda)
		    ++gettingsda;

		if (intnum == 3)
			goto tracetrap;
    	    	if (intnum == 0x2f && (tf.tf_ax & 0xFF00) == 0x1100) {
			debug (D_TRAPS|0x2f, "INT 2F:%04x\n", tf.tf_ax);
			if (int2f_11(&tf)) {
				tf.tf_ip += 2;	/* Skip over int 2f:11 */
				break;
			}
		}
		fake_int(&tf, intnum);
		break;
	case IRET:
    	    	if (gettingsda && --gettingsda == 0) {
		    u_long sda = (tf.tf_ds << 16) | tf.tf_si;
		    tf = sda_trapframe;
    	    	    (*sdafunc)(&tf, sda);
		    tf.tf_ip += 2;
    	    	} else {
		    tf.tf_ip = POP(&tf);
		    tf.tf_cs = POP(&tf);
		    int_state = POP(&tf);

		    debug (D_TRAPS2, "iret to %04x:%04x\n",  tf.tf_cs, tf.tf_ip);

		    okflags = PSL_ALLCC | PSL_T | PSL_D | PSL_V;

		    tf.tf_eflags = (tf.tf_eflags & ~okflags)
			    | (int_state & okflags);

		    tf.tf_eflags &= ~PSL_IOPL;
		    tf.tf_eflags |= PSL_I;

		    int_state &= PSL_I;
    	    	}
		break;
	case INd:
		port = ((u_char *)addr)[1];
		inb(&tf, port);
		tf.tf_ip += 2;
		break;
	case OUTd:
		port = ((u_char *)addr)[1];
		outb(&tf, port);
		tf.tf_ip += 2;
		break;
	case INdX:
		port = ((u_char *)addr)[1];
		debug(D_PORT, "direct INx on port %02x\n", port);
		inx(&tf, port);
		tf.tf_ip += 2;
		break;
	case OUTdX:
		port = ((u_char *)addr)[1];
		debug(D_PORT, "direct OUTx on port %02x\n", port);
		outx(&tf, port);
		tf.tf_ip += 2;
		break;
	case IN:
	    	inb(&tf, tf.tf_dx);
		tf.tf_ip += 1;
		break;
	case INX:
		inx(&tf, tf.tf_dx);
		tf.tf_ip += 1;
	    	break;
	case OUT:
	    	outb(&tf, tf.tf_dx);
		tf.tf_ip += 1;
		break;
	case OUTX:
	    	outx(&tf, tf.tf_dx);
		tf.tf_ip += 1;
		break;
	case OUTSB:
    	    	while (rep-- > 0)
		    	outsb(&tf, tf.tf_dx & 0x3ff);
		tf.tf_ip += 1;
		break;

	case OUTSW:
    	    	while (rep-- > 0)
		    	outsx(&tf, tf.tf_dx & 0x3ff);
		tf.tf_ip += 1;
		break;

	case INSB:
		daddr = MAKE_ADDR(tf.tf_es, tf.tf_di);
		while (rep-- > 0)
			insb(&tf, tf.tf_dx & 0x3ff);
		tf.tf_ip += 1;
		break;
	case INSW:
		while (rep-- > 0)
			insx(&tf, tf.tf_dx & 0x3ff);
		tf.tf_ip += 1;
		break;
    	case LOCK:
		debug(D_TRAPS2, "lock\n");
		tf.tf_ip += 1;
		break;
	default:
		dump_regs(&tf);
		fatal("default trap taken\n");
	unsupported:
		if (vflag) dump_regs(&tf);
		fatal("Unsupported instruction\n");
	}


    	if (tmode) {
		tracetrap(&tf);
	}

	switch_vm86 (sf, tf);
}

void
sigtrap(struct sigframe sf, struct trapframe tf)
{   
	if ((sf.sf_sc.sc_ps & PSL_VM) == 0)
		fatal ("Sigtrap in protected mode\n");
	if (tf.tf_trapno == T_BPTFLT) {
		PUSH((tf.tf_eflags & (~PSL_I)) | int_state | PSL_IOPL, &tf);
		intnum = 3;  
	} else {
		PUSH((tf.tf_eflags & (~PSL_I)) | int_state | PSL_IOPL | PSL_T, &tf);
		intnum = 1;
	}

	PUSH(tf.tf_cs, &tf);
	PUSH(tf.tf_ip + 2, &tf);

	tf.tf_cs = ivec[intnum] >> 16;
	tf.tf_ip = ivec[intnum] & 0xffff;

	switch_vm86 (sf, tf);
}

void
breakpoint(struct sigframe sf, struct trapframe tf)
{
        if (sf.sf_sc.sc_ps & PSL_VM)
		printf("doscmd ");
	printf("breakpoint: %04x\n", *(u_short *)0x8e64);

        __asm__ volatile("mov 0, %eax");
        __asm__ volatile(".byte 0x0f");		/* MOV DR6,EAX */
        __asm__ volatile(".byte 0x21");
        __asm__ volatile(".byte 0x1b");

        if (sf.sf_sc.sc_ps & PSL_VM)
		switch_vm86 (sf, tf);
}


void
sigill(struct sigframe sf, struct trapframe tf)
{
	dump_regs(&tf);
	fatal("%04x:%04x Illegal instruction\n", tf.tf_cs, tf.tf_ip);
}

void
floating (struct sigframe sf, struct trapframe tf)
{
#if 0
        if (sf.sf_sc.sc_ps & PSL_VM) {
	    if ((ivec[0] >> 16) != 0xF000)
		fake_int(&tf, 0);
	    else
		printf("Divide by 0!\n");
	    switch_vm86 (sf, tf);
    	}
#endif
	if (vflag) dump_regs(&tf);
	fatal ("No floating point hardware and no 16 bit FP emulator\n");
}

void
random_sig_handler(struct sigframe sf, struct trapframe tf)
{
        int vm86mode;

        vm86mode = sf.sf_sc.sc_ps & PSL_VM;
        if (vm86mode) {
            if (vflag) ;
		dump_regs(&tf);
            fatal ("random signal %d from DOS program\n", sf.sf_signum);
        } else {
            fatal ("random signal %d from DOSCMD\n", sf.sf_signum);
        }
}

void
switch_vm86(struct sigframe sf, struct trapframe tf)
{
        if ((sf.sf_sc.sc_ps & PSL_VM) == 0
            || (tf.tf_eflags & PSL_VM) == 0) {
                fatal ("illegal switch_vm86");
        }

        __asm__ volatile("addl $8, %esp");      /* drop garbage from stack */
        __asm__ volatile("movl $103,%eax");     /* sigreturn syscall number */
        /* enter kernel with args on stack */
        __asm__ volatile(".byte 0x9a; .long 0; .word 0x7");

        fatal ("sigreturn for v86 mode failed\n");
}
