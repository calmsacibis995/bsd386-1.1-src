/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fpe_main.c,v 1.10 1993/10/31 23:28:49 karels Exp $
 */
 
/*
 * 80387 subset emulator (no transcendental functions)
 *
 * Entry, exit, exceptions
 *
 */

#include "param.h"
#include "machine/reg.h"
#include "machine/frame.h"
#include "machine/pcb.h"
#include "machine/psl.h"
#include "fpe.h"
#include "fp_reg.h"
#include "systm.h"

int fpedna  __P((struct trapframe *));
int (*dnatrap)  __P((struct trapframe *)) = fpedna;

int fp_set_addr32 __P((struct fp_em *fp_emp, char *eap, int seg_override));
int fp_set_addr16 __P((struct fp_em *fp_emp, char *eap, int seg_override));
 
/*
 * Implement device not available (DNA) exception: entry to emulator.
 * ret: 0	- finished, no error
 *	SIGSEGV	- operand faulted
 */
fpedna(tfp)
	struct trapframe *tfp;
{
	struct i386_fp_save *ep = (struct i386_fp_save *) &curpcb->pcb_savefpu;
	unsigned eip;
	int cmd_len;
	struct cmd cmd;
	struct fp_em fp_em, *fp_emp;
	int addr32; 	/* 32 bit address in cmd */
	int data32; 	/* 32 bit offset/data in cmd */
	int seg_override;	/* segment override prefix was used */
	int error;
	register i;

	/*
	 * We don't handle 16-bit code (yet).
	 */
	if (tfp->tf_eflags & PSL_VM)
		return (SIGFPE);

	if ((curpcb->pcb_flags & FP_WASUSED) == 0) {
		curpcb->pcb_flags |= FP_WASUSED;

		ep->fp_control = FPC_DEFAULT;
		ep->fp_status = 0;
		ep->fp_tag = -1;

	}

#define ush(cmd) (*(unsigned short*)&cmd)

	fp_emp = &fp_em;
	fp_emp->exc = 0;
	fp_emp->inv = 0;
	fp_emp->ret = 0;
	fp_emp->outsize = 0;
	fp_emp->left_intact = 0;
	fp_emp->non_st_dst = 0;
	fp_emp->envp = ep;
	fp_emp->regsp = (int *) tfp;
	fp_emp->fpregsp = (void *)((char *)ep + sizeof(*ep));

loop:
	eip = reg_i(EIP);
	if ((i = fusword(seg_addr(reg_seg(CS), eip))) == -1)
		return (SIGSEGV);
	ush(cmd) = i;

	/*
	 * If there is an exception pending, and the new 
	 * instruction is not an FN* call that doesn't check
	 * for exceptions, return SIGFPE to report the exception
	 * now.  Like the current NPX code, we require the program
	 * to clear the exception if it wishes to continue.
	 */
	if ((env(fp_status) & FPS_ES) && ush(cmd) != FINIT &&
	    ush(cmd) != FCLEX && (ush(cmd) & FSTENV_MASK) != FSTENV &&
	    (ush(cmd) & FSAVE_MASK) != FSAVE)
		return (SIGFPE);
	 
	env(fp_eip) = eip;
	env(fp_cs) = reg_seg(CS);

	seg_override = 0;
	addr32 = 1;
	data32 = 1;

	cmd_len = 0;
	while (cmd.c_esc != C_ESC) {
		switch ((int) ush(cmd)) {

		case FWAIT: 
			reg_i(EIP) += cmd_len + 1;
			goto loop;

		case PFX_ADDR:
			addr32 = 0;
			break;
		case PFX_DATA:
			data32 = 0;
			break;
		case PFX_CS:
			env(fp_ds) = reg_seg(CS); 
			seg_override = 1; 
			break;
		case PFX_DS:
			env(fp_ds) = reg_seg(DS); 
			seg_override = 1; 
			break;
		case PFX_SS:
			env(fp_ds) = reg_seg(SS); 
			seg_override = 1; 
			break;
		case PFX_ES:
			env(fp_ds) = reg_seg(ES); 
			seg_override = 1; 
			break;
		case PFX_FS:
			env(fp_ds) = reg_seg(FS); 
			seg_override = 1; 
			break;
		case PFX_GS:
			env(fp_ds) = reg_seg(GS); 
			seg_override = 1; 
		case PFX_LOCK:
			break;
		default:
			/*
			 * 80387 call on not 80387 instruction
			 */
			return (0);
		}
		++cmd_len;
		if ((i = fusword(seg_addr(reg_seg(CS), eip + cmd_len))) == -1)
			return (SIGSEGV);
		ush(cmd) = i;
	}

