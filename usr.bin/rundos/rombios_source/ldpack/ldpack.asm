;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
STCK_SIZE	equ	200
LOW_LIM		equ	1000h

ENV_ADR		equ	2Ch
COM_L_ADR	equ	80h

COMSP_L		equ	8

;*********************************************
ww segment
ww ends
qq segment
qq ends
sss segment
sss ends
eee segment
eee ends
;*********************************************

ww segment

p_name	dd	?		; pointer, not a name

c_line_l db	0
	 db	'/c'
c_line_t db	128 dup (0)
fcb1	db	12 dup (0)
fcb2	db	12 dup (0)

env_seg	dw	0
c_li_p	dd	c_line_l
fcb1_p	dd	fcb1
fcb2_p	dd	fcb2

comspec	db	'COMSPEC=',0
psp	dw	?

err_mem_msg	db	'Memory not available',0Dh,0Ah,'$'
err_env_msg	db	'Environment error or no COMSPEC',0Dh,0Ah,'$'
err_exec_msg	db	'Fail to execute the program',0Dh,0Ah,'$'

ww ends

;*********************************************

qq segment
assume cs:qq,ds:ww,es:nothing,ss:sss

start:
	mov	ax,seg ww
	mov	ds,ax
	mov	psp,es
	cld

	call	set_mem
	call	set_parms
	push	ds
	mov	ax,seg ww
	mov	es,ax
	lea	bx,env_seg
	lds	dx,p_name
	mov	ax,4B00h	; exec
	int	21h
	pop	ds
	jnc	exec_ok
	lea	dx,err_exec_msg
	jmp	err_exit
exec_ok:
	mov	ah,4Dh		; get exit code
	int	21h
	mov	ah,4Ch		; exit (al == exit code)
	int	21h

;----------------------------

set_mem:
	mov	ax,es
	mov	dx,seg eee
	sub	dx,ax
	mov	bx,LOW_LIM
	sub	bx,ax
	jc	no_addi
	cmp	bx,dx
	jae	free_mem
no_addi:
	mov	bx,dx
free_mem:
	mov	ah,4Ah		; set block
	int	21h
	jc	err_exit_m
	ret
err_exit_m:
	lea	dx,err_mem_msg
	jmp	err_exit

;----------------------------

set_parms:
	call	find_comspec
	mov	es,psp
	mov	si,COM_L_ADR
	mov	cl,es:[si]
	mov	c_line_l,cl
	add	c_line_l,2
	mov	ch,0
	inc	si
	lea	di,c_line_t
	push	ds
	push	ds
	pop	es
	mov	ds,psp
	rep movsb
	mov	al,0Dh
	stosb
	pop	ds
	ret

;----------------------------

find_comspec:
	mov	es,psp
	mov	es,es:ENV_ADR
	mov	di,0
next_env:
	lea	si,comspec
	mov	cx,COMSP_L
	repz cmpsb
	jz	com_found
	mov	al,0
	mov	cx,8000h
	dec	di
	repnz scasb
	jnz	env_err
	cmp	byte ptr es:[di],0
	jz	env_err
	jmp	next_env

com_found:
	mov	word ptr p_name,di
	mov	word ptr p_name[2],es
	ret

env_err:
	lea	dx,err_env_msg
	jmp	err_exit

;----------------------------

err_exit:
	mov	ah,9
	int	21h
	mov	ax,4C01h
	int	21h

qq ends

;*********************************************

sss segment stack

	dw	STCK_SIZE dup(0)
t_o_s	label	byte

sss ends

eee segment
eee ends

;*********************************************

end start


