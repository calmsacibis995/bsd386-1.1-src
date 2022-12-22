LL0:
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_SaveFile
_SaveFile:
	!#PROLOGUE# 0
	sethi	%hi(LF104),%g1
	add	%g1,%lo(LF104),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	st	%i0,[%fp+0x44]
	st	%i1,[%fp+0x48]
	.seg	"data1"
	.align	4
L106:
	.ascii	"w\0"
	.seg	"text"
	ld	[%fp+0x44],%o0
	set	L106,%o1
	call	_fopen,2
	nop
	mov	%o0,%i5
	tst	%i5
	bne	L107
	nop
	.seg	"data1"
	.align	4
L109:
	.ascii	"Can't open file `%s'\0"
	.seg	"text"
	set	L109,%o0
	ld	[%fp+0x44],%o1
	call	_Error,2
	nop
	b	LE104
	nop
L107:
	ld	[%fp+0x48],%o0
	tst	%o0
	be	L110
	nop
	.seg	"data1"
	.align	4
L112:
	.ascii	"\011\"%s\"\0"
	.seg	"text"
	set	L112,%o0
	ld	[%fp+0x44],%o1
	call	_printf,2
	nop
L110:
	set	__iob+0x14,%o0
	call	_fflush,1
	nop
	mov	%i5,%o0
	mov	0,%o1
	set	0x7fffffff,%o2
	call	_ListLines,3
	nop
	mov	%i5,%o0
	call	_fclose,1
	nop
	sethi	%hi(_ChangesMade),%o1
	st	%g0,[%o1+%lo(_ChangesMade)]
	ld	[%fp+0x48],%o2
	tst	%o2
	be	L116
	nop
	.seg	"data1"
	.align	4
L117:
	.ascii	", %d lines\012\0"
	.seg	"text"
	sethi	%hi(_FastCount),%o1
	ld	[%o1+%lo(_FastCount)],%o1
	sub	%o1,0x1,%o1
	set	L117,%o0
	call	_printf,2
	nop
L116:
	mov	0,%o0
LE104:
	mov	%o0,%i0
	ret
	restore
       LF104 = -96
	LP104 = 96
	LT104 = 96
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_SaveLine
_SaveLine:
	!#PROLOGUE# 0
	sethi	%hi(LF119),%g1
	add	%g1,%lo(LF119),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	st	%i0,[%fp+0x44]
	st	%i1,[%fp+0x48]
	st	%i2,[%fp+0x4c]
	sethi	%hi(_FastCount),%o0
	ld	[%o0+%lo(_FastCount)],%o0
	sethi	%hi(_FastSize),%o1
	ld	[%o1+%lo(_FastSize)],%o1
	cmp	%o0,%o1
	blu	L121
	nop
	sethi	%hi(_FastSize),%o0
	ld	[%o0+%lo(_FastSize)],%o0
	sll	%o0,1,%o0
	sethi	%hi(_FastSize),%o2
	st	%o0,[%o2+%lo(_FastSize)]
	sethi	%hi(_FastSize),%o1
	ld	[%o1+%lo(_FastSize)],%o1
	sll	%o1,0x4,%o1
	sethi	%hi(_FastTab),%o0
	ld	[%o0+%lo(_FastTab)],%o0
	call	_realloc,2
	nop
	sethi	%hi(_FastTab),%o3
	st	%o0,[%o3+%lo(_FastTab)]
	sethi	%hi(_FastTab),%o4
	ld	[%o4+%lo(_FastTab)],%o4
	sethi	%hi(_FirstLine),%o5
	st	%o4,[%o5+%lo(_FirstLine)]
	sethi	%hi(_FastCount),%o7
	ld	[%o7+%lo(_FastCount)],%o7
	mov	%o7,%l0
	sll	%l0,0x4,%l1
	sethi	%hi(_FastTab),%l2
	ld	[%l2+%lo(_FastTab)],%l2
	add	%l2,%l1,%l3
	sub	%l3,0x10,%l4
	sethi	%hi(_LastLine),%l5
	st	%l4,[%l5+%lo(_LastLine)]
