/*
 * Copyright (c) 1991, 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fpe_cmd.c,v 1.11 1993/03/04 21:07:26 karels Exp $
 */

/*
 * 80387 subset emulator (no transcendental functions)
 *
 * commands
 */

#include "param.h"
#include "machine/reg.h"
#include "fpe.h"
#include "fp_reg.h"


#define	LATER	{fp_unsup(fp_emp);}

fp_a fp_QNaN_Indefinite_a={0, HI_BIT_a|HI_QUIET, FP_EXP_MAX_a, -1};
fp_a fp_QNaN_Infinity_a	= {0, HI_BIT_a, FP_EXP_MAX_a, -1};
fp_a fp_One_a 		= {0, HI_BIT_a, FP_EXP_BIAS_a, 0};
fp_a fp_Zero_a 		= {0, 0, 0, 0};
fp_a fp_Log2e_a 	= {0x5C17F0BC, 0xB8AA3B29, FP_EXP_BIAS_a + 1, 0};
fp_a fp_Log2ten_a 	= {0xCD1B8AFE, 0xD49A784B, FP_EXP_BIAS_a + 1, 0};
fp_a fp_Log10two_a 	= {0xFBCFF799, 0x9A209A84, FP_EXP_BIAS_a - 2, 0};
fp_a fp_Ln2_a		= {0xD1CF79AC, 0xB17217F7, FP_EXP_BIAS_a - 1, 0};
fp_a fp_Pi_a		= {0x2168C235, 0xC90FDAA2, FP_EXP_BIAS_a + 1, 0};

int fp_calc_tags __P((struct fp_em *fp_emp));
void fp_add_aa __P((struct fp_em *fp_emp));
void fp_sub_aa __P((struct fp_em *fp_emp));
void fp_div_aa __P((struct fp_em *fp_emp));
void fp_mul_aa __P((struct fp_em *fp_emp));
enum fp_class fp_what_class __P((fp_a *ap));
enum fp_class fp_check	__P((struct fp_em *fp_emp, fp_a *ap, int arg));
void fp_binop __P((struct fp_em *fp_emp, void (*binop)(), int sr, void (*s2a)(),
		int dr, void (*d2a)(),int rr));
void fp_ldop __P((struct fp_em *fp_emp, int sr, void (*s2a)()));
void fp_ldop_e __P((struct fp_em *fp_emp, int sr));
void fp_stop __P((struct fp_em *fp_emp, int dr, void (*a2d)()));
void fp_stop_e __P((struct fp_em *fp_emp, int dr));
void fp_ld_const __P((struct fp_em *fp_emp, fp_a *cp));
void fp_comop __P((struct fp_em *fp_emp, int sr, void (*s2a)()));

/*
 * FP instructions
 */

fp_2XM1(fp_emp)
	struct fp_em *fp_emp;
		LATER

fp_ABS(fp_emp)
	struct fp_em *fp_emp;
{
	fpregp(ST0)->exp &=~HI_BIT_s; /* set sign to zero */
}

fp_ADD(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_add_aa, STI, fp_e2a, ST0, fp_e2a, ST0);
}

fp_ADDPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_add_aa, STI, fp_e2a, ST0, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_ADD_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_add_aa, NON_ST, fp_d2a, ST0, fp_e2a, ST0);
}

fp_ADD_f(fp_emp)
struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_binop(fp_emp, fp_add_aa, NON_ST, fp_f2a, ST0, fp_e2a, ST0);
}

fp_ADDr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_add_aa, STI, fp_e2a, ST0, fp_e2a, STI);
}

fp_BLD(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, fp_b))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_b2a);
}

fp_BSTP(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, fp_b);
	fp_stop(fp_emp, NON_ST, fp_a2b); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_CHS(fp_emp)
	struct fp_em *fp_emp;
{
	fpregp(ST0)->exp ^= HI_BIT_s;
}

fp_CLEX(fp_emp)
	struct fp_em *fp_emp;
{
	env(fp_status) &= ~(FPS_IE | FPS_DE | FPS_ZE | FPS_OE |
			FPS_UE | FPS_PE | FPS_SF | FPS_ES);
}

fp_COM(fp_emp)
	struct fp_em *fp_emp;
{
	fp_comop(fp_emp, STI, fp_e2a);
}

fp_COMP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_comop(fp_emp, STI, fp_e2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_COMPP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_comop(fp_emp, STI, fp_e2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_COMP_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_d2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_COMP_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_f2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_COM_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_d2a);
}

fp_COM_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_f2a);
}

fp_COS(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
fp_DECSTP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_emp->top = ST(7);
}

fp_DISI(fp_emp)
	struct fp_em *fp_emp;
{	
	/* nop on 80387 */
}

fp_DIV(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, STI, fp_e2a, ST0, fp_e2a, ST0);
}

fp_DIVPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, STI, fp_e2a, ST0, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_DIVR(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, STI, fp_e2a, ST0);
}

fp_DIVRPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, STI, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_DIVR_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, NON_ST, fp_d2a, ST0);
}

fp_DIVR_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, NON_ST, fp_f2a, ST0);
}

fp_DIVRr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, STI, fp_e2a, STI);
}

fp_DIV_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, NON_ST, fp_d2a, ST0, fp_e2a, ST0);
}

fp_DIV_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, NON_ST, fp_f2a, ST0, fp_e2a, ST0);
}

fp_DIVr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_div_aa, STI, fp_e2a, ST0, fp_e2a, STI);
}

fp_ENI(fp_emp)
	struct fp_em *fp_emp;
{
	/* nop on 80387 */
}

fp_FREE(fp_emp)
	struct fp_em *fp_emp;
{
	TAGset(STI, t_empty);
}

fp_IADD_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_add_aa, NON_ST, fp_i2a, ST0, fp_e2a, ST0);
}

fp_IADD_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_binop(fp_emp, fp_add_aa, NON_ST, fp_s2a, ST0, fp_e2a, ST0);
}

fp_ICOMP_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_i2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_ICOMP_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_s2a); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_ICOM_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_i2a);
}

fp_ICOM_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_comop(fp_emp, NON_ST, fp_s2a);
}

fp_IDIVR_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, NON_ST, fp_i2a, ST0);
}

