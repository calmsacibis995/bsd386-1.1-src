;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
page 94,108

;-------------------------------------------defined in barbyn.asm
extrn	Config_tbl_size:byte
extrn	seg_40h:word
;-------------------------------------------defined in barbyt.asm
extrn	Write_CMOS_ah_from_al:near
extrn	Read_CMOS_al_in_al:near
;--------------------------------------------defined in barby15s.asm
extrn	Int_15hWait_Timeout:near	;83
extrn	Game_port_read:near		;84
extrn	int_15h_wait:near		;86
extrn	int_15h_91:near			;91
;------------------------------------------------------------
;RAM for ROM


seg40abs	segment	  para at 40h
beg_seg40	label	word
VALUE_FOR_EQUIP_BITS	equ	0
MEM_SIZE	equ	640
INITIAL_NUMLOCK_STATUS	equ	0
HARD_DISKS	equ	2
VGA_SAVE_PTR_AT_A8	equ	0

.xlist
include	barby40.asm
.list

end_seg40	label	word
seg40abs	ends

.286
JMP_FAR_CODE	equ	0eah

_text	segment	para public 'CODE'
assume	cs:_text,ds:seg40abs

public	int_15h_servics
public	Block_move		;87
public	Get_ext_mem_size	;88
public	Enter_in_Protect_Mode	;89

public	int_15h_CY_exit		;8a-8f
public	int_15h_OK_exit		;90
  
int_15h_servics:
		cmp	ah,91h
		jz	short int_15h_no_sti	
		sti				
int_15h_no_sti:
		sub	ah,80h
		jc	short int15h_exitCY	;for ALL subfunctions <80h
;switch for subfunctions 80h...91h


;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
	cmp	ah,(offset End_Int15tblfun - offset Int15tabl_fun_above80h)/2
     	cmc
	jc	short int15h_exitCY	;for ALL subfunctions >91h excl. 0c0h
;legal subfunctions
	push	di
	push	ax
	mov	al,ah
	xor	ah,ah			
	add	ax,ax
	mov	di,ax
	pop	ax
;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
	jmp	word ptr cs:Int15tabl_fun_above80h[di]	;
;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""

Int15tabl_fun_above80h	label word

	dw	int_15h_OK_exit		;80
	dw	int_15h_OK_exit		;81
	dw	int_15h_OK_exit		;82
	dw	Int_15hWait_Timeout	;83
	dw	Game_port_read		;84
	dw	int_15h_OK_exit		;85
	dw	int_15h_wait		;86

	dw	Block_move		;87
	dw	Get_ext_mem_size	;88
	dw	Enter_in_Protect_Mode	;89

	dw	int_15h_CY_exit		;8a
	dw	int_15h_CY_exit		;8b
	dw	int_15h_CY_exit		;8c
	dw	int_15h_CY_exit		;8d
	dw	int_15h_CY_exit		;8e
	dw	int_15h_CY_exit		;8f
	dw	int_15h_OK_exit		;90
	dw	int_15h_91		;91
End_Int15tblfun	label	word  

int_15h_CY_exit:
		pop	di
int15h_exitCY:
		mov	ah,86h
		stc				
		retf	2			
  
Block_move:
		pop	di
		mov	ah,3
		stc				; CY = 1
		retf	2			; exit

Enter_in_Protect_Mode:	
		stc
		mov	ah,0ffh
		retf 2
;"""""""""""""""""""""""""""""""""""""""" EXT MEM 
Get_ext_mem_size:
		pop	di
		mov	ax,0
		sti			
		clc			
		retf	2		
  
;""""""""""""""""""""""""""""""""""""""""  
int_15h_OK_exit:
		pop	di
		xor	ah,ah			
		retf	2			
  
;""""""""""""""""""""""""""""""""""""""""  
comment #
int_15h_91, int_15h_C0h in bar15s.asm
#
_text	ends
end
