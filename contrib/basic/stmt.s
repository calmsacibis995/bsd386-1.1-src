	.seg	"text"			! [internal]
	.proc	4
	.optim	"-O2"
	.global	_ExecStmt
_ExecStmt:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-104,%sp
	tst	%i0
	be	L77077
	nop
	ld	[%i0+4],%o0
	cmp	%o0,46
	be	L77077
	sethi	%hi(_Stack-24),%o1
	or	%o1,%lo(_Stack-24),%o1	! [internal]
	sethi	%hi(_StringSpace),%o3
	sethi	%hi(_SP),%o2
	st	%o1,[%o2+%lo(_SP)]
	or	%o3,%lo(_StringSpace),%o3 ! [internal]
	sethi	%hi(_StringTempPtr),%o0	! [internal]
	st	%o3,[%o0+%lo(_StringTempPtr)]
	ld	[%o0+%lo(_StringTempPtr)],%o0
	sethi	%hi(_DoubleSpace),%o7
	stb	%g0,[%o0]
	ld	[%o7+%lo(_DoubleSpace)],%o7
	tst	%o7
	be,a	LY11
	sethi	%hi(_IntSpace),%l3
	sethi	%hi(_DoubleSpace),%o0	! [internal]
	ld	[%o0+%lo(_DoubleSpace)],%l0
	ld	[%o0+%lo(_DoubleSpace)],%o0
	cmp	%l0,%o0
	be,a	LY10
	sethi	%hi(_DoubleSpace),%l0	! [internal]
	sethi	%hi(L123),%o0
	call	_RunError,1
	or	%o0,%lo(L123),%o0	! [internal]
	call	_abort,0
	nop
	sethi	%hi(_DoubleSpace),%l0	! [internal]
LY10:					! [internal]
	call	_free,1
	ld	[%l0+%lo(_DoubleSpace)],%o0
	st	%g0,[%l0+%lo(_DoubleSpace)]
	sethi	%hi(_IntSpace),%l3
LY11:					! [internal]
	ld	[%l3+%lo(_IntSpace)],%l3
	tst	%l3
	be,a	LY9
	ld	[%i0+4],%i5
	sethi	%hi(_IntSpace),%o0	! [internal]
	ld	[%o0+%lo(_IntSpace)],%l4
	ld	[%o0+%lo(_IntSpace)],%o0
	cmp	%l4,%o0
	be,a	LY8
	sethi	%hi(_IntSpace),%l0	! [internal]
	sethi	%hi(L130),%o0
	call	_RunError,1
	or	%o0,%lo(L130),%o0	! [internal]
	call	_abort,0
	nop
	sethi	%hi(_IntSpace),%l0	! [internal]
LY8:					! [internal]
	call	_free,1
	ld	[%l0+%lo(_IntSpace)],%o0
	st	%g0,[%l0+%lo(_IntSpace)]
	ld	[%i0+4],%i5
LY9:					! [internal]
	cmp	%i5,81
	bgu	L77074
	sll	%i5,2,%o0
	sethi	%hi(L2000001),%o1
	or	%o1,%lo(L2000001),%o1	! [internal]
	ld	[%o0+%o1],%o0
	jmp	%o0
	nop
L2000001:
	.word	L77015
	.word	L77034
	.word	L77074
	.word	L77035
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77014
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77037
	.word	L77037
	.word	L77074
	.word	L77038
	.word	L77046
	.word	L77074
	.word	L77074
	.word	L77052
	.word	L77063
	.word	L77063
	.word	L77064
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77065
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77066
	.word	L77068
	.word	L77072
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77072
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77073
	.word	L77074
	.word	L77017
	.word	L77025
	.word	L77025
	.word	L77026
	.word	L77018
	.word	L77021
	.word	L77021
	.word	L77021
	.word	L77030
	.word	L77031
	.word	L77033
	.word	L77032
	.word	L77074
	.word	L77023
	.word	L77023
	.word	L77067
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77074
	.word	L77016
	.word	L77027
	.word	L77028
	.word	L77029
L77014:
	sethi	%hi(L134),%o0
	call	_RunError,1
	or	%o0,%lo(L134),%o0	! [internal]
	b	L77077
	mov	0,%i0
L77015:
	ld	[%i0+48],%o0
	call	_ExecAssign,2
	ld	[%i0+56],%o1
	b	L77077
	mov	0,%i0
L77016:
	ld	[%i0+48],%o0
	call	_ExecStrAssign,2
	ld	[%i0+56],%o1
	b	L77077
	mov	0,%i0
L77017:
	mov	1,%o3
	mov	%i0,%o2
	mov	0,%o1
	call	_MatAssign,4
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77018:
	ld	[%i0+56],%o3
	mov	0,%o5
	mov	%i0,%o4
	mov	0,%o2
	mov	%i0,%o1
	call	_MatScalarOp,6
	mov	%i5,%o0
	b	L77077
	mov	0,%i0
L77021:
	ld	[%i0+56],%o3
	mov	2,%o5
	mov	%i0,%o4
	mov	0,%o2
	mov	%i0,%o1
	call	_MatScalarOp,6
	mov	%i5,%o0
	b	L77077
	mov	0,%i0
L77023:
	ld	[%i0+48],%o1
	call	_ExecMatInput,2
	mov	%i5,%o0
	b	L77077
	mov	0,%i0
L77025:
	mov	2,%l7
	st	%l7,[%sp+92]
	mov	%i0,%o5
	mov	1,%o4
	mov	%i0,%o3
	mov	0,%o2
	mov	%i0,%o1
	call	_MatMatOp,6
	mov	%i5,%o0
	b	L77077
	mov	0,%i0
L77026:
	mov	2,%o5
	mov	%i0,%o4
	mov	1,%o3
	mov	%i0,%o2
	mov	0,%o1
	call	_MatMul,6
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77027:
	ld	[%i0+56],%o0
	mov	0,%o2
	call	_MatFunc0,3
	mov	%i0,%o1
	b	L77077
	mov	0,%i0
L77028:
	ld	[%i0+56],%o0
	mov	2,%o4
	mov	%i0,%o3
	mov	0,%o2
	call	_MatFunc1,5
	mov	%i0,%o1
	b	L77077
	mov	0,%i0
L77029:
	mov	3,%i2
	st	%i2,[%sp+92]
	ld	[%i0+56],%o0
	mov	%i0,%o5
	mov	2,%o4
	mov	%i0,%o3
	mov	0,%o2
	call	_MatFunc2,6
	mov	%i0,%o1
	b	L77077
	mov	0,%i0
L77030:
	mov	0,%o1
	call	_MatIdentity,2
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77031:
	mov	0,%o1
	call	_MatZero,2
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77032:
	mov	1,%o3
	mov	%i0,%o2
	mov	0,%o1
	call	_MatTranspose,4
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77033:
	mov	1,%o3
	mov	%i0,%o2
	mov	0,%o1
	call	_MatInverse,4
	mov	%i0,%o0
	b	L77077
	mov	0,%i0
L77034:
	b	L77077
	mov	0,%i0
L77035:
	call	_ExecDim,1
	ld	[%i0+48],%o0
	b	L77077
	mov	0,%i0
L77037:
	ld	[%i0+4],%o0
	ld	[%i0+56],%o3
	ld	[%i0+64],%o4
	ld	[%i0+72],%o5
	mov	0,%o2
	call	_ExecFor,6
	mov	%i0,%o1
	b	L77077
	mov	0,%i0
L77038:
	sethi	%hi(_GP),%o0		! [internal]
	ld	[%o0+%lo(_GP)],%i3
	sethi	%hi(_GosubStack+480),%o1
	inc	4,%i3
	or	%o1,%lo(_GosubStack+480),%o1 ! [internal]
	cmp	%i3,%o1
	blu	L77040
	st	%i3,[%o0+%lo(_GP)]
	sethi	%hi(L177),%o0
	or	%o0,%lo(L177),%o0	! [internal]
	call	_RunError,2
	mov	120,%o1
L77040:
	sethi	%hi(_PC),%o0		! [internal]
	sethi	%hi(_GP),%o2
	ld	[%o2+%lo(_GP)],%o2
	or	%o0,%lo(_PC),%o0	! [internal]
	ld	[%o0],%o3
	sethi	%hi(_MasterSymbolVersion),%l0
	st	%o3,[%o2]
	ld	[%i0+48],%o4
	st	%o4,[%o0]
	ld	[%i0+8],%o7
	ld	[%l0+%lo(_MasterSymbolVersion)],%l0
	cmp	%o7,%l0
	bne,a	LY7
	mov	%o0,%l0			! [internal]
	ld	[%i0+28],%i0
	sethi	%hi(_PCP),%l2
	b	L77076
	st	%i0,[%l2+%lo(_PCP)]
LY7:					! [internal]
	call	_FindLine,1
	ld	[%l0],%o0
	mov	%o0,%i5
	ld	[%i5],%l3
	ld	[%l0],%l0
	cmp	%l3,%l0
	be,a	LY6
	sethi	%hi(_PCP),%l5
	sethi	%hi(_PC),%o1
	ld	[%o1+%lo(_PC)],%o1
	sethi	%hi(L183),%o0
	call	_RunError,2
	or	%o0,%lo(L183),%o0	! [internal]
	sethi	%hi(_PCP),%l5
LY6:					! [internal]
	st	%i5,[%l5+%lo(_PCP)]
	st	%i5,[%i0+28]
	sethi	%hi(_MasterSymbolVersion),%l6
	ld	[%l6+%lo(_MasterSymbolVersion)],%l6
	b	L77076
	st	%l6,[%i0+8]
