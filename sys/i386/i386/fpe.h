/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      $Id: fpe.h,v 1.9 1992/11/24 03:15:24 karels Exp $
 */
 
/*
 * 80387 subset emulator (no transcendental functions)
 */

/* #define FP_DEBUG */

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

struct sib {
	unsigned base:3;
	unsigned index:3;
	unsigned ss:2;
};

/*
 * command prefixes
 */
#define PFX_ADDR	0x67
#define PFX_DATA	0x66
#define PFX_CS		0x2E
#define PFX_DS		0x3E
#define PFX_SS		0x36
#define PFX_ES		0x26
#define PFX_FS		0x64
#define PFX_GS		0x65
#define PFX_LOCK	0xF0


#define	FWAIT		0x9B

/*
 * commands that should be processed in case
 * of unmasked exceptions
 */
#define	FINIT	0xe3db	
#define	FCLEX	0xe2db	
#define	FSTENV	0x30d9	
#define	FSTENV_MASK	0x38ff	
#define	FSAVE	0x30dc	
#define	FSAVE_MASK	0x38ff	


#define FP_ESCAPE	0x00D8
#define FP_ESCAPE_MASK	0x00F8
#define C_ESC		0x001B	/* (FP_ESCAPE >> FP_ESCAPE_SHIFT) */


struct fp_tbl {
	int	(*fp_fun)();	/* function to exec cmd */
#ifdef	FP_DEBUG
	char	*fp_cmd;
#endif
};

/*
 * some bit stuff
 */
#define HI_BIT_s		((short)0x8000)
#define HI_BIT_i		0x80000000
#define	BITS_IN_BYTE		8
#define BITS(x)			(sizeof(x) * BITS_IN_BYTE)
#define LOW_BITS_MASK(n)	((1 << (n)) - 1)
#define BITS_HI2LOW(x, n)	(((x) >> (BITS(x) - (n))) & LOW_BITS_MASK(n))

/*
 *	Data types
 *
 * _s short	short	(16-bit)	Intel's word integer
 * _i int	int	(32-bit)	Intel's short integer
 * _q fp_q	fp_q	(64-bit)	Intel's integer
 * _b fp_b	fp_b	(80-bit)	Intel's	binary-coded-decimals 
 *
 * _f fp_f	float	(32-bit)	Intel's single precision real
 * _d fp_d	double	(64-bit)	Intel's double precision real
 * _e fp_e	ext	(80-bit)	Intel's extended precision real 
 * _a fp_a	accum.	(80+16-bit)	Our accumulator
 *
 * Note: all functions for fp_x types have (fp_x*) pointer parameters
 */

typedef struct fp_q {
	unsigned mant;
	int	mant0;
} fp_q;

typedef struct fp_b {
	unsigned mant;
	unsigned mant0;
	unsigned char bc;
	unsigned char sign;	/* only high bit */
} fp_b;

typedef struct fp_f {
	unsigned mant0:23;
	unsigned exp:8;
	unsigned sign:1;
} fp_f;
#define BIT_OFFSET_f	9	/* 8 + 1 */
#define HI_BIT_f	0x400000
#define ALL_BITS_f	((HI_BIT_f << 1) - 1)
#define FP_EXP_BIAS_f	127
#define FP_EXP_MAX_f	255
#define BITS_MANT_f	23

typedef struct fp_d {
	unsigned mant;
	unsigned mant0:20;
	unsigned exp:11;
	unsigned sign:1;
} fp_d;
#define BIT_OFFSET_d	12	/* 11 + 1 */
#define HI_BIT_d	0x80000
#define ALL_BITS_d	((HI_BIT_d << 1) - 1)
#define FP_EXP_BIAS_d	1023
#define FP_EXP_MAX_d	2047
#define BITS_MANT_d	52

typedef struct fp_e {
	unsigned mant;
	unsigned mant0;
	/*
	 * unsigned short exp:15;
	 * unsigned short sign:1;
	 */
	short exp;	/* with sign in high bit */
} fp_e;
#define HI_BIT_e	HI_BIT_s
#define FP_EXP_BIAS_e	16383
#define FP_EXP_MAX_e	32767
#define BITS_MANT_e	64