	env(fp_opcode) = ush(cmd);

	fp_emp->oldtop = 
	    fp_emp->top = (int)(env(fp_status) & FPS_TOS) >> FPS_TOS_SHIFT;
	fp_emp->rm = cmd.c_rm;
	cmd_len +=2;	/* 2 bytes for fp command */

	if (cmd.c_mod == 3) {
		dprintf(("%s ", fp_tblm3[cmd.c_opa].fp_cmd));
		(*fp_tblm3[cmd.c_opa].fp_fun)(fp_emp, cmd);
	} else {
		env(fp_dp) = 0;
		if ((i = (addr32 ? fp_set_addr32 : fp_set_addr16)
		    (fp_emp, (char *)eip + cmd_len, seg_override)) == -1)
			return (SIGSEGV);
		cmd_len += i;
		dprintf(("%s ",
		    fp_tblmod[(cmd.c_opa << 3) + cmd.c_opb].fp_cmd));
		(*fp_tblmod[(cmd.c_opa << 3) + cmd.c_opb].fp_fun)(fp_emp);
	}

	fp_set_env(fp_emp);
	if (fp_emp->outsize && (error = copyout(fp_emp->membuf, ADDR(fp_emp),
	    fp_emp->outsize)) != 0)
		fp_emp->ret = error;
	/*
	 * Unmasked exceptions are reported at the start of the next FP
	 * instruction; unimplemented instructions and faults are reported now.
	 */
	switch (fp_emp->ret) {
	case 0:
		reg_i(EIP) += cmd_len;
		return (0);
	case EFAULT:
		return (SIGSEGV);
	case EOPNOTSUPP:
		return (SIGFPE);
	case EINVAL:
		return (SIGILL);
	default:
		panic("fpe bad ret");
	}
#undef 	ush
}

fp_set_env(fp_emp)
	struct fp_em *fp_emp;
{

	if (fp_emp->oldtop != fp_emp->top) {
		env(fp_status) &= ~FPS_TOS;
		env(fp_status) |= fp_emp->top << FPS_TOS_SHIFT;
	}
}

/*
 * Address calculation
 */
 
#define	seg_regpfx(sg)  (seg_override ? env(fp_ds) : reg_seg(sg))

/*
 * Calculate effective address for 16 bit addressing scheme
 * ret: number of bytes in sib and displacement; -1 on error
 * arg:	eap : effective address pointer - points to next after mod r/m byte
 */
const static int rmregs16[] = {BX, BX, BP, BP, SI, DI, BP, BX};

int fp_set_addr16(fp_emp, eap, seg_override)
	struct fp_em *fp_emp;
	register char *eap;
	int seg_override;
{
	register rm = fp_emp->rm;
	register mod = (*(struct cmd*)&(env(fp_opcode))).c_mod;
	int i;

	if (rmregs16[rm] != BP || (mod == 0 && rm == 6))
		env(fp_ds) = seg_regpfx(DS);
	else
		env(fp_ds) = seg_regpfx(SS);

	if (mod == 0 && rm == 6) {
		if ((i = fusword(seg_addr(reg_seg(CS), eap))) == -1)
		 	return (-1);
		env(fp_dp) += (short) i;	/* sign extend */
		return (2);
	} else if (mod == 1) {
		if ((i = fubyte(seg_addr(reg_seg(CS), eap))) == -1)
		 	return (-1);
		env(fp_dp) += (char) i;		/* sign extend */
	} else { /* mod == 2 */
		if ((i = fusword(seg_addr(reg_seg(CS), eap))) == -1)
		 	return (-1);
		env(fp_dp) += (short) i;	/* sign extend */
	}

	env(fp_dp) += reg_s(rmregs16[rm]);

	if (rm == 0 || rm == 2)
		env(fp_dp) += reg_s(SI);
	else if (rm == 1 || rm == 3)
		env(fp_dp) += reg_s(DI);
	return (mod);
}

