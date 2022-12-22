/ SCO's memmove() function in the 3.2v2 development system libc.a
/ library has an insidious bug: it trashes the EBX register.  This
/ register is used to hold register variables.  I suspect the bug crept
/ in due to a simple-minded translation of a '286 routine, because on
/ the '286, BX need not be preserved.
/ 
/ The fix is to replace memmove.o in /lib/libc.a with the version
/ included below.  Note that if you use profiling, you must also put a
/ profiling version of memmove() in /usr/lib/libp/libc.a.
/ 
/ To assemble the non-profiling version:
/ 
/     as -m -o memmove.o memmove.s
/ 
/ To assemble the profiling verson:
/ 
/     as -m -o memmove_p.o profile.s memmove.s
/ 
/ where the file profile.s contains the following single line:
/
/ define(`PROFILE',``PROFILE'')
/
/ (How strange that this bug has gone unnoticed for so long...)

/ $Id: sco-memmove.s,v 1.1 1993/10/28 17:57:33 donn Exp $
/
/ Implementation of memmove(), which is inexplicably missing
/ from the SCO Unix C library.
/

	.globl	memmove
memmove:
ifdef(`PROFILE',`
	.bss
.L1:	.=.+4
	.text
	mov	$.L1,%edx
	.globl	_mcount
	call	_mcount
')
	push	%edi
	push	%esi
	mov	12(%esp),%edi
	mov	16(%esp),%esi
	mov	20(%esp),%ecx
	mov	%edi,%eax		/ return value: dest
	jcxz	mm_exit

	mov	%edi,%edx
	sub	%esi,%edx
	jb	mm_simple
	cmp	%edx,%ecx
	jb	mm_simple

	add	%ecx,%edi
	dec	%edi
	add	%ecx,%esi
	dec	%esi
	std
	rep; movsb
	cld
	jmp	mm_exit

mm_simple:
	cld
	mov	%ecx,%edx
	shr	$2,%ecx
	rep; movs
	mov	%edx,%ecx
	and	$3,%ecx
	rep; movsb

mm_exit:
	pop	%esi
	pop	%edi
	ret