L77046:
	ld	[%i0+48],%l7
	sethi	%hi(_PC),%i2
	st	%l7,[%i2+%lo(_PC)]
	ld	[%i0+8],%i3
	sethi	%hi(_MasterSymbolVersion),%i4
	ld	[%i4+%lo(_MasterSymbolVersion)],%i4
	cmp	%i3,%i4
	bne,a	LY5
	sethi	%hi(_PC),%l0		! [internal]
	ld	[%i0+28],%i0
	sethi	%hi(_PCP),%o1
	b	L77076
	st	%i0,[%o1+%lo(_PCP)]
LY5:					! [internal]
	call	_FindLine,1
	ld	[%l0+%lo(_PC)],%o0
	mov	%o0,%i5
	ld	[%i5],%o2
	ld	[%l0+%lo(_PC)],%l0
	cmp	%o2,%l0
	be,a	LY4
	sethi	%hi(_PCP),%o4
	sethi	%hi(_PC),%o1
	ld	[%o1+%lo(_PC)],%o1
	sethi	%hi(L190),%o0
	call	_RunError,2
	or	%o0,%lo(L190),%o0	! [internal]
	sethi	%hi(_PCP),%o4
LY4:					! [internal]
	st	%i5,[%o4+%lo(_PCP)]
	st	%i5,[%i0+28]
	sethi	%hi(_MasterSymbolVersion),%o5
	ld	[%o5+%lo(_MasterSymbolVersion)],%o5
	b	L77076
	st	%o5,[%i0+8]
L77052:
	call	_ExecTree,1
	ld	[%i0+48],%o0
	sethi	%hi(_SP),%o7
	ld	[%o7+%lo(_SP)],%o7
	sethi	%hi(_Stack),%l0
	or	%l0,%lo(_Stack),%l0	! [internal]
	cmp	%o7,%l0
	bgeu,a	LY3
	sethi	%hi(_SP),%l1
	sethi	%hi(L193),%o0
	call	_RunError,1
	or	%o0,%lo(L193),%o0	! [internal]
	sethi	%hi(_SP),%l1
LY3:					! [internal]
	ld	[%l1+%lo(_SP)],%l1
	ld	[%l1],%l1
	cmp	%l1,1
	be,a	LY2
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L194),%o0
	call	_RunError,1
	or	%o0,%lo(L194),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY2:					! [internal]
	ld	[%o0+%lo(_SP)],%l3
	ld	[%l3+8],%f0
	ld	[%l3+12],%f1
	std	%f0,[%fp-8]
	ld	[%o0+%lo(_SP)],%l4
	sethi	%hi(L2000000),%l6
	dec	24,%l4
	st	%l4,[%o0+%lo(_SP)]
	ldd	[%fp-8],%f4
	ldd	[%l6+%lo(L2000000)],%f2
	fcmpd	%f4,%f2
	nop				! [internal]
	fbe	L77060
	nop
	call	_ExecStmt,1
	ld	[%i0+56],%o0
	b	L77077
	mov	0,%i0
L77060:
	call	_ExecStmt,1
	ld	[%i0+64],%o0
	b	L77077
	mov	0,%i0
L77063:
	ld	[%i0+48],%o1
	call	_ExecInput,2
	mov	%i5,%o0
	b	L77077
	mov	0,%i0
L77064:
	sethi	%hi(_DataPC),%l7
	st	%g0,[%l7+%lo(_DataPC)]
	sethi	%hi(_NextData),%i2
	call	_AdvanceData,0
	st	%g0,[%i2+%lo(_NextData)]
	sethi	%hi(_DataCount),%i3
	b	L77076
	st	%g0,[%i3+%lo(_DataCount)]
L77065:
	call	_ExecNext,1
	ld	[%i0+48],%o0
	b	L77077
	mov	0,%i0
L77066:
	call	_ExecPrint,1
	ld	[%i0+48],%o0
	b	L77077
	mov	0,%i0
L77067:
	call	_ExecMatPrint,1
	ld	[%i0+48],%o0
	b	L77077
	mov	0,%i0
L77068:
	sethi	%hi(_GosubStack),%o0
	sethi	%hi(_GP),%i4
	ld	[%i4+%lo(_GP)],%i4
	or	%o0,%lo(_GosubStack),%o0 ! [internal]
	cmp	%i4,%o0
	bgeu,a	LY1
	sethi	%hi(_GP),%o5		! [internal]
	sethi	%hi(L218),%o0
	call	_RunError,1
	or	%o0,%lo(L218),%o0	! [internal]
	sethi	%hi(_GP),%o5		! [internal]
LY1:					! [internal]
	ld	[%o5+%lo(_GP)],%o1
	sethi	%hi(_PC),%o0		! [internal]
	ld	[%o1],%o1
	st	%o1,[%o0+%lo(_PC)]
	ld	[%o5+%lo(_GP)],%o4
	dec	4,%o4
	st	%o4,[%o5+%lo(_GP)]
	call	_FindLine,1
	ld	[%o0+%lo(_PC)],%o0
	sethi	%hi(_PCP),%o7
	b	L77076
	st	%o0,[%o7+%lo(_PCP)]
L77072:
	sethi	%hi(_Stop),%l1
	mov	1,%l0
	b	L77076
	st	%l0,[%l1+%lo(_Stop)]
L77073:
	sethi	%hi(_Break),%l3
	mov	1,%l2
	b	L77076
	st	%l2,[%l3+%lo(_Break)]
L77074:
	call	_OpName,1
	mov	%i5,%o0
	mov	%o0,%o1
	sethi	%hi(L224),%o0
	or	%o0,%lo(L224),%o0	! [internal]
	call	_printf,3
	mov	%i5,%o2
	b	L77077
	mov	0,%i0
L77076:
	mov	0,%i0
L77077:
	ret
	restore
	.proc	4
	.optim	"-O2"
	.global	_ExecAssign
_ExecAssign:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	call	_LValue,1
	mov	%i0,%o0
	call	_ExecTree,1
	mov	%i1,%o0
	sethi	%hi(_Stack),%o0
	sethi	%hi(_SP),%o1
	ld	[%o1+%lo(_SP)],%o1
	add	%o0,%lo(_Stack),%i0
	cmp	%o1,%i0
	bgeu,a	LY15
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L231),%o0
	call	_RunError,1
	or	%o0,%lo(L231),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY15:					! [internal]
	ld	[%o0+%lo(_SP)],%i1
	ld	[%o0+%lo(_SP)],%o2
	dec	24,%o2
	st	%o2,[%o0+%lo(_SP)]
	ld	[%i1],%o4
	cmp	%o4,1
	be,a	LY14
	sethi	%hi(_SP),%o5
	sethi	%hi(L237),%o0
	call	_RunError,1
	or	%o0,%lo(L237),%o0	! [internal]
	sethi	%hi(_SP),%o5
LY14:					! [internal]
	ld	[%o5+%lo(_SP)],%o5
	cmp	%o5,%i0
	bgeu,a	LY13
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L238),%o0
	call	_RunError,1
	or	%o0,%lo(L238),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY13:					! [internal]
	ld	[%o0+%lo(_SP)],%i0
	ld	[%o0+%lo(_SP)],%o7
	dec	24,%o7
	st	%o7,[%o0+%lo(_SP)]
	ld	[%i0],%o0
	cmp	%o0,5
	be	L77089
	cmp	%o0,7
	bne	L77090
	nop
	ld	[%i0+8],%i0
	ld	[%i1+12],%l3
	ld	[%i1+8],%i1
	st	%i1,[%i0]
	b	LY12
	st	%l3,[%i0+4]
L77089:
	ld	[%i0+8],%i0
	ld	[%i1+8],%f0
	ld	[%i1+12],%f1
	fdtoi	%f0,%f2
	b	LY12
	st	%f2,[%i0]
L77090:
	call	_ValueName,1
	ld	[%i0],%o0
	mov	%o0,%o1
	sethi	%hi(L247),%o0
	call	_RunError,2
	or	%o0,%lo(L247),%o0	! [internal]
LY12:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_ExecFor
_ExecFor:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-144,%sp
	st	%i4,[%fp+84]
	st	%i5,[%fp+88]
	add	%i1,48,%o0
	sll	%i2,3,%l7
	add	%o0,%l7,%l7
	st	%i3,[%fp+80]
	ld	[%l7],%i3
	sethi	%hi(_FP),%i5
	ld	[%i5+%lo(_FP)],%i5
	sethi	%hi(_ForStack),%o3
	add	%o3,%lo(_ForStack),%i4
	cmp	%i5,%i4
	blu,a	LY38
	cmp	%i5,%i4
	ld	[%i5+8],%o4
LY37:					! [internal]
	ld	[%o4],%o4
	cmp	%o4,%i3
	be,a	LY38
	cmp	%i5,%i4
	dec	32,%i5
	cmp	%i5,%i4
	bcc,a	LY37
	ld	[%i5+8],%o4
	cmp	%i5,%i4
LY38:					! [internal]
	blu	L77101
	sethi	%hi(_FP),%l0
	dec	32,%i5
	st	%i5,[%l0+%lo(_FP)]