L121:
	ld	[%fp+0x44],%o0
	call	_FindLine,1
	nop
	mov	%o0,%i5
	ld	[%i5],%l6
	ld	[%fp+0x44],%l7
	cmp	%l6,%l7
	bne	L122
	nop
	ld	[%i5+0x8],%o0
	call	_FreeTree,1
	nop
	ld	[%fp+0x48],%o0
	st	%o0,[%i5+0x4]
	ld	[%fp+0x4c],%o1
	st	%o1,[%i5+0x8]
	b	LE119
	nop
L122:
	sethi	%hi(_LastLine),%i4
	ld	[%i4+%lo(_LastLine)],%i4
L126:
	cmp	%i4,%i5
	blu	L125
	nop
	add	%i4,0x10,%o0
	mov	%i4,%o1
	ld	[%o1+12],%g2
	ld	[%o1+0],%g3
	ld	[%o1+4],%g4
	ld	[%o1+8],%g5
	st	%g2,[%o0+12]
	st	%g3,[%o0+0]
	st	%g4,[%o0+4]
	st	%g5,[%o0+8]
L124:
	sub	%i4,0x10,%i4
	b	L126
	nop
L125:
	ld	[%fp+0x44],%o2
	st	%o2,[%i5]
	ld	[%fp+0x48],%o3
	st	%o3,[%i5+0x4]
	ld	[%fp+0x4c],%o4
	st	%o4,[%i5+0x8]
	sethi	%hi(_FastCount),%o5
	ld	[%o5+%lo(_FastCount)],%o5
	add	%o5,0x1,%o5
	sethi	%hi(_FastCount),%o7
	st	%o5,[%o7+%lo(_FastCount)]
	sethi	%hi(_FastCount),%l0
	ld	[%l0+%lo(_FastCount)],%l0
	mov	%l0,%l1
	sll	%l1,0x4,%l2
	sethi	%hi(_FastTab),%l3
	ld	[%l3+%lo(_FastTab)],%l3
	add	%l3,%l2,%l4
	sub	%l4,0x10,%l5
	sethi	%hi(_LastLine),%l6
	st	%l5,[%l6+%lo(_LastLine)]
	mov	0x1,%l7
	sethi	%hi(_ChangesMade),%i0
	st	%l7,[%i0+%lo(_ChangesMade)]
	mov	0,%o0
LE119:
	mov	%o0,%i0
	ret
	restore
       LF119 = -96
	LP119 = 96
	LT119 = 96
	.seg	"data"
	.seg	"text"
	.proc 0110
	.global	_FindLine
_FindLine:
	!#PROLOGUE# 0
	sethi	%hi(LF127),%g1
	add	%g1,%lo(LF127),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	sethi	%hi(_FirstLine),%i5
	ld	[%i5+%lo(_FirstLine)],%i5
	sethi	%hi(_LastLine),%i3
	ld	[%i3+%lo(_LastLine)],%i3
L129:
	cmp	%i5,%i3
	bgeu	L130
	nop
	sub	%i3,%i5,%o0
	sra	%o0,0x4,%o1
	sra	%o1,0x1,%o2
	sll	%o2,0x4,%o3
	add	%i5,%o3,%o4
	mov	%o4,%i4
	ld	[%i4],%o5
	cmp	%i0,%o5
	ble	L131
	nop
	add	%i4,0x10,%o7
	mov	%o7,%i5
	b	L132
	nop
L131:
	mov	%i4,%i3
L132:
	b	L129
	nop
L130:
	ld	[%i5],%l0
	cmp	%i0,%l0
	bg	L133
	nop
	mov	%i5,%o0
	b	LE127
	nop