fp_IDIVR_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, ST0, fp_e2a, NON_ST, fp_s2a, ST0);
}

fp_IDIV_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, NON_ST, fp_i2a, ST0, fp_e2a, ST0);
}

fp_IDIV_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_binop(fp_emp, fp_div_aa, NON_ST, fp_s2a, ST0, fp_e2a, ST0);
}

fp_ILD_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_i2a);
}

fp_ILD_q(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, fp_q))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_q2a);
}

fp_ILD_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_s2a);
}

fp_IMUL_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_mul_aa, NON_ST, fp_i2a, ST0, fp_e2a, ST0);
}

fp_IMUL_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_mul_aa, NON_ST, fp_i2a, ST0, fp_e2a, ST0);
}

fp_INCSTP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_emp->top = ST(1);
}

fp_INIT(fp_emp)
	struct fp_em *fp_emp;
{
	env(fp_control) = 0x037f;
	env(fp_status)= 0;
	env(fp_tag) = -1;
}

fp_ISTP_i(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, int);
	fp_stop(fp_emp, NON_ST, fp_a2i); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_ISTP_q(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, fp_q);
	fp_stop(fp_emp, NON_ST, fp_a2q); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_ISTP_s(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, short);
	fp_stop(fp_emp, NON_ST, fp_a2s); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_IST_i(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, int);
	fp_stop(fp_emp, NON_ST, fp_a2i);
}

fp_IST_s(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, short);
	fp_stop(fp_emp, NON_ST, fp_a2s);
}

fp_ISUBR_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, NON_ST, fp_i2a, ST0);
}

fp_ISUBR_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, NON_ST, fp_s2a, ST0);
}

fp_ISUB_i(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, int))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, NON_ST, fp_i2a, ST0, fp_e2a, ST0);
}

fp_ISUB_s(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, short))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, NON_ST, fp_s2a, ST0, fp_e2a, ST0);
}

fp_LD(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ldop_e(fp_emp, STI);
}

fp_LD1(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_One_a);
}

fp_LDCW(fp_emp)
	struct fp_em *fp_emp;
{

	RETURN (copyin(ADDR(fp_emp), &fp_emp->envp->fp_control,
	    sizeof(fp_emp->envp->fp_control)));
}

fp_LDENV(fp_emp)
	struct fp_em *fp_emp;
{

	RETURN (copyin(ADDR(fp_emp), fp_emp->envp,
	    sizeof(struct i386_fp_save)));
}

fp_LDL2E(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Log2e_a);
}

fp_LDL2T(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Log2ten_a);
}

fp_LDLG2(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Log10two_a);
}

fp_LDLN2(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Ln2_a);
}

fp_LDPI(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Pi_a);
}

fp_LDZ(fp_emp)
	struct fp_em *fp_emp;
{
	fp_ld_const(fp_emp, &fp_Zero_a);
}

fp_LD_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_d2a);
}

fp_LD_e(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, fp_e))
		RETURN (error);
	fp_ldop_e(fp_emp, NON_ST);
}

fp_LD_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_ldop(fp_emp, NON_ST, fp_f2a);
}

fp_MUL(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_mul_aa, STI, fp_e2a, ST0, fp_e2a, ST0);
}

fp_MULPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_mul_aa, STI, fp_e2a, ST0, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_MUL_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_mul_aa, NON_ST, fp_d2a, ST0, fp_e2a, ST0);
}

fp_MUL_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_binop(fp_emp, fp_mul_aa, NON_ST, fp_f2a, ST0, fp_e2a, ST0);
}

fp_MULr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_mul_aa, STI, fp_e2a, ST0, fp_e2a, STI);
}

fp_NOP(fp_emp)
	struct fp_em *fp_emp;
{
	/* nop is nop is nop */
}

fp_PATAN(fp_emp)
	struct fp_em *fp_emp;
		LATER
	 
fp_PREM(fp_emp)
	struct fp_em *fp_emp;
{
	fp_prem(fp_emp, 0);
}

fp_PREM1(fp_emp)
	struct fp_em *fp_emp;
{
	fp_prem(fp_emp, 1);
}

fp_PTAN(fp_emp)
	struct fp_em *fp_emp;
		LATER

fp_RNDINT(fp_emp)
	struct fp_em *fp_emp;
	
{
	fp_q q;

	if (fp_empty(ST0)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		if (fpregp(ST0)->exp >= FP_EXP_BIAS_a + BITS(fp_q) - 1)
			return;
		fp_e2a(fpregp(ST0), &fp_emp->acc0);
		fp_a2q(fp_emp, &fp_emp->acc0, &q);
		CHECK_UNMASKED_EXC;
		fp_q2a(&q, &fp_emp->acc0);
	}
	if (!fp_left_intact(fp_emp))
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

struct fullenv {
	struct i386_fp_save	env;	
	struct fp_fpregs	fregs;
};	

fp_RSTOR(fp_emp)
	struct fp_em *fp_emp;
	
{

	RETURN (copyin(ADDR(fp_emp), fp_emp->envp, sizeof(struct fullenv)));
}

fp_SAVE(fp_emp)
	struct fp_em *fp_emp;
{