L77101:
	call	_ExecTree,1
	ld	[%fp+80],%o0
	cmp	%i0,17
	bne,a	LY36
	sethi	%hi(_SP),%o5
	sethi	%hi(_SP),%l2
	ld	[%l2+%lo(_SP)],%l2
	sethi	%hi(_Stack),%l1
	add	%l1,%lo(_Stack),%i5
	cmp	%l2,%i5
	bgeu,a	LY35
	sethi	%hi(_SP),%l3
	sethi	%hi(L262),%o0
	call	_RunError,1
	or	%o0,%lo(L262),%o0	! [internal]
	sethi	%hi(_SP),%l3
LY35:					! [internal]
	ld	[%l3+%lo(_SP)],%l3
	ld	[%l3],%l3
	cmp	%l3,1
	be,a	LY34
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L263),%o0
	call	_RunError,1
	or	%o0,%lo(L263),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY34:					! [internal]
	ld	[%o0+%lo(_SP)],%o1
	dec	24,%o1
	st	%o1,[%o0+%lo(_SP)]
	ld	[%o1+8],%f0
	ld	[%o1+12],%f1
	fdtoi	%f0,%f2
	fitod	%f2,%f4
	b	L77116
	std	%f4,[%fp-8]
LY36:					! [internal]
	ld	[%o5+%lo(_SP)],%o5
	sethi	%hi(_Stack),%o4
	add	%o4,%lo(_Stack),%i5
	cmp	%o5,%i5
	bgeu,a	LY33
	sethi	%hi(_SP),%o7
	sethi	%hi(L271),%o0
	call	_RunError,1
	or	%o0,%lo(L271),%o0	! [internal]
	sethi	%hi(_SP),%o7
LY33:					! [internal]
	ld	[%o7+%lo(_SP)],%o7
	ld	[%o7],%o7
	cmp	%o7,1
	be,a	LY32
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L272),%o0
	call	_RunError,1
	or	%o0,%lo(L272),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY32:					! [internal]
	ld	[%o0+%lo(_SP)],%l1
	ld	[%l1+8],%f6
	ld	[%l1+12],%f7
	std	%f6,[%fp-16]
	ld	[%o0+%lo(_SP)],%l2
	dec	24,%l2
	st	%l2,[%o0+%lo(_SP)]
	ldd	[%fp-16],%f8
	std	%f8,[%fp-8]
L77116:
	call	_ExecTree,1
	ld	[%fp+84],%o0
	cmp	%i0,17
	bne,a	LY31
	sethi	%hi(_SP),%o7
	sethi	%hi(_SP),%o0
	ld	[%o0+%lo(_SP)],%o0
	cmp	%o0,%i5
	bgeu,a	LY30
	sethi	%hi(_SP),%o1
	sethi	%hi(L281),%o0
	call	_RunError,1
	or	%o0,%lo(L281),%o0	! [internal]
	sethi	%hi(_SP),%o1
LY30:					! [internal]
	ld	[%o1+%lo(_SP)],%o1
	ld	[%o1],%o1
	cmp	%o1,1
	be,a	LY29
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L282),%o0
	call	_RunError,1
	or	%o0,%lo(L282),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY29:					! [internal]
	ld	[%o0+%lo(_SP)],%o3
	dec	24,%o3
	st	%o3,[%o0+%lo(_SP)]
	ld	[%o3+8],%f10
	ld	[%o3+12],%f11
	fdtoi	%f10,%f12
	fitod	%f12,%f14
	b	L77131
	std	%f14,[%fp-24]
LY31:					! [internal]
	ld	[%o7+%lo(_SP)],%o7
	cmp	%o7,%i5
	bgeu,a	LY28
	sethi	%hi(_SP),%l0
	sethi	%hi(L290),%o0
	call	_RunError,1
	or	%o0,%lo(L290),%o0	! [internal]
	sethi	%hi(_SP),%l0
LY28:					! [internal]
	ld	[%l0+%lo(_SP)],%l0
	ld	[%l0],%l0
	cmp	%l0,1
	be,a	LY27
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L291),%o0
	call	_RunError,1
	or	%o0,%lo(L291),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY27:					! [internal]
	ld	[%o0+%lo(_SP)],%l2
	ld	[%l2+8],%f16
	ld	[%l2+12],%f17
	std	%f16,[%fp-32]
	ld	[%o0+%lo(_SP)],%l3
	dec	24,%l3
	st	%l3,[%o0+%lo(_SP)]
	ldd	[%fp-32],%f18
	std	%f18,[%fp-24]
L77131:
	ld	[%fp+88],%o0
	tst	%o0
	be,a	LY26
	sethi	%hi(L2000003),%o2
	call	_ExecTree,1
	nop
	cmp	%i0,17
	bne,a	LY25
	sethi	%hi(_SP),%l0
	sethi	%hi(_SP),%o1
	ld	[%o1+%lo(_SP)],%o1
	cmp	%o1,%i5
	bgeu,a	LY24
	sethi	%hi(_SP),%o2
	sethi	%hi(L302),%o0
	call	_RunError,1
	or	%o0,%lo(L302),%o0	! [internal]
	sethi	%hi(_SP),%o2
LY24:					! [internal]
	ld	[%o2+%lo(_SP)],%o2
	ld	[%o2],%o2
	cmp	%o2,1
	be,a	LY23
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L303),%o0
	call	_RunError,1
	or	%o0,%lo(L303),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY23:					! [internal]
	ld	[%o0+%lo(_SP)],%o4
	dec	24,%o4
	st	%o4,[%o0+%lo(_SP)]
	ld	[%o4+8],%f20
	ld	[%o4+12],%f21
	fdtoi	%f20,%f22
	fitod	%f22,%f24
	b	L77149
	std	%f24,[%fp-40]
LY25:					! [internal]
	ld	[%l0+%lo(_SP)],%l0
	cmp	%l0,%i5
	bgeu,a	LY22
	sethi	%hi(_SP),%l1
	sethi	%hi(L311),%o0
	call	_RunError,1
	or	%o0,%lo(L311),%o0	! [internal]
	sethi	%hi(_SP),%l1
LY22:					! [internal]
	ld	[%l1+%lo(_SP)],%l1
	ld	[%l1],%l1
	cmp	%l1,1
	be,a	LY21
	sethi	%hi(_SP),%o1		! [internal]
	sethi	%hi(L312),%o0
	call	_RunError,1
	or	%o0,%lo(L312),%o0	! [internal]
	sethi	%hi(_SP),%o1		! [internal]
LY21:					! [internal]
	ld	[%o1+%lo(_SP)],%l3
	ld	[%l3+8],%f26
	ld	[%l3+12],%f27
	std	%f26,[%fp-48]
	ld	[%o1+%lo(_SP)],%o0
	dec	24,%o0
	st	%o0,[%o1+%lo(_SP)]
	ldd	[%fp-48],%f28
	b	L77149
	std	%f28,[%fp-40]
LY26:					! [internal]
	ldd	[%o2+%lo(L2000003)],%f30
	std	%f30,[%fp-40]
L77149:
	sethi	%hi(_FP),%o0		! [internal]
	ld	[%o0+%lo(_FP)],%o3
	sethi	%hi(_ForStack+320),%o7
	inc	32,%o3
	or	%o7,%lo(_ForStack+320),%o7 ! [internal]
	cmp	%o3,%o7
	blu	L77151
	st	%o3,[%o0+%lo(_FP)]
	sethi	%hi(L322),%o0
	call	_RunError,1
	or	%o0,%lo(L322),%o0	! [internal]
L77151:
	sethi	%hi(_FP),%o0		! [internal]
	ld	[%o0+%lo(_FP)],%l0
	sethi	%hi(_PC),%l1
	ld	[%l1+%lo(_PC)],%l1
	sethi	%hi(_PCP),%l3
	st	%l1,[%l0]
	ld	[%o0+%lo(_FP)],%o0
	ld	[%l3+%lo(_PCP)],%l3
	add	%i1,8,%i4
	sll	%i2,2,%i5
	add	%i4,%i5,%i4
	st	%l3,[%o0+4]
	ld	[%i4],%o3
	sethi	%hi(_MasterSymbolVersion),%o4
	ld	[%o4+%lo(_MasterSymbolVersion)],%o4
	cmp	%o3,%o4
	bne,a	LY20
	sethi	%hi(_MasterSymbolVersion),%l0
	add	%i1,28,%o5
	add	%o5,%i5,%i5
	b	L77154
	ld	[%i5],%i0
LY20:					! [internal]
	ld	[%l0+%lo(_MasterSymbolVersion)],%l0
	add	%i1,28,%l1
	st	%l0,[%i4]
	call	_LookupSymbol,1
	ld	[%l7],%o0
	mov	%o0,%i0
	add	%l1,%i5,%i5
	st	%i0,[%i5]
L77154:
	sethi	%hi(_FP),%o0		! [internal]
	ld	[%o0+%lo(_FP)],%l3
	st	%i0,[%l3+8]
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+8],%o0
	tst	%o0
	bne,a	LY19
	sethi	%hi(_FP),%o3		! [internal]
	call	_AllocSymbol,1
	mov	%i3,%o0
	sethi	%hi(_FP),%o1		! [internal]
	ld	[%o1+%lo(_FP)],%o2
	sethi	%hi(_MasterSymbolVersion),%o3
	st	%o0,[%o2+8]
	ld	[%o3+%lo(_MasterSymbolVersion)],%o3
	st	%o3,[%i4]
	ld	[%o1+%lo(_FP)],%o1
	ld	[%o1+8],%o1
	st	%o1,[%i5]
	sethi	%hi(_FP),%o3		! [internal]