L133:
	sethi	%hi(_LastLine),%o0
	ld	[%o0+%lo(_LastLine)],%o0
	cmp	%i5,%o0
	bgeu	L134
	nop
	add	%i5,0x10,%o0
	b	LE127
	nop
L134:
	sethi	%hi(_LastLine),%o0
	ld	[%o0+%lo(_LastLine)],%o0
	b	LE127
	nop
	mov	0,%o0
LE127:
	mov	%o0,%i0
	ret
	restore
       LF127 = -64
	LP127 = 64
	LT127 = 64
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_ListLines
_ListLines:
	!#PROLOGUE# 0
	sethi	%hi(LF135),%g1
	add	%g1,%lo(LF135),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	sethi	%hi(_FirstLine),%o0
	ld	[%o0+%lo(_FirstLine)],%o0
	tst	%o0
	bne	L137
	nop
	.seg	"data1"
	.align	4
L138:
	.ascii	"Workspace empty.\0"
	.seg	"text"
	set	L138,%o0
	call	_Error,1
	nop
	b	LE135
	nop
L137:
	sethi	%hi(_LastLine),%o0
	ld	[%o0+%lo(_LastLine)],%o0
	ld	[%o0],%o1
	cmp	%i2,%o1
	ble	L139
	nop
	sethi	%hi(_LastLine),%o2
	ld	[%o2+%lo(_LastLine)],%o2
	ld	[%o2],%o3
	mov	%o3,%i2
L139:
	sethi	%hi(_LastLine),%o4
	ld	[%o4+%lo(_LastLine)],%o4
	ld	[%o4],%o5
	cmp	%i1,%o5
	ble	L140
	nop
	sethi	%hi(_LastLine),%o7
	ld	[%o7+%lo(_LastLine)],%o7
	ld	[%o7],%l0
	mov	%l0,%i1
L140:
	st	%g0,[%fp+-0x4]
	mov	%i1,%o0
	call	_FindLine,1
	nop
	mov	%o0,%i5
L143:
	sethi	%hi(_LastLine),%l1
	ld	[%l1+%lo(_LastLine)],%l1
	cmp	%i5,%l1
	bgeu	L142
	nop
	ld	[%i5],%l2
	cmp	%l2,%i2
	bg	L142
	nop
	.seg	"data1"
	.align	4
L145:
	.ascii	"%5d\011\0"
	.seg	"text"
	mov	%i0,%o0
	set	L145,%o1
	ld	[%i5],%o2
	call	_fprintf,3
	nop
	mov	0x1,%o0
	st	%o0,[%fp+-0x8]
L148:
	ld	[%i5+0x4],%o1
	sra	%o1,0x3,%o2
	sub	%o2,0x1,%o3
	ld	[%fp+-0x8],%o4
	cmp	%o4,%o3
	bg	L147
	nop
	ld	[%i0],%o5
	sub	%o5,0x1,%o5
	st	%o5,[%i0]
	tst	%o5
	bneg	L2000000
	nop
	ld	[%i0+0x4],%o7
	add	%o7,0x1,%o7
	st	%o7,[%i0+0x4]
	sub	%o7,0x1,%l0
	mov	0x9,%l1
	stb	%l1,[%l0]
	mov	%l1,%l2
	b	L2000001
	nop
L2000000:
	ldsh	[%i0+0x10],%l3
	mov	%l3,%l4
	andcc	%l4,0x80,%g0
	be	L2000002
	nop
	ld	[%i0],%l5
	sub	%g0,%l5,%l6
	ld	[%i0+0xc],%l7
	cmp	%l6,%l7
	bge	L2000002
	nop
	ld	[%i0+0x4],%i3
	mov	0x9,%l2
	stb	%l2,[%i3]
	cmp	%l2,0xa
	be	L2000003
	nop
	ld	[%i0+0x4],%i4
	add	%i4,0x1,%i4
	st	%i4,[%i0+0x4]
	sub	%i4,0x1,%o0
	ldub	[%o0],%l2
	b	L2000004
	nop