	fp_emp->envp->fp_tag = fp_calc_tags(fp_emp);
	RETURN (copyout(fp_emp->envp, ADDR(fp_emp), sizeof(struct fullenv)));
}


fp_SCALE(fp_emp)
	struct fp_em *fp_emp;
{
	if (fp_empty(ST0) || fp_empty(ST(1))) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		fp_e2a(fpregp(ST(1)), &fp_emp->acc0);
		fp_check(fp_emp, &fp_emp->acc0, 1);
		CHECK_UNMASKED_EXC;
		if ((fp_emp->inv) == 0) {
			int i;

			fp_a2i(fp_emp, &fp_emp->acc0, &i);
			CHECK_UNMASKED_EXC;
			fp_e2a(fpregp(ST0), &fp_emp->acc0);
			/*
			 * Don't scale if ST is zero; what about other classes?
			 * (this is probably wrong for infinities)
			 */
			if (fp_what_class(&fp_emp->acc0) != fp_zero)
				fp_emp->acc0.exp += i;
			fp_check(fp_emp, &fp_emp->acc0, 1);
			CHECK_UNMASKED_EXC;
		}
	}
	if (!fp_left_intact(fp_emp))
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

fp_SETPM(fp_emp)
	struct fp_em *fp_emp;
{
	/* NOP on 80387 */
}

fp_SIN(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
fp_SINCOS(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
fp_SQRT(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
fp_ST(fp_emp)
	struct fp_em *fp_emp;
{
	fp_stop_e(fp_emp, STI);
}

fp_STCW(fp_emp)
	struct fp_em *fp_emp;
{

	RETURN (copyout(&fp_emp->envp->fp_control, ADDR(fp_emp),
	    sizeof(short)));
}

fp_STENV(fp_emp)
	struct fp_em *fp_emp;
{
	
	fp_emp->envp->fp_tag = fp_calc_tags(fp_emp);
	RETURN (copyout(ADDR(fp_emp), fp_emp->envp,
	    sizeof(struct i386_fp_save)));
}

fp_STP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_stop_e(fp_emp, STI); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_STP_d(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, double);
	fp_stop(fp_emp, NON_ST, fp_a2d); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_STP_e(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, fp_e);
	fp_stop_e(fp_emp, NON_ST); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_STP_f(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, float);
	fp_stop(fp_emp, NON_ST, fp_a2f); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_STSW(fp_emp)
	struct fp_em *fp_emp;
{

	RETURN (copyout(&fp_emp->envp->fp_status, ADDR(fp_emp),
	    sizeof(fp_emp->envp->fp_status)));
}

fp_STSW_AX(fp_emp)
	struct fp_em *fp_emp;
{
	reg_s(AX) = env(fp_status);
}

fp_ST_d(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, double);
	fp_stop(fp_emp, NON_ST, fp_a2d);
}

fp_ST_f(fp_emp)
	struct fp_em *fp_emp;
{
	SET_MEM_DST(fp_emp, float);
	fp_stop(fp_emp, NON_ST, fp_a2f);
}

fp_SUB(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, STI, fp_e2a, ST0, fp_e2a, ST0);
}

fp_SUBPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, STI, fp_e2a, ST0, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_SUBR(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, STI, fp_e2a, ST0);
}

fp_SUBRPr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, STI, fp_e2a, STI);
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_SUBR_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, NON_ST, fp_d2a, ST0);
}

fp_SUBR_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, NON_ST, fp_f2a, ST0);
}

fp_SUBRr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, ST0, fp_e2a, STI, fp_e2a, STI);
}

fp_SUB_d(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, double))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, NON_ST, fp_d2a, ST0, fp_e2a, ST0);
}

fp_SUB_f(fp_emp)
	struct fp_em *fp_emp;
{
	int error;

	if (error = GET_MEM_SRC(fp_emp, float))
		RETURN (error);
	fp_binop(fp_emp, fp_sub_aa, NON_ST, fp_f2a, ST0, fp_e2a, ST0);
}

fp_SUBr(fp_emp)
	struct fp_em *fp_emp;
{
	fp_binop(fp_emp, fp_sub_aa, STI, fp_e2a, ST0, fp_e2a, STI);
}

fp_TST(fp_emp)
	struct fp_em *fp_emp;
{
	env(fp_status) |= (FPS_C3 | FPS_C2 | FPS_C0);
	if (fp_empty(ST0))
		fp_exc_stack(fp_emp, underflow);
	else {
		fp_e2a(fpregp(ST0), &fp_emp->acc0);
		fp_set_cc(fp_emp);
	}
}

fp_UCOM(fp_emp)
	struct fp_em *fp_emp;
{
	register ie = env(fp_control) & FPS_IE;

	env(fp_control) |= FPS_IE;
	fp_comop(fp_emp, STI, fp_e2a);
	env(fp_control) &= ~FPS_IE;
	env(fp_control) |= ie;
}

fp_UCOMP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_UCOM(fp_emp); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_UCOMPP(fp_emp)
	struct fp_em *fp_emp;
{
	fp_UCOM(fp_emp); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp); 
	CHECK_UNMASKED_EXC;
	fp_ST_pop(fp_emp);
}

fp_XAM(fp_emp)
	struct fp_em *fp_emp;
{
	if (fpregp(ST0)->exp & HI_BIT_s)
		env(fp_status) |= FPS_C1;
	if (fp_empty(ST0)) {
		env(fp_status) |= FPS_C3 | FPS_C0;
		return;
	}
	fp_e2a(fpregp(ST0), &fp_emp->acc0);
	switch (fp_what_class(&fp_emp->acc0)) {
	case fp_unsupported:
		return;
	case fp_signaling:
	case fp_quiet:
		env(fp_status) |= FPS_C0;
		return;
	case fp_normal:
		env(fp_status) |= FPS_C2;
		return;
	case fp_infinity:
		env(fp_status) |= FPS_C2 | FPS_C0;
		return;
	case fp_zero:
		env(fp_status) |= FPS_C3;
		return;
	case fp_denormal:
		env(fp_status) |= FPS_C3 | FPS_C2;
		return;
	}
}

fp_XCH(fp_emp)
	struct fp_em *fp_emp;
{
	fp_e e;
	register g, i = STI, t = ST0;

	if (fp_empty(i)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(i));
	}
	if (fp_empty(t)) {
		fp_exc_stack(underflow);
		CHECK_UNMASKED_EXC;
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(t));
	}
	e = *fpregp(i);
	*fpregp(i) = *fpregp(t);
	*fpregp(t) = e;
	g = TAG(i);
	TAGset(i, TAG(t));
	TAGset(t, g);
}

fp_XTRACT(fp_emp)
	struct fp_em *fp_emp;
{
	int r;
	