LY19:					! [internal]
	ld	[%o3+%lo(_FP)],%o7
	mov	7,%l1
	ld	[%o7+8],%o7
	st	%l1,[%o7+8]
	ldd	[%fp-40],%f2
	ldd	[%fp-8],%f0
	fsubd	%f0,%f2,%f4
	ld	[%o3+%lo(_FP)],%l2
	ld	[%l2+8],%l2
	st	%f4,[%l2+16]
	st	%f5,[%l2+20]
	ld	[%fp-20],%o2
	ld	[%fp-24],%o1
	ld	[%o3+%lo(_FP)],%o0
	st	%o1,[%o0+16]
	st	%o2,[%o0+20]
	ld	[%fp-36],%o5
	ld	[%fp-40],%o4
	ld	[%o3+%lo(_FP)],%o3
	mov	%i1,%o0
	st	%o4,[%o3+24]
	st	%o5,[%o3+28]
	call	_SkipToNext,2
	mov	%i2,%o1
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_ExecNext
_ExecNext:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_ForStack),%o0
	sethi	%hi(_FP),%o1
	ld	[%o1+%lo(_FP)],%o1
	add	%o0,%lo(_ForStack),%i3
	cmp	%o1,%i3
	bcs	LY40
	sethi	%hi(_FP),%o2
	ld	[%o2+%lo(_FP)],%o2
	ld	[%o2+8],%i5
	ld	[%i5],%i5
	ldsb	[%i5],%o5
	ldsb	[%i0],%o7
	cmp	%o5,%o7
	bne,a	LY52
	mov	%i0,%o1
	ldsb	[%i5+1],%l0
	tst	%l0
	bne,a	LY52
	mov	%i0,%o1
	ldsb	[%i0+1],%l2
	tst	%l2
	be,a	LY51
	sethi	%hi(_FP),%o0
	mov	%i0,%o1
LY52:					! [internal]
	call	_strcmp,2
	mov	%i5,%o0
	tst	%o0
	be,a	LY51
	sethi	%hi(_FP),%o0
	sethi	%hi(_FP),%i5
	ld	[%i5+%lo(_FP)],%i5
	dec	32,%i5
LY49:					! [internal]
	cmp	%i5,%i3
	blu,a	LY50
	cmp	%i5,%i3
	ld	[%i5+8],%o0
	ld	[%o0],%o0
	ldsb	[%o0],%l6
	ldsb	[%i0],%l7
	cmp	%l6,%l7
	bne,a	LY49
	dec	32,%i5
	call	_strcmp,2
	mov	%i0,%o1
	tst	%o0
	bne,a	LY49
	dec	32,%i5
	cmp	%i5,%i3
LY50:					! [internal]
	bcs	LY40
	sethi	%hi(_FP),%i2
	st	%i5,[%i2+%lo(_FP)]
	sethi	%hi(_FP),%o0
LY51:					! [internal]
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+8],%i5
	tst	%i5
	bne,a	LY48
	ld	[%i5+8],%o2
	sethi	%hi(L352),%o0
	or	%o0,%lo(L352),%o0	! [internal]
	call	_RunError,2
	mov	%i0,%o1
	ld	[%i5+8],%o2
LY48:					! [internal]
	cmp	%o2,7
	be,a	LY47
	sethi	%hi(_FP),%o0		! [internal]
	sethi	%hi(L355),%o0
	or	%o0,%lo(L355),%o0	! [internal]
	call	_RunError,2
	mov	%i0,%o1
	sethi	%hi(_FP),%o0		! [internal]
LY47:					! [internal]
	ld	[%o0+%lo(_FP)],%o3
	ld	[%o3+24],%f0
	ld	[%o3+28],%f1
	ld	[%i5+16],%f2
	ld	[%i5+20],%f3
	faddd	%f2,%f0,%f4
	sethi	%hi(L2000004),%o4
	st	%f4,[%i5+16]
	st	%f5,[%i5+20]
	ldd	[%o4+%lo(L2000004)],%f6
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+24],%f8
	ld	[%o0+28],%f9
	fcmped	%f8,%f6
	nop				! [internal]
	fbuge,a	LY46
	sethi	%hi(L2000004),%i2
	sethi	%hi(_FP),%o7
	ld	[%o7+%lo(_FP)],%o7
	ld	[%o7+16],%f10
	ld	[%o7+20],%f11
	ld	[%i5+16],%f12
	ld	[%i5+20],%f13
	fcmped	%f10,%f12
	nop				! [internal]
	fbule	LY45
	sethi	%hi(_FP),%o0		! [internal]
	ld	[%o0+%lo(_FP)],%l0
	dec	32,%l0
	b	L77192
	st	%l0,[%o0+%lo(_FP)]
LY45:					! [internal]
	ld	[%o0+%lo(_FP)],%l2
	sethi	%hi(_PC),%l4
	ld	[%l2],%l2
	sethi	%hi(_PCP),%l7
	st	%l2,[%l4+%lo(_PC)]
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+4],%o0
	b	L77192
	st	%o0,[%l7+%lo(_PCP)]
LY46:					! [internal]
	ldd	[%i2+%lo(L2000004)],%f14
	sethi	%hi(_FP),%o0
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+24],%f16
	ld	[%o0+28],%f17
	fcmped	%f16,%f14
	nop				! [internal]
	fbule,a	LY44
	sethi	%hi(L368),%o0
	sethi	%hi(_FP),%o1
	ld	[%o1+%lo(_FP)],%o1
	ld	[%o1+16],%f18
	ld	[%o1+20],%f19
	ld	[%i5+16],%f20
	ld	[%i5+20],%f21
	fcmped	%f18,%f20
	nop				! [internal]
	fbuge	LY43
	sethi	%hi(_FP),%o0		! [internal]
	ld	[%o0+%lo(_FP)],%o2
	dec	32,%o2
	st	%o2,[%o0+%lo(_FP)]
L77192:
	mov	0,%i5
LY40:					! [internal]
	ret
	restore	%g0,%i5,%o0
LY43:					! [internal]
	ld	[%o0+%lo(_FP)],%o4
	sethi	%hi(_PC),%o7
	ld	[%o4],%o4
	sethi	%hi(_PCP),%l2
	st	%o4,[%o7+%lo(_PC)]
	ld	[%o0+%lo(_FP)],%o0
	ld	[%o0+4],%o0
	b	L77192
	st	%o0,[%l2+%lo(_PCP)]
LY44:					! [internal]
	call	_RunError,1
	or	%o0,%lo(L368),%o0	! [internal]
	b	LY40
	mov	0,%i5
	.proc	4
	.optim	"-O2"
	.global	_ExecInput
_ExecInput:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_Stack),%o0
	tst	%i1
	be	LY54
	add	%o0,%lo(_Stack),%i4
	ld	[%i1+48],%o1
LY56:					! [internal]
	tst	%o1
	be,a	LY58
	ld	[%i1+56],%i1
	call	_LValue,1
	ld	[%i1+48],%o0
	cmp	%i0,24
	bne	L77202
	nop
	call	_InputValue,0
	nop
	sethi	%hi(L373),%o2
	b	L77203
	st	%o0,[%o2+%lo(L373)]
L77202:
	call	_ReadValue,0
	nop
	sethi	%hi(L373),%o3
	st	%o0,[%o3+%lo(L373)]
L77203:
	sethi	%hi(L373),%o4
	ld	[%o4+%lo(L373)],%o4
	tst	%o4
	be,a	LY60
	sethi	%hi(_SP),%l1
	sethi	%hi(_SP),%o5
	ld	[%o5+%lo(_SP)],%o5
	cmp	%o5,%i4
	bgeu,a	LY59
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L386),%o0
	call	_RunError,1
	or	%o0,%lo(L386),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY59:					! [internal]
	ld	[%o0+%lo(_SP)],%o1
	ld	[%o0+%lo(_SP)],%o7
	sethi	%hi(L373),%o2
	dec	24,%o7
	st	%o7,[%o0+%lo(_SP)]
	ld	[%i1+48],%o0
	call	_InputAssign,3
	ld	[%o2+%lo(L373)],%o2
	b	LY58
	ld	[%i1+56],%i1
LY60:					! [internal]
	ld	[%l1+%lo(_SP)],%l1
	cmp	%l1,%i4
	bgeu,a	LY57
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L391),%o0
	call	_RunError,1
	or	%o0,%lo(L391),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY57:					! [internal]
	ld	[%o0+%lo(_SP)],%i5
	ld	[%o0+%lo(_SP)],%l2
	dec	24,%l2
	st	%l2,[%o0+%lo(_SP)]
	ld	[%i1+56],%i1
LY58:					! [internal]
	tst	%i1
	bne,a	LY56
	ld	[%i1+48],%o1
LY54:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_ExecPrint
_ExecPrint:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_Stack),%o0
	tst	%i0
	mov	14,%i4
	be	L77231
	add	%o0,%lo(_Stack),%i2
	ld	[%i0+48],%o1
LY64:					! [internal]
	tst	%o1
	be,a	LY66
	ld	[%i0+56],%i0
	ld	[%i0+4],%i4
	call	_ExecTree,1
	ld	[%i0+48],%o0
	sethi	%hi(_SP),%o2
	ld	[%o2+%lo(_SP)],%o2
	cmp	%o2,%i2
	bgeu,a	LY65
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L406),%o0
	call	_RunError,1
	or	%o0,%lo(L406),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY65:					! [internal]
	ld	[%o0+%lo(_SP)],%i3
	ld	[%o0+%lo(_SP)],%o3
	cmp	%i4,14
	dec	24,%o3
	bne	L77228
	st	%o3,[%o0+%lo(_SP)]
	ld	[%i0+56],%o5
	tst	%o5
	be,a	L77229
	mov	0,%i5
	b	L77229
	mov	1,%i5