L2000003:
	ld	[%i0+0x4],%o1
	ldub	[%o1],%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l2
L2000004:
	b	L2000005
	nop
L2000002:
	mov	0x9,%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l2
L2000005:
L2000001:
L146:
	ld	[%fp+-0x8],%o2
	add	%o2,0x1,%o2
	st	%o2,[%fp+-0x8]
	b	L148
	nop
L147:
	st	%g0,[%fp+-0x8]
L152:
	ld	[%i5+0x4],%o3
	and	%o3,0x7,%o4
	ld	[%fp+-0x8],%o5
	cmp	%o5,%o4
	bge	L151
	nop
	ld	[%i0],%o7
	sub	%o7,0x1,%o7
	st	%o7,[%i0]
	tst	%o7
	bneg	L2000006
	nop
	ld	[%i0+0x4],%l0
	add	%l0,0x1,%l0
	st	%l0,[%i0+0x4]
	sub	%l0,0x1,%l1
	mov	0x20,%l2
	stb	%l2,[%l1]
	mov	%l2,%l3
	b	L2000007
	nop
L2000006:
	ldsh	[%i0+0x10],%l4
	mov	%l4,%l5
	andcc	%l5,0x80,%g0
	be	L2000008
	nop
	ld	[%i0],%l6
	sub	%g0,%l6,%l7
	ld	[%i0+0xc],%i3
	cmp	%l7,%i3
	bge	L2000008
	nop
	ld	[%i0+0x4],%i4
	mov	0x20,%l3
	stb	%l3,[%i4]
	cmp	%l3,0xa
	be	L2000009
	nop
	ld	[%i0+0x4],%o0
	add	%o0,0x1,%o0
	st	%o0,[%i0+0x4]
	sub	%o0,0x1,%o1
	ldub	[%o1],%l3
	b	L2000010
	nop
L2000009:
	ld	[%i0+0x4],%o2
	ldub	[%o2],%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l3
L2000010:
	b	L2000011
	nop
L2000008:
	mov	0x20,%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l3
L2000011:
L2000007:
L150:
	ld	[%fp+-0x8],%o3
	add	%o3,0x1,%o3
	st	%o3,[%fp+-0x8]
	b	L152
	nop
L151:
	mov	%i0,%o0
	ld	[%i5+0x8],%o1
	call	_PrintTree,2
	nop
	ld	[%i0],%o4
	sub	%o4,0x1,%o4
	st	%o4,[%i0]
	tst	%o4
	bneg	L2000012
	nop
	ld	[%i0+0x4],%o5
	add	%o5,0x1,%o5
	st	%o5,[%i0+0x4]
	sub	%o5,0x1,%o7
	mov	0xa,%l0
	stb	%l0,[%o7]
	mov	%l0,%l1
	b	L2000013
	nop
L2000012:
	ldsh	[%i0+0x10],%l2
	mov	%l2,%l3
	andcc	%l3,0x80,%g0
	be	L2000014
	nop
	ld	[%i0],%l4
	sub	%g0,%l4,%l5
	ld	[%i0+0xc],%l6
	cmp	%l5,%l6
	bge	L2000014
	nop
	ld	[%i0+0x4],%l7
	mov	0xa,%l1
	stb	%l1,[%l7]
	cmp	%l1,0xa
	be	L2000015
	nop
	ld	[%i0+0x4],%i3
	add	%i3,0x1,%i3
	st	%i3,[%i0+0x4]
	sub	%i3,0x1,%i4
	ldub	[%i4],%l1
	b	L2000016
	nop
L2000015:
	ld	[%i0+0x4],%o0
	ldub	[%o0],%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l1
L2000016:
	b	L2000017
	nop
L2000014:
	mov	0xa,%o0
	mov	%i0,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%l1
L2000017:
L2000013:
	ld	[%fp+-0x4],%o1
	add	%o1,0x1,%o1
	st	%o1,[%fp+-0x4]