	if (fp_empty(ST0)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	}	
	r = fp_ST_alloc();
	CHECK_UNMASKED_EXC;
	if (!r || fp_empty(ST0))
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
	else {
		short e = fpregp(ST0)->exp - FP_EXP_BIAS_e;
		fp_s2a(&e, &fp_emp->acc0);
		CHECK_UNMASKED_EXC;
		fp_check(fp_emp, &fp_emp->acc0, 1);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		*fpregp(ST0) = *fpregp(ST(1));
		fpregp(ST0)->exp = FP_EXP_BIAS_e;
	}
	if (!fp_left_intact(fp_emp))
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

fp_YL2X(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
fp_YL2XP1(fp_emp)
	struct fp_em *fp_emp;
		LATER
		
/*
 * Unimplemented instruction
 */
fp_unsup(fp_emp)
	struct fp_em *fp_emp;
{
#ifdef DEBUG
	printf("fp: unsupported op 0x%x at %x:%x\n",
		 env(fp_opcode), env(fp_cs), env(fp_eip));
#endif
	fp_emp->ret = EOPNOTSUPP;
}

/*
 * Reserved (illegal) instruction
 */
fp_reserved(fp_emp)
	struct fp_em *fp_emp;
{
	fp_emp->ret = EINVAL;
}

/*
 * arithmetic operations emulation
 */
/*
 * add acc0 and acc1
 * should CHECK_UNMASKEd_EXC after call to this function
 * should CHECK_INV_OP after call to this function or use fp_emp->inv
 */ 
void 
fp_add_aa(fp_emp)
	struct fp_em *fp_emp;
	
{
	register de;
	register t0, t1;
	int i;

	t0 = fp_check(fp_emp, &fp_emp->acc0, 1);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	t1 = fp_check(fp_emp, &fp_emp->acc1, 2);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;

	if (fp_emp->acc0.sign != fp_emp->acc1.sign) {
		fp_emp->acc1.sign = fp_emp->acc0.sign;
		fp_sub_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		return;
	}
	if (t0 == fp_infinity)
		return;
	if (t1 == fp_infinity) {
		fp_emp->acc0 = fp_emp->acc1;
		return;
	}
	if (t0 == fp_denormal || t1 == fp_denormal) {
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	de = fp_emp->acc0.exp - fp_emp->acc1.exp;
	if (de >= BITS_MANT_a) {
		if (t1 != fp_zero) {
			fp_exc_precision(fp_emp);
			CHECK_UNMASKED_EXC;
		}	
		return;
	}
	if  (-de >= BITS_MANT_a) {
		fp_emp->acc0 = fp_emp->acc1;
		if (t0 != fp_zero)
			fp_exc_precision(fp_emp);
		return;
	}
	if (de > 0)
		i = fp_shr_m(&fp_emp->acc1.mant, de);
	else if (de < 0) {
		fp_emp->acc0.exp = fp_emp->acc1.exp;
		i = fp_shr_m(&fp_emp->acc0.mant, - de);
	}
	if (i) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	if (i = fp_add_norm_mm(&fp_emp->acc0.mant, &fp_emp->acc1.mant)) {
		++fp_emp->acc0.exp;
		if (i & 1)
			fp_exc_precision(fp_emp);
	}
}

/*
 * substract acc1 from acc0
 * should CHECK_UNMASKEd_EXC after call to this function
 * should CHECK_INV_OP after call to this function or use fp_emp->inv
 */ 
void 
fp_sub_aa(fp_emp)
	struct fp_em *fp_emp;
{
	register de;
	register t0, t1;
	int i;

	t0 = fp_check(fp_emp, &fp_emp->acc0, 1);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	t1 = fp_check(fp_emp, &fp_emp->acc1, 2);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;

	if (fp_emp->acc0.sign != fp_emp->acc1.sign) {
		fp_emp->acc1.sign = fp_emp->acc0.sign;
		fp_add_aa(fp_emp);
		return;
	}
	if (t0 == fp_infinity) {
		if (t1 == fp_infinity)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
		return;
	}
	if (t1 == fp_infinity) {
		fp_emp->acc0 = fp_emp->acc1;
		fp_emp->acc0.sign ^= -1;
		return;
	}
	if (t0 == fp_denormal || t1 == fp_denormal) {
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	de = fp_emp->acc0.exp - fp_emp->acc1.exp;
	if (de >= BITS_MANT_a) {
		if (t1 != fp_zero)
			fp_exc_precision(fp_emp);
		return;
	}
	if  (-de >= BITS_MANT_a) {
		fp_emp->acc0 = fp_emp->acc1;
		fp_emp->acc0.sign ^= -1;
		if (t0 != fp_zero)
			fp_exc_precision(fp_emp);
		return;
	}
	if (de > 0)
		i = fp_shr_m(&fp_emp->acc1.mant, de);
	else if (de < 0) {
		fp_emp->acc0.exp = fp_emp->acc1.exp;
		i = fp_shr_m(&fp_emp->acc0.mant, - de);
	}
	if (i) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	if (i = fp_sub_mm(&fp_emp->acc0.mant, &fp_emp->acc1.mant)) {
		fp_emp->acc0.sign ^= -1;
		fp_chs_m(&fp_emp->acc0.mant);
	}
	fp_norm(&fp_emp->acc0);
}

/*
 * divide acc0 by acc1
 * should CHECK_UNMASKEd_EXC after call to this function
 * should CHECK_INV_OP after call to this function or use fp_emp->inv
 */ 
void 
fp_div_aa(fp_emp)
	struct fp_em *fp_emp;
{
	register t0, t1;

	t0 = fp_check(fp_emp, &fp_emp->acc0, 1);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	t1 = fp_check(fp_emp, &fp_emp->acc1, 2);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;

	if (t0 == fp_infinity) {
		if (t1 == fp_infinity)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
		return;
	}
	fp_emp->acc0.sign ^= fp_emp->acc1.sign;
	if (t1 == fp_zero) {
		if (t0 == fp_zero)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
		else
			fp_exc_zerodivide(fp_emp);
		return;
	}
	if (t0 == fp_zero)
		return;
	if (t0 == fp_denormal || t1 == fp_denormal) {
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	if (t0 == fp_denormal)
		fp_emp->acc0.exp -= fp_norm_m(&fp_emp->acc0.mant);
	if (t1 == fp_denormal)
		fp_emp->acc1.exp -= fp_norm_m(&fp_emp->acc0.mant);
	fp_emp->acc0.exp -= fp_emp->acc1.exp - FP_EXP_BIAS_a;

	if (fp_div_mm(&fp_emp->acc0.mant, &fp_emp->acc1.mant)) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	fp_finish_divmul(fp_emp);
}

/*
 * multiply acc0 and acc1
 * should CHECK_UNMASKEd_EXC after call to this function
 * should CHECK_INV_OP after call to this function or use fp_emp->inv
 */
void 
fp_mul_aa(fp_emp)
	struct fp_em *fp_emp;
{
	register t0, t1;
	int i;

	t0 = fp_check(fp_emp, &fp_emp->acc0, 1);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	t1 = fp_check(fp_emp, &fp_emp->acc1, 2);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;

	if (t0 == fp_infinity) {
		if (t1 == fp_zero)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
		return;
	}
	if (t1 == fp_infinity) {
		if (t0 == fp_zero)
			fp_emp->acc0 = fp_QNaN_Indefinite_a;
		return;
	}
	fp_emp->acc0.sign ^= fp_emp->acc1.sign;
	if (t0 == fp_zero)
		return;
	if (t1 == fp_zero) {
		fp_emp->acc0 = fp_Zero_a;
		return;
	}
	if (t0 == fp_denormal || t1 == fp_denormal) {
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	if (t0 == fp_denormal)
		fp_emp->acc0.exp -= fp_norm_m(&fp_emp->acc0.mant);
	if (t1 == fp_denormal)
		fp_emp->acc1.exp -= fp_norm_m(&fp_emp->acc0.mant);
	fp_emp->acc0.exp += fp_emp->acc1.exp - FP_EXP_BIAS_a;

	i = fp_mul_mm(&fp_emp->acc0.mant, &fp_emp->acc1.mant);
	if (i & 1)
		++ fp_emp->acc0.exp;
	if (i & 2) {
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
	}	
	fp_finish_divmul(fp_emp);
}

fp_finish_divmul(fp_emp)
	struct fp_em *fp_emp;
{

	if (!(fp_emp->acc0.mant0 & HI_BIT_i))
		fp_emp->acc0.exp -= fp_norm_m(&fp_emp->acc0.mant);

	if (fp_emp->acc0.exp < - BITS_MANT_a) {
		fp_exc_underflow(fp_emp, &fp_emp->acc0);
		CHECK_UNMASKED_EXC;
		fp_exc_precision(fp_emp);
		CHECK_UNMASKED_EXC;
		/* fp_emp->acc0 = fp_Zero_a; */
	} else if (fp_emp->acc0.exp < 0) {
		register i = - fp_emp->acc0.exp;
		fp_emp->acc0.exp = 0;
		fp_exc_denormal(fp_emp);
		CHECK_UNMASKED_EXC;
		if (fp_shr_m(&fp_emp->acc0.mant, i))
			fp_exc_precision(fp_emp);
	} else if (fp_emp->acc0.exp > FP_EXP_MAX_a)
		fp_exc_overflow(fp_emp, &fp_emp->acc0);
}

/*
 * check, class functions
 */
enum fp_class
fp_what_class(ap)
	register fp_a *ap;
{

	if (ap->exp == 0) {
		if (ap->mant == 0 && ap->mant0 == 0)
			return (fp_zero);
		else if (!(ap->mant0 & HI_BIT_a))
			return (fp_denormal);
		else /* pseudodenormal is normal */
			return (fp_normal);
	} else if (ap->exp >= FP_EXP_MAX_a) {
		if (ap->mant0 == HI_BIT_a && ap->mant == 0)
			return (fp_infinity);
		else if ((ap->mant0 & (HI_BIT_a|HI_QUIET)) == (HI_BIT_a|HI_QUIET))
			return (fp_quiet);
		else if (ap->mant0 & HI_BIT_a)
			return (fp_signaling);
		else
			return (fp_unsupported);
	} else if (ap->mant0 & HI_BIT_a)
		return (fp_normal);
	else
		return (fp_unsupported);
}

/*
 * Return the class of the given data.
 * NB: fp_check() sets fp_emp->inv if data is invalid operand
 * caller should CHECK_INV_OP or look at fp_emp->inv explicitly
 */
enum fp_class
fp_check(fp_emp, ap, arg)
	struct fp_em *fp_emp;
	register fp_a *ap;
	int arg;
{
	int c;

	switch (c = fp_what_class(ap)) {
	case fp_unsupported:
		fp_emp->inv = arg;
		fp_exc_inv_op(fp_emp, set_indefinite);
		break;
	case fp_signaling:
		ap->mant0 |= HI_QUIET;
		fp_exc_inv_op(fp_emp, 0);
		/* fall through */
	case fp_quiet:
		fp_emp->inv = arg;
		break;
	}
	return (c);
}

/*
 * operations
 */

#define adr(r)	((r) == NON_ST ? (void *) fp_emp->membuf : (void *) fpregp(r))

/*
 * binary operation
 * s - source, d - destination, r - result
 * ?p - pointer to data, ?r - fp register number or NON_ST
 * ?2a, a2? - pointer to function to convert from/to data to/from fp_a format
 * binop - pointer to function to execute binary operation on accumulators
 */
void
fp_binop(fp_emp, binop, src, s2a, dst, d2a, result)
	struct fp_em *fp_emp;
	void (*binop)(); 
	int src; 
	void (*s2a)();
	int dst; 
	void (*d2a)(); 
	int result;
{

	fp_emp->non_st_dst = (result == NON_ST);
	if ((src != NON_ST && fp_empty(src)) ||
	    (dst != NON_ST && fp_empty(dst))) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		(*d2a)(adr(dst), &fp_emp->acc0);
		(*s2a)(adr(src), &fp_emp->acc1);
		(*binop)(fp_emp);
		CHECK_UNMASKED_EXC;
		if (fp_emp->inv == 2)
			fp_emp->acc0 = fp_emp->acc1;
	}
	fp_a2e(fp_emp, &fp_emp->acc0, fpregp(result));
	TAGset(result, t_valid);	/* really not empty */
}

/*
 * load (really push) to stack
 */
void
fp_ldop(fp_emp, src, s2a)
	struct fp_em *fp_emp;
	int src;
	void (*s2a)();
{

	if (src != NON_ST && fp_empty(src)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		int r = fp_ST_alloc(fp_emp);
		CHECK_UNMASKED_EXC;
		if (r)
			(*s2a)(adr(src), &fp_emp->acc0);
	}		
	fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

void
fp_ldop_e(fp_emp, src)
	struct fp_em *fp_emp;
	int src;
{
	if (src != NON_ST && fp_empty(src)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		int r = fp_ST_alloc(fp_emp);
		CHECK_UNMASKED_EXC;
		if (r) {
			*fpregp(ST0) = *(fp_e *)adr(src);
			return;
		}	
	}
	fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

/*
 * store from top of stack (no pop!)
 */
void
fp_stop(fp_emp, dst, a2d)
	struct fp_em *fp_emp;
	int dst;
	void (*a2d)();
{
	fp_emp->non_st_dst = (dst == NON_ST);
	if (fp_empty(ST0)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		fp_e2a(fpregp(ST0), &fp_emp->acc0);
		if (dst != NON_ST)
			TAGset(dst, t_valid);	/* really not empty */
	}
	if (!fp_left_intact(fp_emp)) {
		(*a2d)(fp_emp, &fp_emp->acc0, adr(dst));
		CHECK_UNMASKED_EXC;
	}	
}

void
fp_stop_e(fp_emp, dst)
	struct fp_em *fp_emp;
	int dst;
{
	if (fp_empty(ST0)) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
	} else {
		*(fp_e *)adr(dst) = *fpregp(ST0);
		if (dst != NON_ST)
			TAGset(dst, t_valid);	/* really not empty */
	}
}

/*
 * Load constant
 */
void
fp_ld_const(fp_emp, cp)
	struct fp_em *fp_emp;
	fp_a *cp;
{
	int r = fp_ST_alloc(fp_emp);
	CHECK_UNMASKED_EXC;
	fp_a2e(fp_emp, r ? cp : &fp_emp->acc0, fpregp(ST0));
}

/*
 * compare and set flags
 */
void
fp_comop(fp_emp, src, s2a)
	struct fp_em *fp_emp;
	int src;
	void (*s2a)();
{

	env(fp_status) |= (FPS_C3 | FPS_C2 | FPS_C0);
	if (fp_empty(ST0) || (src != NON_ST && fp_empty(src))) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
		fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
	} else {
		fp_e2a(fpregp(ST0), &fp_emp->acc0);
		(*s2a)(adr(src), &fp_emp->acc1);
		fp_sub_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		if (fp_emp->inv == 2)
			fp_emp->acc0 = fp_emp->acc1;
		fp_set_cc(fp_emp);
	}
}

/*
 * set Condition Codes according to fp_emp->acc0
 */
fp_set_cc(fp_emp)
	struct fp_em *fp_emp;
{
#define	set(code) (env(fp_status) |= (code))

	env(fp_status) &=~(FPS_C3 | FPS_C2 | FPS_C0);
	if (fp_emp->acc0.exp == 0 && fp_emp->acc0.mant0 == 0 && fp_emp->acc0.mant == 0)
		set(FPS_C3);
	else if (fp_emp->acc0.exp == 0 || (fp_emp->acc0.mant0 & HI_BIT_a) == 0)
		set(FPS_C3 | FPS_C2 | FPS_C0);
	else if (fp_emp->acc0.sign)
		set(FPS_C0);
#undef set
}

fp_prem(fp_emp, to_nearest)
	struct fp_em *fp_emp;
	int to_nearest;
{
	int r;

	if (fp_empty(ST0) || fp_empty(ST(1))) {
		fp_exc_stack(fp_emp, underflow);
		CHECK_UNMASKED_EXC;
	} else {
		fp_e2a(fpregp(ST0), &fp_emp->acc0);
		fp_e2a(fpregp(ST(1)), &fp_emp->acc1);
		r =  fp_prem_aa(fp_emp, to_nearest);
		CHECK_UNMASKED_EXC;
		switch (fp_emp->inv) {
		case 0:	
			if (!r)
				env(fp_status) |= FPS_C2;
			else {
				env(fp_status) &=~FPS_C2;
				fp_prem_set_cc_1(fp_emp);
			}
			break;
		case 2:
			fp_emp->acc0 = fp_emp->acc1;
		}
	}
	fp_a2e(fp_emp, &fp_emp->acc0, fpregp(ST0));
}

/*
 * ret: 1 - complete reduction
 *	0 - incomplete
 * side-effects:
 *	fp_emp->acc0 - remainder
 *	fp_emp->acc1 - quotient
 */
fp_prem_aa(fp_emp, to_nearest)
	struct fp_em *fp_emp;
	int to_nearest;
{
	fp_a a0, a1, aq;
	int t0, t1, i, sign;

	t0 = fp_check(fp_emp, &fp_emp->acc0, 1);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	t1 = fp_check(fp_emp, &fp_emp->acc1, 2);
	CHECK_UNMASKED_EXC;
	CHECK_INV_OP;
	if (t0 == fp_infinity || t1 == fp_zero) {
		fp_emp->acc0 = fp_QNaN_Indefinite_a;
		env(fp_status) |= FPS_C2;
		return (0);
	}
	sign = fp_emp->acc0.sign;
	fp_emp->acc0.sign = fp_emp->acc1.sign = 0;
	aq = fp_Zero_a;
	a1 = fp_emp->acc1;
	for (i = 0; i < 64; i ++) {
		a0 = fp_emp->acc0;
		fp_sub_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		if (fp_emp->acc0.sign) {
			fp_emp->acc0 = a0;
			fp_emp->acc1 = aq;
			fp_emp->acc0.sign = sign;
			return (1);
		}
		fp_emp->acc0 = a0;
		fp_div_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		fp_prem_round(&fp_emp->acc0, to_nearest);
		aq = fp_emp->acc0;
		fp_mul_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		fp_emp->acc1 = fp_emp->acc0;
		fp_emp->acc0 = a0;
		fp_sub_aa(fp_emp);
		CHECK_UNMASKED_EXC;
		CHECK_INV_OP;
		fp_emp->acc1 = a1;
	}
	fp_emp->acc1 = aq;
	fp_emp->acc0.sign = sign;
	return (0);
}

/*
 * if (to_nearest) round to nearest
 * else chop
 */
fp_prem_round(ap, to_nearest)
	register fp_a *ap;
	int to_nearest;
{
	int re;

	re = ap->exp - FP_EXP_BIAS_a;
	if (re >= BITS_MANT_a)
		return;
	if (re < 0) {
		*ap = fp_Zero_a;
		return;
	}
	if (to_nearest) {
		/* index to 0.5 bit */
		int i = BITS_MANT_a - 1 - ap->exp + FP_EXP_BIAS_a - 1;
		if (i >= 0 && i < BITS_MANT_a)
			fp_round_nearest(ap, i);
	}
	fp_shr_m(&(ap->mant), BITS_MANT_a - 1 - re);
	fp_norm_m(&(ap->mant));
}

/*
 * assign quotient 3 least bits to Condition Codes according to fp_emp->acc1
 */
fp_prem_set_cc_1(fp_emp)
	struct fp_em *fp_emp;
{
#define	set(code) (env(fp_status) |= (code))
	int re, i;

	env(fp_status) &=~(FPS_C3 | FPS_C1 | FPS_C0);
	if (fp_emp->acc1.exp < FP_EXP_BIAS_a ||
	    fp_emp->acc1.exp >= FP_EXP_BIAS_a + BITS_MANT_a - 1 + 3)
		return;
	re = fp_emp->acc1.exp - FP_EXP_BIAS_a;
	if (re >= BITS_MANT_a - 1)
		i = fp_emp->acc1.mant << (re - BITS_MANT_a + 1);
	else {
		fp_shr_m(&fp_emp->acc1.mant, BITS_MANT_a - 1 - re);
		i = fp_emp->acc1.mant;
	}
	if (i & 1)
		set(FPS_C1);
	if (i & 2)
		set(FPS_C3);
	if (i & 4)
		set(FPS_C0);
#undef set
}

fp_calc_tags(fp_emp)
	struct fp_em *fp_emp;
{
	register fp_e *p;
	register g = 0;
#define setgtag(i, v)	(g |= ((v) << ((i) << 1)))
	register i;

	for (i = 0; i < NFPREGS; i ++) {
		if (fp_empty(i) == t_empty)
			setgtag(i, t_empty);
		p = fpregp(i);
		if (p->exp == 0 && p->mant == 0 && p->mant0 == 0)
			setgtag(i, t_zero);
		else if (p->exp < FP_EXP_MAX_e && (p->mant0 & HI_BIT_e))
			setgtag(i, t_valid);
		else
			setgtag(i, t_special);
	}
	return (g);
}

/*
 * Instruction codes' tables
 */

#ifdef FP_DEBUG
#define DEB(s)	, s
#else
#define DEB(s)
#endif

struct fp_tbl fp_tblmod[64] = {
			/* D8 single real: float */
	fp_ADD_f	DEB("addf"),
	fp_MUL_f	DEB("mulf"),
	fp_COM_f	DEB("comf"),
	fp_COMP_f	DEB("compf"),
	fp_SUB_f	DEB("subf"),
	fp_SUBR_f	DEB("subrf"),
	fp_DIV_f	DEB("divf"),
	fp_DIVR_f	DEB("divrf"),
			/* D9 */
	fp_LD_f		DEB("ldf"),
	fp_reserved	DEB("-r-"),
	fp_ST_f		DEB("stf"),
	fp_STP_f	DEB("stpf"),
	fp_LDENV	DEB("ldenv"),
	fp_LDCW		DEB("ldcw"),
	fp_STENV	DEB("stenv"),
	fp_STCW		DEB("stcw"),
			/* DA - short integer (32 bit): int */
	fp_IADD_i	DEB("iaddi"),
	fp_IMUL_i	DEB("imuli"),
	fp_ICOM_i	DEB("icomi"),
	fp_ICOMP_i	DEB("icompi"),
	fp_ISUB_i	DEB("isubi"),
	fp_ISUBR_i	DEB("isubri"),
	fp_IDIV_i	DEB("idivi"),
	fp_IDIVR_i	DEB("idivri"),
			/* DB */
	fp_ILD_i	DEB("ildi"),
	fp_reserved	DEB("-r-"),
	fp_IST_i	DEB("isti"),
	fp_ISTP_i	DEB("istpi"),
	fp_reserved	DEB("-r-"),
	fp_LD_e		DEB("lde"),	/* extended real - 80 bits */
	fp_reserved	DEB("-r-"),
	fp_STP_e	DEB("stpe"),
			/* DC double real: double*/
	fp_ADD_d	DEB("addd"),
	fp_MUL_d	DEB("muld"),
	fp_COM_d	DEB("comd"),
	fp_COMP_d	DEB("compd"),
	fp_SUB_d	DEB("subd"),
	fp_SUBR_d	DEB("subrd"),
	fp_DIV_d	DEB("divd"),
	fp_DIVR_d	DEB("divrd"),
			/* DD */
	fp_LD_d		DEB("ldd"),
	fp_reserved	DEB("-r-"),
	fp_ST_d		DEB("std"),
	fp_STP_d	DEB("stpd"),
	fp_RSTOR	DEB("rstor"),
	fp_reserved	DEB("-r-"),
	fp_SAVE		DEB("save"),
	fp_STSW		DEB("stsw"),
			/* DE - word integer (16 bit): short */
	fp_IADD_s	DEB("iadds"),
	fp_IMUL_s	DEB("imuls"),
	fp_ICOM_s	DEB("icoms"),
	fp_ICOMP_s	DEB("icomps"),
	fp_ISUB_s	DEB("isubs"),
	fp_ISUBR_s	DEB("isubrs"),
	fp_IDIV_s	DEB("idivs"),
	fp_IDIVR_s	DEB("idivrs"),
			/* DF */
	fp_ILD_s	DEB("ilds"),
	fp_reserved	DEB("-r-"),
	fp_IST_s	DEB("ists"),
	fp_ISTP_s	DEB("istps"),
	fp_BLD		DEB("bld"),
	fp_ILD_q	DEB("ildq"),
	fp_BSTP		DEB("bstp"),
	fp_ISTP_q	 DEB("istpq")
};

struct fp_tbl fp_tblD8m3[8] = {
	fp_ADD		DEB("add"),
	fp_MUL		DEB("mul"),
	fp_COM		DEB("com"),
	fp_COMP		DEB("comp"),
	fp_SUB		DEB("sub"),
	fp_SUBR		DEB("subr"),
	fp_DIV		DEB("div"),
	fp_DIVR		DEB("divr")
};

fp_D8m3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{
	dprintf(("%s ", fp_tblD8m3[cmd.c_opb].fp_cmd));
	fp_tblD8m3[cmd.c_opb].fp_fun(fp_emp);
}

struct fp_tbl fp_tblD9m34[32] = {
	fp_CHS		DEB("chs"),
	fp_ABS		DEB("abs"),
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-"),
	fp_TST		DEB("tst"),
	fp_XAM		DEB("xam"),
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-"),

	fp_LD1		DEB("ld1"),
	fp_LDL2T	DEB("ldl2t"),
	fp_LDL2E	DEB("ldl2e"),
	fp_LDPI		DEB("ldpi"),
	fp_LDLG2	DEB("ldlg2"),
	fp_LDLN2	DEB("ldln2"),
	fp_LDZ		DEB("ldz"),
	fp_reserved	DEB("-r-"),

	fp_2XM1		DEB("2xm1"),
	fp_YL2X		DEB("yl2x"),
	fp_PTAN		DEB("ptan"),
	fp_PATAN	DEB("patan"),
	fp_XTRACT	DEB("xtract"),
	fp_PREM1	DEB("prem1"),
	fp_DECSTP	DEB("decstp"),
	fp_INCSTP	DEB("incstp"),

	fp_PREM		DEB("prem"),
	fp_YL2XP1	DEB("yl2xp1"),
	fp_SQRT		DEB("sqrt"),
	fp_SINCOS	DEB("sincos"),
	fp_RNDINT	DEB("rndint"),
	fp_SCALE	DEB("scale"),
	fp_SIN		DEB("sin"),
	fp_COS		DEB("cos")
};

fp_D9m3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{
	register i = cmd.c_opb;

	if (i & 4) {
		dprintf(("%s ", fp_tblD9m34[((i & 3) << 3) + cmd.c_rm].fp_cmd));
		fp_tblD9m34[((i & 3) << 3) + cmd.c_rm].fp_fun(fp_emp);
	} else if (i == 0) {
		dprintf(("LD "));
		fp_LD(fp_emp);
	} else if (i == 1) {
		dprintf(("XCH "));
		fp_XCH(fp_emp);
	} else if (i == 2 && cmd.c_rm == 0) {
		dprintf(("NOP "));
		fp_NOP(fp_emp);
	} else 
		fp_reserved(fp_emp);
}

fp_DAm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	if (cmd.c_opb == 5 && cmd.c_rm == 1) {
		dprintf(("UCOMPP "));
		fp_UCOMPP(fp_emp);
	} else 
		fp_reserved(fp_emp);
}

struct fp_tbl fp_tblDBm34[8] = {
	fp_ENI		DEB("eni"),	/* NOP on 386/387 */
	fp_DISI		DEB("disi"),	/* NOP on 386/387 */
	fp_CLEX		DEB("clex"),
	fp_INIT		DEB("init"),
	fp_SETPM	DEB("setpm"),	/* NOP on 386/387 */
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-")
};

fp_DBm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	if (cmd.c_opb == 4) {
		dprintf(("%s ", fp_tblDBm34[cmd.c_rm].fp_cmd));
		fp_tblDBm34[cmd.c_rm].fp_fun(fp_emp);
	} else
		fp_reserved(fp_emp);
}

struct fp_tbl fp_tblDCm3[8] = { /* reverse: OP ST(I), ST */
	fp_ADDr		DEB("addr"),
	fp_MULr		DEB("mulr"),
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-"),
	fp_SUBr		DEB("subr"),		/* doc says DC E8+i */
	fp_SUBRr	DEB("subrr"),		/* doc says DC E0+i */
	fp_DIVr		DEB("divr"),		/* doc says DC F8+i */
	fp_DIVRr	DEB("divrr")		/* doc says DC F0+i */
};

fp_DCm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	dprintf(("%s ", fp_tblDCm3[cmd.c_opb].fp_cmd));
	fp_tblDCm3[cmd.c_opb].fp_fun(fp_emp);
}

struct fp_tbl fp_tblDDm3[8] = {
	fp_FREE		DEB("free"),
	fp_reserved	DEB("-r-"),
	fp_ST		DEB("st"),
	fp_STP		DEB("stp"),
	fp_UCOM		DEB("ucom"),
	fp_UCOMP	DEB("ucomp"),
	fp_reserved	DEB("-r-"),
	fp_reserved	DEB("-r-")
};

fp_DDm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	dprintf(("%s ", fp_tblDDm3[cmd.c_opb].fp_cmd));
	fp_tblDDm3[cmd.c_opb].fp_fun(fp_emp);
}

fp_isCOMPP(fp_emp)
	struct fp_em *fp_emp;
{
	int rm = ((struct cmd*)&(env(fp_opcode)))->c_rm;