L77228:
	mov	0,%i5
L77229:
	mov	%i5,%o1
	call	_PrintValue,2
	mov	%i3,%o0
	ld	[%i0+56],%i0
LY66:					! [internal]
	tst	%i0
	bne,a	LY64
	ld	[%i0+48],%o1
L77231:
	cmp	%i4,14
	bne	LY62
	sethi	%hi(L416),%o0
	call	_printf,1
	or	%o0,%lo(L416),%o0	! [internal]
	sethi	%hi(_OutputCol),%o7
	st	%g0,[%o7+%lo(_OutputCol)]
LY62:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_ExecProgram
_ExecProgram:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_FastCount),%o0
	ld	[%o0+%lo(_FastCount)],%o0
	dec	%o0
	cmp	%o0,1
	bgeu	L77239
	sethi	%hi(L425),%o0
	call	_Error,1
	or	%o0,%lo(L425),%o0	! [internal]
	b	L77240
	nop
L77239:
	call	_InitProgram,0
	mov	0,%i5
	call	_Continue,0
	nop
L77240:
	ret
	restore	%g0,%i5,%o0
	.proc	4
	.optim	"-O2"
	.global	_ExecStrAssign
_ExecStrAssign:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	call	_LValue,1
	mov	%i0,%o0
	call	_ExecTree,1
	mov	%i1,%o0
	sethi	%hi(_Stack),%o0
	sethi	%hi(_SP),%o1
	ld	[%o1+%lo(_SP)],%o1
	add	%o0,%lo(_Stack),%i0
	cmp	%o1,%i0
	bgeu,a	LY70
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L432),%o0
	call	_RunError,1
	or	%o0,%lo(L432),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY70:					! [internal]
	ld	[%o0+%lo(_SP)],%i1
	ld	[%o0+%lo(_SP)],%o2
	dec	24,%o2
	st	%o2,[%o0+%lo(_SP)]
	ld	[%o0+%lo(_SP)],%o0
	cmp	%o0,%i0
	bgeu,a	LY69
	sethi	%hi(_SP),%o0		! [internal]
	sethi	%hi(L436),%o0
	call	_RunError,1
	or	%o0,%lo(L436),%o0	! [internal]
	sethi	%hi(_SP),%o0		! [internal]
LY69:					! [internal]
	ld	[%o0+%lo(_SP)],%i0
	ld	[%o0+%lo(_SP)],%o5
	dec	24,%o5
	st	%o5,[%o0+%lo(_SP)]
	ld	[%i0],%o0
	cmp	%o0,6
	bne	L77252
	nop
	ld	[%i1],%l0
	cmp	%l0,3
	be,a	LY68
	ldsh	[%i0+12],%o2
	sethi	%hi(L445),%o0
	call	_RunError,1
	or	%o0,%lo(L445),%o0	! [internal]
	ldsh	[%i0+12],%o2
LY68:					! [internal]
	ld	[%i1+8],%o1
	call	_strncpy,3
	ld	[%i0+8],%o0
	b	LY67
	nop
L77252:
	call	_ValueName,1
	ld	[%i0],%o0
	mov	%o0,%o1
	sethi	%hi(L447),%o0
	call	_RunError,2
	or	%o0,%lo(L447),%o0	! [internal]
LY67:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_InputAssign
_InputAssign:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	ld	[%i1],%o0
	cmp	%o0,5
	be	L77264
	cmp	%o0,6
	be	L77258
	cmp	%o0,7
	be,a	LY75
	ld	[%i2],%o1
	b	LY71
	nop
L77258:
	ld	[%i2],%o0
	cmp	%o0,3
	be,a	LY74
	ldsh	[%i1+12],%o2
	call	_ValueName,1
	ld	[%i2],%o0
	mov	%o0,%o1
	sethi	%hi(L458),%o0
	call	_RunError,2
	or	%o0,%lo(L458),%o0	! [internal]
	ldsh	[%i1+12],%o2
LY74:					! [internal]
	ld	[%i2+8],%o1
	call	_strncpy,3
	ld	[%i1+8],%o0
	b	LY71
	nop
LY75:					! [internal]
	cmp	%o1,1
	be,a	LY73
	ld	[%i1+8],%i1
	call	_ValueName,1
	ld	[%i2],%o0
	mov	%o0,%o1
	sethi	%hi(L462),%o0
	call	_RunError,2
	or	%o0,%lo(L462),%o0	! [internal]
	ld	[%i1+8],%i1
LY73:					! [internal]
	ld	[%i2+12],%o4
	ld	[%i2+8],%i2
	st	%i2,[%i1]
	b	LY71
	st	%o4,[%i1+4]
L77264:
	ld	[%i2],%o5
	cmp	%o5,1
	be,a	LY72
	ld	[%i1+8],%i1
	call	_ValueName,1
	ld	[%i2],%o0
	mov	%o0,%o1
	sethi	%hi(L466),%o0
	call	_RunError,2
	or	%o0,%lo(L466),%o0	! [internal]
	ld	[%i1+8],%i1
LY72:					! [internal]
	ld	[%i2+8],%f0
	ld	[%i2+12],%f1
	fdtoi	%f0,%f2
	st	%f2,[%i1]
LY71:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	72
	.optim	"-O2"
	.global	_InputValue
_InputValue:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_IPtr),%o3
	ld	[%o3+%lo(_IPtr)],%o3
	sethi	%hi(__ctype_+1),%o0
	ldsb	[%o3],%o3
	add	%o0,%lo(__ctype_+1),%i5
	ldsb	[%o3+%i5],%o3
	sethi	%hi(_InputLine),%o1
	andcc	%o3,8,%g0
	sethi	%hi(__iob),%o2
	add	%o1,%lo(_InputLine),%i4
	be	L77306
	add	%o2,%lo(__iob),%i3
	sethi	%hi(_IPtr),%o0		! [internal]
LY84:					! [internal]
	ld	[%o0+%lo(_IPtr)],%l1
	inc	%l1
	st	%l1,[%o0+%lo(_IPtr)]
L77274:
	sethi	%hi(_IPtr),%o3
LY79:					! [internal]
	ld	[%o3+%lo(_IPtr)],%o3
	ldsb	[%o3],%o3
	ldsb	[%o3+%i5],%o3
	andcc	%o3,8,%g0
	bne,a	LY84
	sethi	%hi(_IPtr),%o0		! [internal]
L77306:
	sethi	%hi(_IPtr),%l3
	ld	[%l3+%lo(_IPtr)],%l3
	ldsb	[%l3],%l3
	tst	%l3
	bne	L77284
	sethi	%hi(L482),%o0
LY80:					! [internal]
	call	_printf,1
	or	%o0,%lo(L482),%o0	! [internal]
	call	_FlushOutput,0
	nop
	mov	%i4,%o0
	mov	256,%o1
	call	_fgets,3
	mov	%i3,%o2
	tst	%o0
	bne,a	LY83
	sethi	%hi(_OutputCol),%i0
	sethi	%hi(__iob+16),%o0	! [internal]
	ldsh	[%o0+%lo(__iob+16)],%l6
	mov	0,%i5
	and	%l6,-49,%l6
	sth	%l6,[%o0+%lo(__iob+16)]
	sethi	%hi(L486),%o0
	call	_RunError,1
	or	%o0,%lo(L486),%o0	! [internal]
	b	L77303
	nop
LY83:					! [internal]
	call	_InitLexicon,0
	st	%g0,[%i0+%lo(_OutputCol)]
	sethi	%hi(_IPtr),%i1
	ld	[%i1+%lo(_IPtr)],%i1
	ldsb	[%i1],%i1
	ldsb	[%i1+%i5],%i1
	andcc	%i1,8,%g0
	be,a	LY82
	sethi	%hi(_SaveStrings),%o5
	sethi	%hi(_IPtr),%o0		! [internal]
LY81:					! [internal]
	ld	[%o0+%lo(_IPtr)],%o3
	inc	%o3
	st	%o3,[%o0+%lo(_IPtr)]
	ld	[%o0+%lo(_IPtr)],%o0
	ldsb	[%o0],%o0
	ldsb	[%o0+%i5],%o0
	andcc	%o0,8,%g0
	bne,a	LY81
	sethi	%hi(_IPtr),%o0		! [internal]
	sethi	%hi(_SaveStrings),%o5
LY82:					! [internal]
	st	%g0,[%o5+%lo(_SaveStrings)]
	sethi	%hi(_IPtr),%l3
	ld	[%l3+%lo(_IPtr)],%l3
	ldsb	[%l3],%l3
	tst	%l3
	be,a	LY80
	sethi	%hi(L482),%o0
L77284:
	call	_FlushOutput,0
	nop
	sethi	%hi(_IPtr),%o2
	ld	[%o2+%lo(_IPtr)],%o2
	ldsb	[%o2],%o2
	dec	34,%o2
	cmp	%o2,23
	bgu	L77300
	sll	%o2,2,%o2
	sethi	%hi(L2000007),%o1
	or	%o1,%lo(L2000007),%o1	! [internal]
	ld	[%o2+%o1],%o0
	jmp	%o0
	nop