L141:
	add	%i5,0x10,%o2
	mov	%o2,%i5
	b	L143
	nop
L142:
	ld	[%fp+-0x4],%o3
	tst	%o3
	bne	L154
	nop
	.seg	"data1"
	.align	4
L155:
	.ascii	"No lines in range.\0"
	.seg	"text"
	set	L155,%o0
	call	_Error,1
	nop
L154:
	mov	0,%o0
LE135:
	mov	%o0,%i0
	ret
	restore
       LF135 = -104
	LP135 = 96
	LT135 = 96
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_DeleteLine
_DeleteLine:
	!#PROLOGUE# 0
	sethi	%hi(LF157),%g1
	add	%g1,%lo(LF157),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	sethi	%hi(_FastTab),%o0
	ld	[%o0+%lo(_FastTab)],%o0
	tst	%o0
	bne	L159
	nop
	b	LE157
	nop
L159:
	mov	%i0,%o0
	call	_FindLine,1
	nop
	mov	%o0,%i5
	ld	[%i5],%o1
	cmp	%o1,%i0
	be	L160
	nop
	b	LE157
	nop
L160:
L163:
	sethi	%hi(_LastLine),%o2
	ld	[%o2+%lo(_LastLine)],%o2
	cmp	%i5,%o2
	bgeu	L162
	nop
	add	%i5,0x10,%o3
	mov	%i5,%o4
	ld	[%o3+12],%g2
	ld	[%o3+0],%g3
	ld	[%o3+4],%g4
	ld	[%o3+8],%g5
	st	%g2,[%o4+12]
	st	%g3,[%o4+0]
	st	%g4,[%o4+4]
	st	%g5,[%o4+8]
L161:
	add	%i5,0x10,%i5
	b	L163
	nop
L162:
	sethi	%hi(_FastCount),%o5
	ld	[%o5+%lo(_FastCount)],%o5
	sub	%o5,0x1,%o5
	sethi	%hi(_FastCount),%o7
	st	%o5,[%o7+%lo(_FastCount)]
	sethi	%hi(_FastCount),%l0
	ld	[%l0+%lo(_FastCount)],%l0
	mov	%l0,%l1
	sll	%l1,0x4,%l2
	sethi	%hi(_FastTab),%l3
	ld	[%l3+%lo(_FastTab)],%l3
	add	%l3,%l2,%l4
	sub	%l4,0x10,%l5
	sethi	%hi(_LastLine),%l6
	st	%l5,[%l6+%lo(_LastLine)]
	mov	0x1,%l7
	sethi	%hi(_ChangesMade),%i1
	st	%l7,[%i1+%lo(_ChangesMade)]
	mov	0,%o0
LE157:
	mov	%o0,%i0
	ret
	restore
       LF157 = -96
	LP157 = 96
	LT157 = 96
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_ClearBuffer
_ClearBuffer:
	!#PROLOGUE# 0
	sethi	%hi(LF165),%g1
	add	%g1,%lo(LF165),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
L167:
	sethi	%hi(_FirstLine),%o0
	ld	[%o0+%lo(_FirstLine)],%o0
	sethi	%hi(_LastLine),%o1
	ld	[%o1+%lo(_LastLine)],%o1
	cmp	%o0,%o1
	bgeu	L168
	nop
	sethi	%hi(_FirstLine),%o2
	ld	[%o2+%lo(_FirstLine)],%o2
	ld	[%o2],%o0
	call	_DeleteLine,1
	nop
	b	L167
	nop
L168:
	sethi	%hi(_ChangesMade),%o3
	st	%g0,[%o3+%lo(_ChangesMade)]
	mov	0,%o0
LE165:
	mov	%o0,%i0
	ret
	restore
       LF165 = -96
	LP165 = 96
	LT165 = 96
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_CanQuit
_CanQuit:
	!#PROLOGUE# 0
	sethi	%hi(LF170),%g1
	add	%g1,%lo(LF170),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	sethi	%hi(_ChangesMade),%o0
	ld	[%o0+%lo(_ChangesMade)],%o0
	tst	%o0
	be	L172
	nop
	.seg	"data1"
	.align	4