/*
 * Calculate effective address for 32-bit addressing mode
 * ret: number of bytes in sib and displacement; -1 on error
 * arg:	eap : effective addres pointer - points to next after mod r/m byte
 */
const static int rmregs32[] = {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI};

int fp_set_addr32(fp_emp, eap, seg_override)
	struct fp_em *fp_emp;
	register char *eap;
	int seg_override;
{
	register rm = fp_emp->rm;
	int mod, i;

	if (rm == 4)
		return (fp_calc_sib(fp_emp, eap, seg_override));

	mod = ((struct cmd*)&(env(fp_opcode)))->c_mod;
	env(fp_ds) = seg_regpfx(DS);
	switch (mod) {
	case 0:
		if (rm == 5) {
			if (copyin((caddr_t) seg_addr(reg_seg(CS), eap), &i,
			    sizeof(int)) != 0)
			    	return (-1);
			env(fp_dp) += i;
			return (4);
		} else {
			env(fp_dp) += reg_i(rmregs32[rm]);
			return (0);
		}
	case 1:
		if (rm == 5)
			env(fp_ds) = seg_regpfx(SS);
		if ((i = fubyte(seg_addr(reg_seg(CS), eap))) == -1)
			return (-1);
		env(fp_dp) += reg_i(rmregs32[rm]) + (char) i; /* sign extend */
		return (1);
	case 2:
		if (rm == 5)
			env(fp_ds) = seg_regpfx(SS);
		if (copyin((caddr_t) seg_addr(reg_seg(CS), eap), &i,
		    sizeof(int)) != 0)
			return (-1);
		env(fp_dp) += reg_i(rmregs32[rm]) + i;
		return (4);
	}
}

/*
 * calculate address according to s-i-b field for 32-bit addressing mode
 * return number of bytes in sib and displacement, or -1 on error
 */
int fp_calc_sib(fp_emp, eap, seg_override)
	struct fp_em *fp_emp;
	register char *eap;
	int seg_override;
{
	struct sib sib;
	int base;
	int mod, i;

	if ((i = fubyte(seg_addr(reg_seg(CS), eap))) == -1)
		return (-1);
	*(unsigned char *)&sib = i;
	base = sib.base;
	mod = ((struct cmd*)&(env(fp_opcode)))->c_mod;
	if (base == 5 && mod == 0) {
		env(fp_ds) = seg_regpfx(DS);
		if (copyin((caddr_t) seg_addr(reg_seg(CS), eap + 1), &i,
		    sizeof(int)) != 0)
			return (-1);
		env(fp_dp) += i;
		if (sib.index != 4)
			env(fp_dp) += reg_i(rmregs32[sib.index]) << sib.ss;
		return (5);
	}
	if (base == 4 || base == 5)
		env(fp_ds) = seg_regpfx(SS);
	else 
		env(fp_ds) = seg_regpfx(DS);
	env(fp_dp) += reg_i(rmregs32[base]);

	if (sib.index != 4)
		env(fp_dp) += reg_i(rmregs32[sib.index]) << sib.ss;

	if (mod == 0)
		return (1);
	else if (mod == 1) {
		if ((i = fubyte(seg_addr(reg_seg(CS), eap + 1))) == -1)
			return (-1);
		env(fp_dp) += (char) i;		/* sign extend */
		return (2);
	} else { /* mod == 2 */
		if (copyin((caddr_t) seg_addr(reg_seg(CS), eap + 1), &i,
		    sizeof(int)) != 0)
			return (-1);
		env(fp_dp) += i;
		return (5);
	}
}