L2000007:
	.word	L77299
	.word	L77300
	.word	L77300
	.word	L77300
	.word	L77300
	.word	L77299
	.word	L77300
	.word	L77300
	.word	L77300
	.word	L77297
	.word	L77285
	.word	L77297
	.word	L77300
	.word	L77300
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
	.word	L77297
L77285:
	sethi	%hi(_IPtr),%o0		! [internal]
	ld	[%o0+%lo(_IPtr)],%o7
	inc	%o7
	b	L77274
	st	%o7,[%o0+%lo(_IPtr)]
L77297:
	call	_NumberLex,0
	mov	1,%l1
	sethi	%hi(L472),%o0		! [internal]
	or	%o0,%lo(L472),%o0	! [internal]
	st	%l1,[%o0]
	sethi	%hi(_yylval),%l3
	ldd	[%l3+%lo(_yylval)],%f0
	std	%f0,[%o0+8]
	b	L77303
	mov	%o0,%i5
L77299:
	call	_StringLex,0
	mov	3,%l6
	sethi	%hi(L472),%o0		! [internal]
	or	%o0,%lo(L472),%o0	! [internal]
	st	%l6,[%o0]
	sethi	%hi(_yylval),%i0
	ld	[%i0+%lo(_yylval)],%i0
	mov	%o0,%i5
	b	L77303
	st	%i0,[%o0+8]
L77300:
	sethi	%hi(_IPtr),%o2		! [internal]
	ld	[%o2+%lo(_IPtr)],%o0
	inc	%o0
	st	%o0,[%o2+%lo(_IPtr)]
	ld	[%o2+%lo(_IPtr)],%o1
	sethi	%hi(L511),%o0
	or	%o0,%lo(L511),%o0	! [internal]
	call	_RunError,2
	dec	%o1
	b	LY79
	sethi	%hi(_IPtr),%o3
L77303:
	ret
	restore	%g0,%i5,%o0
	.proc	4
	.optim	"-O2"
	.global	_Print
_Print:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(__iob+20),%o0
	sethi	%hi(__ctype_+1),%o1
	add	%o1,%lo(__ctype_+1),%i1
	add	%o0,%lo(__iob+20),%i2
	mov	8,%l7
L77310:
	ldsb	[%i0],%i5
LY93:					! [internal]
	inc	%i0
	tst	%i5
	be	LY90
	cmp	%i5,8
	bne,a	LY101
	cmp	%i5,12
	sethi	%hi(__iob+20),%o0	! [internal]
	or	%o0,%lo(__iob+20),%o0	! [internal]
	ld	[%o0],%o4
	deccc	%o4
	bneg	L77314
	st	%o4,[%o0]
	inc	4,%o0			! [internal]
	ld	[%o0],%l0
	mov	8,%l1
	stb	%l1,[%l0]
	ld	[%o0],%l2
	inc	%l2
	b	L77322
	st	%l2,[%o0]
L77314:
	sethi	%hi(__iob+36),%l4
	ldsh	[%l4+%lo(__iob+36)],%l4
	andcc	%l4,128,%g0
	be,a	LY85
	mov	8,%o0
	ld	[%o0],%l6
	add	%o0,12,%o1
	ld	[%o1],%o1
	sub	%g0,%l6,%l6
	cmp	%l6,%o1
	bge,a	LY85
	mov	8,%o0
	sethi	%hi(__iob+24),%o2
	ld	[%o2+%lo(__iob+24)],%o2
	cmp	%l7,10
	mov	8,%o3
	be	L77318
	stb	%o3,[%o2]
	inc	4,%o0			! [internal]
	ld	[%o0],%o4
	ldub	[%o4],%o4
	ld	[%o0],%l0
	inc	%l0
	st	%l0,[%o0]
L77322:
	sethi	%hi(_OutputCol),%o0	! [internal]
LY100:					! [internal]
	ld	[%o0+%lo(_OutputCol)],%l3
	dec	%l3
	b	L77310
	st	%l3,[%o0+%lo(_OutputCol)]
L77318:
	sethi	%hi(__iob+24),%l2
	ld	[%l2+%lo(__iob+24)],%l2
	ldub	[%l2],%o0
LY85:					! [internal]
	call	__flsbuf,2
	mov	%i2,%o1
	b	LY100
	sethi	%hi(_OutputCol),%o0	! [internal]
LY101:					! [internal]
	be,a	LY99
	sethi	%hi(__iob+20),%o0	! [internal]
	cmp	%i5,10
	be,a	LY99
	sethi	%hi(__iob+20),%o0	! [internal]
	cmp	%i5,13
	bne,a	LY98
	cmp	%i5,9
	sethi	%hi(__iob+20),%o0	! [internal]
LY99:					! [internal]
	or	%o0,%lo(__iob+20),%o0	! [internal]
	ld	[%o0],%l5
	deccc	%l5
	bneg	L77329
	st	%l5,[%o0]
	inc	4,%o0			! [internal]
	ld	[%o0],%o2
	and	%i5,255,%i5
	stb	%i5,[%o2]
	ld	[%o0],%o3
	inc	%o3
	b	L77337
	st	%o3,[%o0]
L77329:
	sethi	%hi(__iob+36),%o5
	ldsh	[%o5+%lo(__iob+36)],%o5
	andcc	%o5,128,%g0
	be,a	LY86
	and	%i5,255,%o0
	ld	[%o0],%l0
	ld	[%o0+12],%o0
	sub	%g0,%l0,%l0
	cmp	%l0,%o0
	bge,a	LY86
	and	%i5,255,%o0
	and	%i5,255,%i5
	sethi	%hi(__iob+24),%l4
	ld	[%l4+%lo(__iob+24)],%l4
	cmp	%i5,10
	be	L77333
	stb	%i5,[%l4]
	sethi	%hi(__iob+24),%o4	! [internal]
	ld	[%o4+%lo(__iob+24)],%l6
	ldub	[%l6],%l6
	ld	[%o4+%lo(__iob+24)],%o2
	inc	%o2
	st	%o2,[%o4+%lo(__iob+24)]
L77337:
	sethi	%hi(_OutputCol),%o5
LY97:					! [internal]
	b	L77310
	st	%g0,[%o5+%lo(_OutputCol)]
L77333:
	sethi	%hi(__iob+24),%o4
	ld	[%o4+%lo(__iob+24)],%o4
	ldub	[%o4],%o0
LY86:					! [internal]
	call	__flsbuf,2
	mov	%i2,%o1
	b	LY97
	sethi	%hi(_OutputCol),%o5
LY98:					! [internal]
	bne,a	LY96
	ldsb	[%i5+%i1],%o3
	sethi	%hi(__iob+20),%o0	! [internal]
	or	%o0,%lo(__iob+20),%o0	! [internal]
	ld	[%o0],%o7
	deccc	%o7
	bneg	L77341
	st	%o7,[%o0]
	inc	4,%o0			! [internal]
	ld	[%o0],%l3
	and	%i5,255,%i5
	stb	%i5,[%l3]
	ld	[%o0],%l4
	inc	%l4
	b	L77349
	st	%l4,[%o0]
L77341:
	sethi	%hi(__iob+36),%l6
	ldsh	[%l6+%lo(__iob+36)],%l6
	andcc	%l6,128,%g0
	be,a	LY87
	and	%i5,255,%o0
	ld	[%o0],%o1
	ld	[%o0+12],%o0
	sub	%g0,%o1,%o1
	cmp	%o1,%o0
	bge,a	LY87
	and	%i5,255,%o0
	and	%i5,255,%i5
	sethi	%hi(__iob+24),%o5
	ld	[%o5+%lo(__iob+24)],%o5
	cmp	%i5,10
	be	L77345
	stb	%i5,[%o5]
	sethi	%hi(__iob+24),%o0	! [internal]
	ld	[%o0+%lo(__iob+24)],%l0
	ldub	[%l0],%l0
	ld	[%o0+%lo(__iob+24)],%l3
	inc	%l3
	st	%l3,[%o0+%lo(__iob+24)]
L77349:
	sethi	%hi(_OutputCol),%o2	! [internal]
LY95:					! [internal]
	ld	[%o2+%lo(_OutputCol)],%l6
	inc	8,%l6
	st	%l6,[%o2+%lo(_OutputCol)]
	ld	[%o2+%lo(_OutputCol)],%o1
	and	%o1,-8,%o1
	b	L77310
	st	%o1,[%o2+%lo(_OutputCol)]
L77345:
	sethi	%hi(__iob+24),%l5
	ld	[%l5+%lo(__iob+24)],%l5
	ldub	[%l5],%o0
LY87:					! [internal]
	call	__flsbuf,2
	mov	%i2,%o1
	b	LY95
	sethi	%hi(_OutputCol),%o2	! [internal]
LY96:					! [internal]
	andcc	%o3,32,%g0
	be,a	LY94
	cmp	%i5,127
	sethi	%hi(__iob+20),%o0	! [internal]
	or	%o0,%lo(__iob+20),%o0	! [internal]
	ld	[%o0],%o5
	deccc	%o5
	bneg	L77353
	st	%o5,[%o0]
	inc	4,%o0			! [internal]
	ld	[%o0],%l2
	and	%i5,255,%i5
	stb	%i5,[%l2]
	ld	[%o0],%l3
	inc	%l3
	b	L77310
	st	%l3,[%o0]