L173:
	.ascii	"\011You've changed your program. %s\012\0"
	.seg	"text"
	.seg	"data1"
	.align	4
L174:
	.ascii	"You should save it first.\0"
	.seg	"text"
	set	L173,%o0
	set	L174,%o1
	call	_printf,2
	nop
	sethi	%hi(_ChangesMade),%o0
	st	%g0,[%o0+%lo(_ChangesMade)]
	mov	0,%o0
	b	LE170
	nop
L172:
	mov	0x1,%o0
	b	LE170
	nop
	mov	0,%o0
LE170:
	mov	%o0,%i0
	ret
	restore
       LF170 = -96
	LP170 = 96
	LT170 = 96
	.seg	"data"
	.common	_tempfile,0x14,"data"
	.seg	"text"
	.proc 04
	.global	_RunEditor
_RunEditor:
	!#PROLOGUE# 0
	sethi	%hi(LF177),%g1
	add	%g1,%lo(LF177),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	st	%i0,[%fp+0x44]
	sethi	%hi(_tempfile),%o0
	ldsb	[%o0+%lo(_tempfile)],%o0
	mov	%o0,%o1
	tst	%o1
	bne	L180
	nop
	.seg	"data1"
	.align	4
L181:
	.ascii	"/tmp/BasXXXXXX\0"
	.seg	"text"
	set	_tempfile,%o0
	set	L181,%o1
	call	_strcpy,2
	nop
	set	_tempfile,%o0
	call	_mktemp,1
	nop
L180:
	set	_tempfile,%o0
	mov	0,%o1
	call	_SaveFile,2
	nop
	.seg	"data1"
	.align	4
L182:
	.ascii	"%s %s\0"
	.seg	"text"
	sub	%fp,0x8c,%o0
	set	L182,%o1
	ld	[%fp+0x44],%o2
	set	_tempfile,%o3
	call	_sprintf,4
	nop
	sub	%fp,0x8c,%o0
	call	_system,1
	nop
	sub	%fp,0x64,%o0
	set	_CurFile,%o1
	call	_strcpy,2
	nop
	set	_tempfile,%o0
	mov	0,%o1
	call	_LoadFile,2
	nop
	.seg	"data1"
	.align	4
L185:
	.ascii	"rm %s\0"
	.seg	"text"
	sub	%fp,0x8c,%o0
	set	L185,%o1
	set	_tempfile,%o2
	call	_sprintf,3
	nop
	sub	%fp,0x8c,%o0
	call	_system,1
	nop
	mov	0x1,%o0
	sethi	%hi(_ChangesMade),%o1
	st	%o0,[%o1+%lo(_ChangesMade)]
	sub	%fp,0x64,%o1
	set	_CurFile,%o0
	call	_strcpy,2
	nop
	mov	0,%o0
LE177:
	mov	%o0,%i0
	ret
	restore
       LF177 = -240
	LP177 = 96
	LT177 = 96
	.seg	"data"
	.seg	"text"
	.proc 04
	.global	_DumpLineNos
_DumpLineNos:
	!#PROLOGUE# 0
	sethi	%hi(LF187),%g1
	add	%g1,%lo(LF187),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	st	%i0,[%fp+0x44]
	st	%i1,[%fp+0x48]
	mov	-0x1,%o0
	st	%o0,[%fp+-0x8]
	ld	[%fp+0x44],%o1
	st	%o1,[%fp+-0x4]
