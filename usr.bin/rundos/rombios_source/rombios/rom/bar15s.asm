;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
page 94,108

;RAM for ROM
seg40abs	segment	  para at 40h
beg_seg40	label	word
;-------------------------------------------no take in the mind!
VALUE_FOR_EQUIP_BITS	equ	0
MEM_SIZE	equ	640
INITIAL_NUMLOCK_STATUS	equ	0
HARD_DISKS	equ	2
VGA_SAVE_PTR_AT_A8	equ	0
;-------------------------------------------

.xlist
include	barby40.asm
.list

end_seg40	label	word
seg40abs	ends

_text	segment	para public 'CODE'
assume	cs:_text,ds:seg40abs

extrn	seg_40h:word
extrn	Write_CMOS_ah_from_al:near
extrn	Read_CMOS_al_in_al:near

public	Int_15hWait_Timeout	;83
public	Game_port_read		;84
public	int_15h_wait		;86
public	int_15h_91		;91
;---------------------------------------------------------------  

public int_2h 
public int_0eh 
public int_75h_80287 
public int_76h 

;""""""""""""""""""""""""""""""""""""""""  
int_15h_OK_exit:
		pop	di
		xor	ah,ah			
		retf	2			
  
;""""""""""""""""""""""""""""""""""""""""  
int_15h_91:
		pop	di
		xor	ah,ah			
		iret				
  
;""""""""""""""""""""""""""""""""""""""""  
Int_15hWait_Timeout:
		cmp	al,2
		jb	short Int15WT1		
		stc				
		jmp	short Int15WT_illfun		
Int15WT1:
		pop	di
		push	ds
		mov	ds,word ptr cs:seg_40h
		dec	al
		jnz	short Int15WT_fun1		
		mov	timer_clk_flag,1	
		mov	al,0Bh
		call	Read_CMOS_al_in_al	
		and	al,3Fh			
		mov	ah,0Bh
		call	Write_CMOS_ah_from_al	
		in	al,0A1h			
		or	al,1
		out	0A1h,al			
		sti				
		clc				
		jmp	short Int15WT_illfun		
Int15WT_fun1:
		cli				
		test	timer_clk_flag,1	
		stc				
		jnz	short Int15WT_illfun		
		mov	timer_clk_flag,1	
		mov	timer_clk_low,dx	
		mov	timer_clk_hi,cx		
		mov	@timer_wait_off,bx	
		mov	@timer_wait_seg,es	
		sti				
		mov	al,0Bh
		call	Read_CMOS_al_in_al	
		and	al,7Fh
		or	al,40h			
		mov	ah,0Bh
		call	Write_CMOS_ah_from_al	
		in	al,0A1h			
		and	al,0FEh
		out	0A1h,al			
Int15WT_illfun:
		sti				
		mov	ah,0
		pop	ds
		retf	2			
  
;""""""""""""""""""""""""""""""""""""""""  
int_15h_wait:
		pop	di
		push	ds
		mov	ds,word ptr cs:seg_40h
		cli				
		test	timer_clk_flag,1	
		stc				
		jnz	short if_set_timer_clk_flag		
		mov	timer_clk_flag,1	
		mov	timer_clk_low,dx	
		mov	timer_clk_hi,cx		
		mov	@timer_wait_off,0A0h	
		mov	@timer_wait_seg,ds	
		mov	al,0Bh
		call	Read_CMOS_al_in_al	
		and	al,7Fh
		or	al,40h			
		mov	ah,0Bh
		call	Write_CMOS_ah_from_al	
		in	al,0A1h			
		and	al,0FEh
		out	0A1h,al			
		sti				
		mov	ax,620
		div	cs:const_60	;  al,ah (rest) = ax/data data=60
		mov	ah,0
		push	cx
		push	dx
wait_cycle:
		jmp	short $+2		
		jmp	short $+2		
		nop
		jmp	short $+2		
		jmp	short $+2		
		sub	dx,ax
		sbb	cx,0
		jc	short end_wait_cycle		
		test	timer_clk_flag,80h	
		jz	wait_cycle			
end_wait_cycle:
		pop	dx
		pop	cx
		mov	timer_clk_flag,0	
if_set_timer_clk_flag:
		sti				
		mov	ah,0
		pop	ds
		retf	2			

;-----------------------------------------------SEREGA
Game_port_read:
		cmp	dl,1
		mov	dx,201h
		jz	short game_read_positions 	; if (dl == 1)
		pop	di
		jc	short game_read_switches  	; if (dl == 0)
		mov	ah,86h
		stc				; CY=1 because illegal dl
		retf	2			; 
game_read_switches:
		in	al,dx			; port 0201h
		and	ax,0F0h			;CY=0
		retf	2			; 
game_read_positions:
		mov	ah,1
		mov	cx,4
;-------------------------------------------------------------  
game_rp_cycl:
		push	cx
		mov	cx,8E8h
		cli				
		call	set_timer_mode		
		mov	di,bx
		out	dx,al			
		mov	bx,0
  
	;----------------------------------  
	game_rp_c1:
			in	al,dx			
			test	ah,al
	loopnz	game_rp_c1		
	;----------------------------------  

	jnz	short game_rp_cycl_a		

		call	set_timer_mode		
		sti				
		neg	bx
		add	bx,di
		mov	cl,4
		shr	bx,cl			
		and	bh,1
game_rp_cycl_a:
		sti				
		mov	cx,8E8h
	;----------------------------------  
	game_rp_c2:
		in	al,dx			
		test	al,0Fh
	loopnz	game_rp_c2		
	;----------------------------------  
  
		pop	cx
		shl	ah,1			
		push	bx

loop	game_rp_cycl		
;-------------------------------------------------------------  
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		pop	di
		retf	2			

;===============================================
set_timer_mode:
		push	dx
		mov	dx,43h
		mov	al,0
		out	dx,al			
						
		dec	dx
		dec	dx
		dec	dx
		in	al,dx			
		mov	bl,al
		jmp	short $+2		
		in	al,dx			
		mov	bh,al
		pop	dx
		ret

;===============================================
int_0eh:
		iret				

;---------------
int_76h:
		iret				
;------------------------------------------------------------------
int_2h:		
		iret 

;========================================================
int_75h_80287: 
		push	ax
		mov	al,0
		out	0F0h,al		
		mov	al,20h		
		out	0A0h,al		
					
		out	20h,al		
					
		pop	ax
		int	2		
		iret			


;
const_60	db	60
_text	ends
end