typedef struct fp_a {	/* accumulator format */
	unsigned mant;
	unsigned mant0;
	short	exp;
	ushort	sign;
} fp_a;
#define HI_BIT_a	HI_BIT_i
#define HI_QUIET	(HI_BIT_i>>1)
#define FP_EXP_MAX_a	32767
#define FP_EXP_BIAS_a	16383
#define BITS_MANT_a	64

#define mant1	mant

#define FP_EXC_BIAS	24576	/* for over/underflow exceptions */

enum fp_tag {
	t_valid, t_zero, t_special, t_empty
};

#ifdef KERNEL
/*
 * environment & exceptions
 */

#define env(name)	(((fp_emp)->envp)->name)

enum {underflow, overflow};
enum {inexact_up, inexact_chop};
#define	set_indefinite	1
enum {uread, uwrite};

#define	fp_exc_precision(fp_emp) fp_exc_inexact(fp_emp, inexact_chop)
#define fp_exc_unmasked(fp_emp)  (env(fp_status) |= FPS_ES, \
				  (fp_emp)->exc++)
#define fp_was_unmasked(fp_emp)	 ((fp_emp)->exc)

#define CHECK_UNMASKED_EXC	{if (fp_was_unmasked(fp_emp)) return;}

#define CHECK_INV_OP		{if (fp_emp->inv) return;}

enum fp_class {              /* floating-point classes */
	fp_zero	= 1,
	fp_denormal = 2,
	fp_normal = 0,
	fp_infinity = 3,
	fp_quiet = 4,
	fp_signaling = 5,
	fp_unsupported = 6
};

#ifdef FP_DEBUG
int fpdebug;
#define dprintf(f)	if (fpdebug) printf f
#else
#define	dprintf(f)
#endif

/*
 * emulator internal data 
 */
#define	MAXARG	sizeof(fp_e)	/* largest datum copied in/out via membuf */

struct fp_fpregs { 
	fp_e fpr[8];
};

struct fp_em {
	struct i386_fp_save	*envp;		/* ptr to environment */
	struct fp_fpregs	*fpregsp;	/* ptr to 386 regs in env */
	int			*regsp;		/* ptr to saved 80386 
						 * (not 387!!) registers */
	fp_a	acc0;		/* 2 accumulators */
	fp_a	acc1;
	int	top;		/* current top */
	int	oldtop;		/* previous top */
	int	rm;		/* reg-mem field in 80387 command */
	unsigned
		non_st_dst :1,	/* cmd destination is not fp stack */
		left_intact :1,	/* left stack intact (exception) */
		exc :1,		/* was unmasked exception */
		inv :2;		/* was invalid operand */
	char	membuf[MAXARG];	/* buffer for memory operand */
	int	outsize;	/* amount to copyout from membuf, if any */
	int	ret;		/* XXX temp, "return" from op */
};

/*
 * Memory source operands are copied into a temporary buffer for use,
 * and memory destination operands are placed into the temporary buffer
 * to be copied out when completed.  MAXARG specifies the size of the buffer.
 */
#define GET_MEM_SRC(fp_emp, type) \
	copyin(ADDR(fp_emp), (fp_emp)->membuf, sizeof(type))
	
#define SET_MEM_DST(fp_emp, type) \
	(fp_emp)->outsize = sizeof(type)

#define	RETURN(v)	{ fp_emp->ret = (v); return; }	/* XXX temp */

#define	TAG(i)		(env(fp_tag) >> ((i) << 1) & 3)
#define	TAGset(i, t)	(env(fp_tag) &= ~(3 << ((i) << 1)), \
			 env(fp_tag) |= (t) << ((i) << 1))

#define fp_empty(i)	(TAG(i) == t_empty)

/*
 * address of user data in emulator data space 
 * [in BSD/386 we currently assume that ds is zero and don't use env(fp_ds)]
 */
#define	ADDR(fpe)	((void *) (fpe)->envp->fp_dp)
#define	seg_addr(seg, addr)	(addr)

