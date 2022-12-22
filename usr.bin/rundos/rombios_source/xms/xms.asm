;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
 .386

code	segment	byte use16
	assume	cs:code,ds:nothing,es:nothing
	org	0

drv_head:
	dw	-1,-1
	dw	0a000h
	dw	strat
	dw	command
	db	'rdxms   '

ptrsav	dd	0
intsav	dd	0

strat	proc	far
	mov	word ptr cs:[ptrsav],bx
	mov	word ptr cs:[ptrsav+2],es
	ret
strat	endp

command	proc	far
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es

	lds	bx,cs:[ptrsav]
	mov	al,byte ptr ds:[bx+2]
	cmp	al,0
	je	ok
	mov	word ptr ds:[bx+3],8001h
	jmp	exit
ok:
	call	init
exit:
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
command	endp

inter	proc	far
	cmp	ax,4300h
	je	set_installed
	cmp	ax,4310h
	je	return_entry
	jmp	dword ptr cs:[intsav]
set_installed:
	mov	al,80h
	iret
return_entry:
	push	cs
	pop	es
	mov	bx,offset entry
	iret
inter	endp

entry	proc	far
	push	fs
	push	gs
	mov	fs,ax
	mov	gs,dx
	mov	dx,08000h
	mov	al,100
	out	dx,al
	pop	gs
	pop	fs
	ret
entry endp

init	proc	near
	mov	word ptr ds:[bx+3],1h
	mov	word ptr ds:[bx+14],offset init
	mov	word ptr ds:[bx+16],cs
	mov	ax,352fh
	int	21h
	mov	word ptr cs:[intsav],bx
	mov	word ptr cs:[intsav+2],es
	push	cs
	pop	ds
	mov	dx,offset inter
	mov	ax,252fh
	int	21h
	push	cs
	pop	ds
	mov	dx,offset copyright
	mov	ah,9h
	int	21h
	ret
init	endp

copyright	db	'Rundos XMS manager (version 1.0) installed',0dh,0ah,'$'

code	ends
	end