L77353:
	sethi	%hi(__iob+36),%l5
	ldsh	[%l5+%lo(__iob+36)],%l5
	andcc	%l5,128,%g0
	be,a	LY88
	and	%i5,255,%o0
	mov	%o0,%o2			! [internal]
	ld	[%o2],%o0
	inc	12,%o2
	ld	[%o2],%o2
	sub	%g0,%o0,%o0
	cmp	%o0,%o2
	bge,a	LY88
	and	%i5,255,%o0
	and	%i5,255,%i5
	sethi	%hi(__iob+24),%o4
	ld	[%o4+%lo(__iob+24)],%o4
	cmp	%i5,10
	be	L77357
	stb	%i5,[%o4]
	sethi	%hi(__iob+24),%o0	! [internal]
	ld	[%o0+%lo(__iob+24)],%o7
	ldub	[%o7],%o7
	ld	[%o0+%lo(__iob+24)],%l2
	inc	%l2
	b	L77310
	st	%l2,[%o0+%lo(__iob+24)]
L77357:
	sethi	%hi(__iob+24),%l4
	ld	[%l4+%lo(__iob+24)],%l4
	ldub	[%l4],%o0
LY88:					! [internal]
	call	__flsbuf,2
	mov	%i2,%o1
	b	LY93
	ldsb	[%i0],%i5
LY94:					! [internal]
	bgu,a	LY93
	ldsb	[%i0],%i5
	sethi	%hi(__iob+20),%o1	! [internal]
	or	%o1,%lo(__iob+20),%o1	! [internal]
	ld	[%o1],%l6
	deccc	%l6
	bneg	L77366
	st	%l6,[%o1]
	inc	4,%o1			! [internal]
	ld	[%o1],%o3
	and	%i5,255,%i5
	stb	%i5,[%o3]
	ld	[%o1],%o4
	inc	%o4
	b	L77374
	st	%o4,[%o1]
L77366:
	sethi	%hi(__iob+36),%o7
	ldsh	[%o7+%lo(__iob+36)],%o7
	andcc	%o7,128,%g0
	be,a	LY89
	and	%i5,255,%o0
	ld	[%o1],%l1
	ld	[%o1+12],%o0
	sub	%g0,%l1,%l1
	cmp	%l1,%o0
	bge,a	LY89
	and	%i5,255,%o0
	and	%i5,255,%i5
	sethi	%hi(__iob+24),%l5
	ld	[%l5+%lo(__iob+24)],%l5
	cmp	%i5,10
	be	L77370
	stb	%i5,[%l5]
	inc	4,%o1			! [internal]
	ld	[%o1],%o0
	ldub	[%o0],%o0
	ld	[%o1],%o3
	inc	%o3
	st	%o3,[%o1]
L77374:
	sethi	%hi(_OutputCol),%o0	! [internal]
LY92:					! [internal]
	ld	[%o0+%lo(_OutputCol)],%o7
	inc	%o7
	b	L77310
	st	%o7,[%o0+%lo(_OutputCol)]
L77370:
	sethi	%hi(__iob+24),%o5
	ld	[%o5+%lo(__iob+24)],%o5
	ldub	[%o5],%o0
LY89:					! [internal]
	call	__flsbuf,2
	mov	%i2,%o1
	b	LY92
	sethi	%hi(_OutputCol),%o0	! [internal]
LY90:					! [internal]
	ret
	restore	%g0,0,%o0
	.proc	4
	.optim	"-O2"
	.global	_PrintValue
_PrintValue:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-400,%sp
	ld	[%i0],%o0
	cmp	%o0,1
	be	L77385
	cmp	%o0,3
	be,a	LY108
	tst	%i1
	call	_ValueName,1
	ld	[%i0],%o0
	mov	%o0,%o1
	sethi	%hi(L609),%o0
	call	_RunError,2
	or	%o0,%lo(L609),%o0	! [internal]
	add	%fp,-300,%i0
	call	_strlen,1
	mov	%i0,%o0
	sethi	%hi(_OutputCol),%o3
	ld	[%o3+%lo(_OutputCol)],%o3
	add	%o0,%o3,%o0
	cmp	%o0,80
	bl	LY103
	nop
LY104:					! [internal]
	call	_NewLine,0
	nop
	b	LY103
	nop
L77385:
	tst	%i1
	be,a	LY107
	ld	[%i0+12],%o3
	ld	[%i0+12],%o4
	ld	[%i0+8],%o3
	sethi	%hi(L599),%o1
	or	%o1,%lo(L599),%o1	! [internal]
	mov	-13,%o2
	call	_sprintf,5
	add	%fp,-300,%o0
	b	LY105
	add	%fp,-300,%i0
LY107:					! [internal]
	ld	[%i0+8],%o2
	sethi	%hi(L601),%o1
	or	%o1,%lo(L601),%o1	! [internal]
	call	_sprintf,4
	add	%fp,-300,%o0
	b	LY105
	add	%fp,-300,%i0
LY108:					! [internal]
	be,a	LY106
	ld	[%i0+8],%o1
	call	_strlen,1
	ld	[%i0+8],%o0
	inc	13,%o0
	call	.div,2
	mov	14,%o1
	ld	[%i0+8],%o3
	mov	%o0,%o2
	sub	%g0,%o2,%o2
	sll	%o2,1,%o2
	sll	%o2,3,%o1
	sub	%g0,%o2,%o2
	add	%o2,%o1,%o2
	sethi	%hi(L606),%o1
	or	%o1,%lo(L606),%o1	! [internal]
	inc	%o2
	call	_sprintf,4
	add	%fp,-300,%o0
	b	LY105
	add	%fp,-300,%i0
LY106:					! [internal]
	call	_strcpy,2
	add	%fp,-300,%o0
	add	%fp,-300,%i0
LY105:					! [internal]
	call	_strlen,1
	mov	%i0,%o0
	sethi	%hi(_OutputCol),%o3
	ld	[%o3+%lo(_OutputCol)],%o3
	add	%o0,%o3,%o0
	cmp	%o0,80
	bge	LY104
	nop
LY103:					! [internal]
	call	_Print,1
	mov	%i0,%o0
	ret
	restore	%g0,0,%o0
	.proc	72
	.optim	"-O2"
	.global	_ReadValue
_ReadValue:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	sethi	%hi(_NextData),%o0
	ld	[%o0+%lo(_NextData)],%o0
	tst	%o0
	be	L77404
	sethi	%hi(_NextData),%o1
	ld	[%o1+%lo(_NextData)],%o1
	sethi	%hi(_DataCount),%o3
	ld	[%o1+48],%o1
	ld	[%o3+%lo(_DataCount)],%o3
	cmp	%o3,%o1
	bl,a	LY113
	sethi	%hi(_NextData),%o4
L77404:
	call	_AdvanceData,0
	nop
	sethi	%hi(_NextData),%o4
LY113:					! [internal]
	ld	[%o4+%lo(_NextData)],%o4
	tst	%o4
	bne,a	LY112
	sethi	%hi(_NextData),%o5
	sethi	%hi(L626),%o0
	call	_RunError,1
	or	%o0,%lo(L626),%o0	! [internal]
	sethi	%hi(_NextData),%o5
LY112:					! [internal]
	ld	[%o5+%lo(_NextData)],%o5
	sethi	%hi(_DataCount),%l0
	ld	[%o5+48],%o5
	ld	[%l0+%lo(_DataCount)],%l0
	cmp	%l0,%o5
	bl,a	LY111
	sethi	%hi(_DataCount),%o0	! [internal]
	sethi	%hi(L631),%o2
	sethi	%hi(L630),%o1
	sethi	%hi(__iob+40),%o0
	or	%o0,%lo(__iob+40),%o0	! [internal]
	or	%o1,%lo(L630),%o1	! [internal]
	or	%o2,%lo(L631),%o2	! [internal]
	call	_fprintf,4
	mov	629,%o3
	call	_exit,1
	mov	1,%o0
	sethi	%hi(_DataCount),%o0	! [internal]
LY111:					! [internal]
	ld	[%o0+%lo(_DataCount)],%l1
	sethi	%hi(_NextData),%l3
	inc	%l1
	st	%l1,[%o0+%lo(_DataCount)]
	ld	[%l3+%lo(_NextData)],%l3
	ld	[%l3+56],%i5
	ld	[%i5+4],%o0
	cmp	%o0,33
	be	L77410
	cmp	%o0,39
	be,a	LY110
	sethi	%hi(L619),%o0		! [internal]
	call	_OpName,1
	ld	[%i5+4],%o0
	mov	%o0,%o1
	sethi	%hi(L638),%o0
	call	_RunError,2
	or	%o0,%lo(L638),%o0	! [internal]
	b	LY109
	sethi	%hi(L619),%i4
L77410:
	sethi	%hi(L619),%o0		! [internal]
	or	%o0,%lo(L619),%o0	! [internal]
	mov	1,%l5
	st	%l5,[%o0]
	ld	[%i5+48],%f0
	ld	[%i5+52],%f1
	b	L77414
	std	%f0,[%o0+8]
LY110:					! [internal]
	or	%o0,%lo(L619),%o0	! [internal]
	mov	3,%i0
	st	%i0,[%o0]
	ld	[%i5+48],%i5
	st	%i5,[%o0+8]
L77414:
	sethi	%hi(L619),%i4
LY109:					! [internal]
	add	%i4,%lo(L619),%i0
	ret
	restore
	.proc	4
	.optim	"-O2"
	.global	_SkipToNext