/*
 * allocate (for push) and free (pop) floating point stack
 */
fp_ST_alloc(fp_emp)
	struct fp_em *fp_emp;
{
	register i = ST(7);

	dprintf(("%dv", fp_emp->top));
	if (fp_empty(i)) {
		TAGset(i, t_valid);
		fp_emp->top = i;
		return (1);
	} else {
		fp_exc_stack(fp_emp, overflow);
		CHECK_UNMASKED_EXC;
		TAGset(i, t_valid);
		fp_emp->top = i;
		return (0);
	}
}

fp_ST_pop(fp_emp)
	struct fp_em *fp_emp;
{

	dprintf(("%d%c", fp_emp->top, fp_left_intact(fp_emp) ? '-':'^'));
	if (fp_left_intact(fp_emp))
		return;
	TAGset(ST0, t_empty);
	fp_emp->top = ST(1);
}

/*
 * floating point exceptions
 * if exception is masked, continue
 * else call fp_exc_unmasked(fp_emp) and return;
 */

fp_exc_stack(fp_emp, ovr)
	struct fp_em *fp_emp;
	int ovr;
{

	env(fp_status) |= FPS_IE | FPS_SF;
	if (ovr == overflow) 
		env(fp_status) |= FPS_C1;
	else
		env(fp_status) &= ~FPS_C1;
	if (!(env(fp_control) & FPC_IE))
		fp_exc_unmasked(fp_emp);
	else
		fp_emp->acc0 = fp_QNaN_Indefinite_a;
}

fp_exc_inv_op(fp_emp, set_acc)
	struct fp_em *fp_emp;
	int set_acc;
{

	env(fp_status) |= FPS_IE;
	if (!(env(fp_control) & FPC_IE))
		fp_exc_unmasked(fp_emp);
	else if (set_acc == set_indefinite)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
}

fp_exc_denormal(fp_emp)
	struct fp_em *fp_emp;
{

	env(fp_status) |= FPS_DE;
	if (!(env(fp_control) & FPC_DE))
		fp_exc_unmasked(fp_emp);
}

fp_exc_zerodivide(fp_emp)
	struct fp_em *fp_emp;
{

	env(fp_status) |= FPS_ZE;
	if (!(env(fp_control) & FPC_ZE))
		fp_exc_unmasked(fp_emp);
	else {
		fp_emp->acc0.exp = fp_QNaN_Infinity_a.exp;
		fp_emp->acc0.mant0 = fp_QNaN_Infinity_a.mant0;
		fp_emp->acc0.mant = fp_QNaN_Infinity_a.mant;
		/* do not touch fp_emp->acc0.sign */
	}
}

fp_exc_overflow(fp_emp, ap)
	struct fp_em *fp_emp;
	register fp_a *ap;
{

	env(fp_status) |= FPS_OE;
	if (!(env(fp_control) & FPC_OE)) {
		if (fp_emp->non_st_dst)
			fp_emp->left_intact = 1;
		else
			ap->exp -= FP_EXC_BIAS;
		fp_exc_unmasked(fp_emp);
	}
}

fp_exc_underflow(fp_emp, ap)
	struct fp_em *fp_emp;
	register fp_a *ap;
{

	env(fp_status) |= FPS_UE;
	if (!(env(fp_control) & FPC_UE)) {
		if (fp_emp->non_st_dst)
			fp_emp->left_intact = 1;
		else
			ap->exp += FP_EXC_BIAS;
		fp_exc_unmasked(fp_emp);
	}
}

fp_exc_inexact(fp_emp, round_dir)
	struct fp_em *fp_emp;
{

	env(fp_status) |= FPS_PE;
	if (round_dir == inexact_up)
		env(fp_status) |= FPS_C1;
	else
		env(fp_status) &= ~FPS_C1;

	if (!(env(fp_control) & FPC_PE))
		fp_exc_unmasked(fp_emp);
}