L191:
	ld	[%fp+0x48],%o2
	mov	%o2,%o3
	sll	%o3,0x4,%o4
	ld	[%fp+0x44],%o5
	add	%o5,%o4,%o7
	ld	[%fp+-0x4],%l0
	cmp	%l0,%o7
	bgeu	L190
	nop
	ld	[%fp+-0x8],%o0
	add	%o0,0x1,%o0
	st	%o0,[%fp+-0x8]
	mov	0xa,%o1
	call	.rem,2
	nop
	tst	%o0
	bne	L192
	nop
	sethi	%hi(__iob+0x28),%o0
	ld	[%o0+%lo(__iob+0x28)],%o0
	sub	%o0,0x1,%o0
	sethi	%hi(__iob+0x28),%o1
	st	%o0,[%o1+%lo(__iob+0x28)]
	tst	%o0
	bneg	L2000018
	nop
	sethi	%hi(__iob+0x2c),%o2
	ld	[%o2+%lo(__iob+0x2c)],%o2
	add	%o2,0x1,%o2
	sethi	%hi(__iob+0x2c),%o3
	st	%o2,[%o3+%lo(__iob+0x2c)]
	sub	%o2,0x1,%o4
	mov	0xa,%o5
	stb	%o5,[%o4]
	mov	%o5,%o7
	b	L2000019
	nop
L2000018:
	sethi	%hi(__iob+0x38),%l0
	ldsh	[%l0+%lo(__iob+0x38)],%l0
	mov	%l0,%l1
	andcc	%l1,0x80,%g0
	be	L2000020
	nop
	sethi	%hi(__iob+0x28),%l2
	ld	[%l2+%lo(__iob+0x28)],%l2
	sub	%g0,%l2,%l3
	sethi	%hi(__iob+0x34),%l4
	ld	[%l4+%lo(__iob+0x34)],%l4
	cmp	%l3,%l4
	bge	L2000020
	nop
	sethi	%hi(__iob+0x2c),%l5
	ld	[%l5+%lo(__iob+0x2c)],%l5
	mov	0xa,%o7
	stb	%o7,[%l5]
	cmp	%o7,0xa
	be	L2000021
	nop
	sethi	%hi(__iob+0x2c),%l6
	ld	[%l6+%lo(__iob+0x2c)],%l6
	add	%l6,0x1,%l6
	sethi	%hi(__iob+0x2c),%l7
	st	%l6,[%l7+%lo(__iob+0x2c)]
	sub	%l6,0x1,%i0
	ldub	[%i0],%o7
	b	L2000022
	nop
L2000021:
	sethi	%hi(__iob+0x2c),%i1
	ld	[%i1+%lo(__iob+0x2c)],%i1
	ldub	[%i1],%o0
	set	__iob+0x28,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%o7
L2000022:
	b	L2000023
	nop
L2000020:
	mov	0xa,%o0
	set	__iob+0x28,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%o7
L2000023:
L2000019:
L192:
	.seg	"data1"
	.align	4
L193:
	.ascii	"%7d\0"
	.seg	"text"
	ld	[%fp+-0x4],%i2
	ld	[%i2],%o1
	set	L193,%o0
	call	_printf,2
	nop
L189:
	ld	[%fp+-0x4],%i3
	add	%i3,0x10,%i3
	st	%i3,[%fp+-0x4]
	b	L191
	nop
L190:
	sethi	%hi(__iob+0x28),%i4
	ld	[%i4+%lo(__iob+0x28)],%i4
	sub	%i4,0x1,%i4
	sethi	%hi(__iob+0x28),%i5
	st	%i4,[%i5+%lo(__iob+0x28)]
	tst	%i4
	bneg	L2000024
	nop
	sethi	%hi(__iob+0x2c),%o0
	ld	[%o0+%lo(__iob+0x2c)],%o0
	add	%o0,0x1,%o0
	sethi	%hi(__iob+0x2c),%o1
	st	%o0,[%o1+%lo(__iob+0x2c)]
	sub	%o0,0x1,%o2
	mov	0xa,%o3
	stb	%o3,[%o2]
	mov	%o3,%o4
	b	L2000025
	nop