	if (rm == 1) {
		dprintf(("COMPP "));
		fp_COMPP(fp_emp);
	} else
		fp_reserved(fp_emp);
}

struct fp_tbl fp_tblDEm3[8] = {
	fp_ADDPr	DEB("addpr"),
	fp_MULPr	DEB("mulpr"),
	fp_reserved	DEB("-r-"),
	fp_isCOMPP	DEB("?"),
	fp_SUBPr	DEB("subpr"),		/* doc says DE E8+i */
	fp_SUBRPr	DEB("subrpr"),		/* doc says DE E0+i */
	fp_DIVPr	DEB("divpr"),		/* doc says DE F8+i */
	fp_DIVRPr	DEB("divrpr")		/* doc says DE F0+i */
};

fp_DEm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	dprintf(("%s ", fp_tblDEm3[cmd.c_opb].fp_cmd));
	fp_tblDEm3[cmd.c_opb].fp_fun(fp_emp);
}

fp_DFm3(fp_emp, cmd)
	struct fp_em *fp_emp;
	register struct cmd cmd;
{

	if (cmd.c_opb == 4 && cmd.c_rm == 0) {
		dprintf(("STSW "));
		fp_STSW_AX(fp_emp);
	} else
		fp_reserved(fp_emp);
}

struct fp_tbl fp_tblm3[8] = {
	fp_D8m3		DEB(" "),
	fp_D9m3		DEB(" "),
	fp_DAm3		DEB(" "),
	fp_DBm3		DEB(" "),
	fp_DCm3		DEB(" "),
	fp_DDm3		DEB(" "),
	fp_DEm3		DEB(" "),
	fp_DFm3		DEB(" ")

};