#define	fpregp(i)	(((fp_e *) (fp_emp->fpregsp)) + i)

#define NFPREGS		8	/* number of floating point registers */

#define	reg_i(r)	(((int *)(fp_emp->regsp))[r])
#define	reg_s(r)	(((short *)(fp_emp->regsp))[r])
#define	reg_c(r)	(((char *)(fp_emp->regsp))[r])
#define	reg_seg(r)	(((short *)(fp_emp->regsp))[r])

#define	ST(n)	(((fp_emp)->top + (n) + NFPREGS) % NFPREGS)
#define	ST0	((fp_emp)->top)
#define STI	ST((fp_emp)->rm)
#define NON_ST	(-1)

#define fp_left_intact(fp_emp)	((fp_emp)->left_intact)

fp_a fp_Zero_a, fp_One_a, fp_QNaN_Indefinite_a, fp_QNaN_Infinity_a;
fp_a fp_Log2e_a, fp_Log2ten_a, fp_Log10two_a, fp_Ln2_a, fp_Pi_a; 

extern struct fp_tbl fp_tblmod[];
extern struct fp_tbl fp_tblm3[];

void fp_s2a	__P((short *sp, fp_a* ap));
void fp_i2a	__P((int *ip, fp_a* ap));
void fp_q2a	__P((fp_q *qp, fp_a* ap));
void fp_f2a	__P((fp_f *fp, fp_a* ap));
void fp_d2a	__P((fp_d *dp, fp_a* ap));
void fp_e2a	__P((fp_e *ep, fp_a* ap));
void fp_b2a	__P((fp_b *bp, fp_a* ap));

void fp_a2s	__P((struct fp_em *fp_emp, fp_a* ap, short *sp));
void fp_a2i	__P((struct fp_em *fp_emp, fp_a* ap, int *ip));
void fp_a2q	__P((struct fp_em *fp_emp, fp_a* ap, fp_q *qp));
void fp_a2f	__P((struct fp_em *fp_emp, fp_a* ap, fp_f *fp));
void fp_a2d	__P((struct fp_em *fp_emp, fp_a* ap, fp_d *dp));
void fp_a2e	__P((struct fp_em *fp_emp, fp_a* ap, fp_e *ep));
void fp_a2b	__P((struct fp_em *fp_emp, fp_a* ap, fp_b *bp));

/*
 * register offsets for emulator. 
 */
#ifdef SP
#	undef EAX
#	undef EBX
#	undef ECX
#	undef EDX
#	undef EBP
#	undef ESI
#	undef EDI
#	undef ESP
#	undef EIP
#	undef AX
#	undef BX
#	undef CX
#	undef DX
#	undef BP
#	undef SI
#	undef DI
#	undef SP
#	undef AH
#	undef AL
#	undef BH
#	undef BL
#	undef CH
#	undef CL
#	undef DH
#	undef DL
#	undef CS
#	undef DS
#	undef ES
#	undef SS
#	undef FS
#	undef GS
#endif

/*
 * for 4-byte registers scale 4 is assumed
 * for 2-byte registers scale 2 is assumed
 * for 1-byte registers scale 1 is assumed 
 * for segment registers scale 2 is assumed
 */
enum {
	EAX = tEAX, EBX = tEBX, ECX = tECX, EDX = tEDX, EBP = tEBP, 
	ESI = tESI, EDI = tEDI, ESP = tESP,
	EIP = tEIP,
	CS = tCS, DS = tDS, ES = tES, SS = tSS, 
		/* in current kernel FS and GS unused, so map them to DS */
	FS = tDS, GS = tDS,

	AX = EAX + EAX, BX = EBX + EBX, CX = ECX + ECX, DX = EDX + EDX,
	BP = EBP + EBP, SI = ESI + ESI, DI = EDI + EDI, SP = ESP + ESP,

	AL = AX + AX, AH = AL + 1, BL = BX + BX, BH = BL + 1,  
	CL = CX + CX, CH = CL + 1, DL = DX + DX, DH = DL + 1
};

#endif /* KERNEL */