L2000024:
	sethi	%hi(__iob+0x38),%o5
	ldsh	[%o5+%lo(__iob+0x38)],%o5
	mov	%o5,%o7
	andcc	%o7,0x80,%g0
	be	L2000026
	nop
	sethi	%hi(__iob+0x28),%l0
	ld	[%l0+%lo(__iob+0x28)],%l0
	sub	%g0,%l0,%l1
	sethi	%hi(__iob+0x34),%l2
	ld	[%l2+%lo(__iob+0x34)],%l2
	cmp	%l1,%l2
	bge	L2000026
	nop
	sethi	%hi(__iob+0x2c),%l3
	ld	[%l3+%lo(__iob+0x2c)],%l3
	mov	0xa,%o4
	stb	%o4,[%l3]
	cmp	%o4,0xa
	be	L2000027
	nop
	sethi	%hi(__iob+0x2c),%l4
	ld	[%l4+%lo(__iob+0x2c)],%l4
	add	%l4,0x1,%l4
	sethi	%hi(__iob+0x2c),%l5
	st	%l4,[%l5+%lo(__iob+0x2c)]
	sub	%l4,0x1,%l6
	ldub	[%l6],%o4
	b	L2000028
	nop
L2000027:
	sethi	%hi(__iob+0x2c),%l7
	ld	[%l7+%lo(__iob+0x2c)],%l7
	ldub	[%l7],%o0
	set	__iob+0x28,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%o4
L2000028:
	b	L2000029
	nop
L2000026:
	mov	0xa,%o0
	set	__iob+0x28,%o1
	call	__flsbuf,2
	nop
	mov	%o0,%o4
L2000029:
L2000025:
	mov	0,%o0
LE187:
	mov	%o0,%i0
	ret
	restore
       LF187 = -104
	LP187 = 96
	LT187 = 96
	.seg	"data"
	.common	_FastTab,0x4,"data"
	.align	4
	.global	_FastCount
_FastCount:
	.word	0x0
	.align	4
	.global	_FastSize
_FastSize:
	.word	0x80
	.seg	"text"
	.proc 04
	.global	_AllocFastTab
_AllocFastTab:
	!#PROLOGUE# 0
	sethi	%hi(LF195),%g1
	add	%g1,%lo(LF195),%g1
	save	%sp,%g1,%sp
	!#PROLOGUE# 1
	sethi	%hi(_FastTab),%o0
	ld	[%o0+%lo(_FastTab)],%o0
	tst	%o0
	be	L197
	nop
	.seg	"data1"
	.align	4
L198:
	.ascii	"Argh!  table being reallocated.\012\0"
	.seg	"text"
	set	L198,%o0
	call	_Error,1
	nop
	call	_abort,0
	nop
	b	LE195
	nop
L197:
	sethi	%hi(_FastSize),%o0
	ld	[%o0+%lo(_FastSize)],%o0
	sll	%o0,0x4,%o0
	call	_malloc,1
	nop
	sethi	%hi(_FastTab),%o1
	st	%o0,[%o1+%lo(_FastTab)]
	mov	0x1,%o2
	sethi	%hi(_FastCount),%o3
	st	%o2,[%o3+%lo(_FastCount)]
	sethi	%hi(_FastTab),%o4
	ld	[%o4+%lo(_FastTab)],%o4
	sethi	%hi(_LastLine),%o5
	st	%o4,[%o5+%lo(_LastLine)]
	sethi	%hi(_FirstLine),%o7
	st	%o4,[%o7+%lo(_FirstLine)]
	sethi	%hi(_FirstLine),%l0
	ld	[%l0+%lo(_FirstLine)],%l0
	set	0x7fffffff,%l1
	st	%l1,[%l0]
	sethi	%hi(_FirstLine),%l2
	ld	[%l2+%lo(_FirstLine)],%l2
	st	%g0,[%l2+0x8]
	mov	0,%o0
LE195:
	mov	%o0,%i0
	ret
	restore
       LF195 = -96
	LP195 = 96
	LT195 = 96
	.seg	"data"