_SkipToNext:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-96,%sp
	inc	48,%i0
	sll	%i1,3,%i5
	ld	[%i0+%i5],%i5
	sethi	%hi(_PC),%o0
	call	_FindLine,1
	ld	[%o0+%lo(_PC)],%o0
	sethi	%hi(_LastLine),%o3
	ld	[%o3+%lo(_LastLine)],%o3
	mov	%o0,%i0
	cmp	%i0,%o3
	bgeu,a	LY122
	sethi	%hi(L659),%o0
	ld	[%i0],%o4
LY119:					! [internal]
	sethi	%hi(_PC),%o5
	st	%o4,[%o5+%lo(_PC)]
	sethi	%hi(_PCP),%o7
	st	%i0,[%o7+%lo(_PCP)]
	ld	[%i0+8],%l0
	tst	%l0
	be,a	LY121
	sethi	%hi(_LastLine),%o3
	ld	[%i0+8],%l1
	ld	[%l1+4],%l1
	cmp	%l1,31
	bne,a	LY121
	sethi	%hi(_LastLine),%o3
	ld	[%i0+8],%i1
	ld	[%i1+48],%i1
	ldsb	[%i1],%l5
	ldsb	[%i5],%l6
	cmp	%l5,%l6
	bne,a	LY121
	sethi	%hi(_LastLine),%o3
	ldsb	[%i1+1],%l7
	tst	%l7
	bne,a	LY120
	mov	%i5,%o1
	ldsb	[%i5+1],%i3
	tst	%i3
	be	LY117
	nop
	mov	%i5,%o1
LY120:					! [internal]
	call	_strcmp,2
	mov	%i1,%o0
	tst	%o0
	be	LY117
	nop
	sethi	%hi(_LastLine),%o3
LY121:					! [internal]
	ld	[%o3+%lo(_LastLine)],%o3
	inc	16,%i0
	cmp	%i0,%o3
	bcs,a	LY119
	ld	[%i0],%o4
	sethi	%hi(L659),%o0
LY122:					! [internal]
	or	%o0,%lo(L659),%o0	! [internal]
	mov	%i5,%o2
	call	_RunError,3
	mov	%i5,%o1
	mov	0,%i0
LY117:					! [internal]
	ret
	restore
	.proc	4
	.optim	"-O2"
	.global	_AdvanceData
_AdvanceData:
!#PROLOGUE# 0
!#PROLOGUE# 1
	save	%sp,-104,%sp
	sethi	%hi(_DataCount),%o0
	st	%g0,[%o0+%lo(_DataCount)]
	sethi	%hi(_NextData),%o1
	ld	[%o1+%lo(_NextData)],%o1
	tst	%o1
	be,a	LY129
	sethi	%hi(_DataPC),%o0
	sethi	%hi(_NextData),%o0	! [internal]
	ld	[%o0+%lo(_NextData)],%o2
	ld	[%o2+64],%o2
	st	%o2,[%o0+%lo(_NextData)]
	sethi	%hi(_DataPC),%o0
LY129:					! [internal]
	call	_FindLine,1
	ld	[%o0+%lo(_DataPC)],%o0
	sethi	%hi(_NextData),%o5
	ld	[%o5+%lo(_NextData)],%o5
	mov	%o0,%i5
	tst	%o5
	bne,a	LY128
	ld	[%fp-4],%i0
	sethi	%hi(_LastLine),%o7
	ld	[%o7+%lo(_LastLine)],%o7
	cmp	%i5,%o7
	bgeu,a	LY128
	ld	[%fp-4],%i0
	sethi	%hi(_LastLine),%l0
	ld	[%l0+%lo(_LastLine)],%l0
	cmp	%i5,%l0
	bgeu,a	LY127
	sethi	%hi(_LastLine),%i1
	ld	[%i5+8],%l1
LY125:					! [internal]
	tst	%l1
	be,a	LY126
	sethi	%hi(_LastLine),%l0
	ld	[%i5+8],%l2
	ld	[%l2+4],%l2
	cmp	%l2,1
	bne,a	LY126
	sethi	%hi(_LastLine),%l0
	ld	[%i5+16],%l4
	sethi	%hi(_DataPC),%l5
	st	%l4,[%l5+%lo(_DataPC)]
	ld	[%i5+8],%i5
	sethi	%hi(_NextData),%i0
	ld	[%i5+48],%i5
	b	L77452
	st	%i5,[%i0+%lo(_NextData)]
LY126:					! [internal]
	ld	[%l0+%lo(_LastLine)],%l0
	inc	16,%i5
	cmp	%i5,%l0
	bcs,a	LY125
	ld	[%i5+8],%l1
	sethi	%hi(_LastLine),%i1
LY127:					! [internal]
	ld	[%i1+%lo(_LastLine)],%i1
	cmp	%i5,%i1
	bgeu,a	LY124
	ld	[%i5],%i5
	ld	[%i5+16],%i5
	sethi	%hi(_DataPC),%i3
	b	L77452
	st	%i5,[%i3+%lo(_DataPC)]
LY124:					! [internal]
	sethi	%hi(_DataPC),%o0
	st	%i5,[%o0+%lo(_DataPC)]
L77452:
	ld	[%fp-4],%i0
LY128:					! [internal]
	ret
	restore
	.seg	"data"			! [internal]
	.align	4
	.global	_PC
_PC:
	.word	0
	.align	4
	.global	_GP
_GP:
	.word	_GosubStack-4
	.align	4
	.global	_FP
_FP:
	.word	_ForStack-32
	.align	4
	.global	_OutputCol
_OutputCol:
	.word	0
	.align	8
L2000000:
	.word	0
	.word	0
	.align	8
L2000003:
	.word	0x3ff00000
	.word	0
	.align	8
L2000004:
	.word	0
	.word	0
	.seg	"data1"			! [internal]
	.align	4
L123:
	.ascii	"Basic Bug!  Bad double space returned.\0"
	.align	4
L130:
	.ascii	"Basic Bug!  Bad int space returned.\0"
	.align	4
L134:
	.ascii	"Syntax Error.\0"
	.align	4
L177:
	.ascii	"Gosub stack overflow: more than %d gosubs.\0"
	.align	4
L183:
	.ascii	"Line number %d doesn\`t exist.\0"
	.align	4
L190:
	.ascii	"Line number %d doesn\`t exist.\0"
	.align	4
L193:
	.ascii	"Stack underflow (PopReal).  Basic bug!\0"
	.align	4
L194:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L218:
	.ascii	"Gosub stack underflow: too many RETURNs\0"
	.align	4
L224:
	.ascii	"You forgot stmt %s (%d)\n\0"
	.align	4
L231:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L237:
	.ascii	"Need numeric quantity on right-hand side.\0"
	.align	4
L238:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L247:
	.ascii	"Can\`t assign to %s\0"
	.align	4
L262:
	.ascii	"Stack underflow (PopInt).  Basic bug!\0"
	.align	4
L263:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L271:
	.ascii	"Stack underflow (PopReal).  Basic bug!\0"
	.align	4
L272:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L281:
	.ascii	"Stack underflow (PopInt).  Basic bug!\0"
	.align	4
L282:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L290:
	.ascii	"Stack underflow (PopReal).  Basic bug!\0"
	.align	4
L291:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L302:
	.ascii	"Stack underflow (PopInt).  Basic bug!\0"
	.align	4
L303:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L311:
	.ascii	"Stack underflow (PopReal).  Basic bug!\0"
	.align	4
L312:
	.ascii	"Non-numeric value popped.\0"
	.align	4
L322:
	.ascii	"For stack overflow: too many nested for-statements.\0"
	.align	4
L352:
	.ascii	"For variable %s not initialized.  Basic bug!\0"
	.align	4
L355:
	.ascii	"For variable %s not a numeric variable.\0"
	.align	4
L368:
	.ascii	"Step on for-statement is zero.\0"
	.align	4
L386:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L391:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L406:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L416:
	.ascii	"\n\0"
	.align	4
L425:
	.ascii	"No lines to run!\0"
	.align	4
L432:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L436:
	.ascii	"Stack underflow (PopValue).  Basic bug!\0"
	.align	4
L445:
	.ascii	"Need string quantity on right-hand side.\0"
	.align	4
L447:
	.ascii	"Can\`t assign to %s\0"
	.align	4
L458:
	.ascii	"Read %s when expecting string.\0"
	.align	4
L462:
	.ascii	"Read %s when expecting number.\0"
	.align	4
L466:
	.ascii	"Read %s when expecting number.\0"
	.align	4
L482:
	.ascii	"? \0"
	.align	4
L486:
	.ascii	"Ran out of input!\0"
	.align	4
L511:
	.ascii	"Invalid input, near \`%.12s\`\0"
	.align	4
L599:
	.ascii	"%*g \0"
	.align	4
L601:
	.ascii	"%g\0"
	.align	4
L606:
	.ascii	"%*s \0"
	.align	4
L609:
	.ascii	"Can\`t PrintValue %s.\0"
	.align	4
L626:
	.ascii	"No more data to read.\0"
	.align	4
L630:
	.ascii	"Assertion failed: file \"%s\", line %d\n\0"
	.align	4
L631:
	.ascii	"stmt.c\0"
	.align	4
L638:
	.ascii	"Basic Bug!  Bad op %s in data list.\0"
	.align	4
L659:
	.ascii	"For-loop for \`%s\` has no corresponding \`next %s\` stateme"
	.ascii	"nt.\0"
	.seg	"bss"			! [internal]
	.align	4
L373:
	.skip	4
	.align	8
L472:
	.skip	24
	.align	8
L619:
	.skip	24
