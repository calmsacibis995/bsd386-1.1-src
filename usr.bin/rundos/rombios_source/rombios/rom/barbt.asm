;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
page 94,108
extrn skipped_ticks_lo:word
extrn skipped_ticks_hi:word

extrn	seg_40h:word	;defined in barby.asm
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

RTC_status_reg_A	equ 0ah
RTC_status_reg_B	equ 0bh
RTC_status_reg_C	equ 0ch
  
_text	segment	para public 'CODE'
assume	cs:_text,ds:seg40abs


public	Write_CMOS_ah_from_al
public	Read_CMOS_al_in_al
public	_Read_CMOS_al_in_al
public int_1Ah_RTC
public _int_1Ah_RTC
public int_8_timer
public int_70h_clock

;"""""""""""""""""""""""""""""""""""""""""""""""""""""""
_int_1Ah_RTC:
int_1Ah_RTC:
		sti				
;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
		cmp	ah,(offset End_Clock_tabl_fun-offset Clock_tabl_fun)/2
		jc	int_1a_legal_subf
	     	cmc
		retf	2	;exit with CY=1

int_1a_legal_subf:
		push	ds
		mov	ds,word ptr cs:seg_40h
		push	di
		push	ax
		mov	al,ah
		xor	ah,ah			
		add	ax,ax
		mov	di,ax
		pop	ax
;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""
		jmp	word ptr cs:Clock_tabl_fun[di] 
;----------------------------------------------------------------
Clock_tabl_fun	label	word
	dw	Get_syst_timer
	dw	Set_syst_timer
	dw	Get_time_in_BCD
	dw	Set_time_in_BCD
	dw	Get_date_in_BCD
	dw	Set_date_in_BCD
	dw	Set_Clock_alarm
	dw	Clear_Clock_alarm
End_Clock_tabl_fun	label	word
;----------------------------------------------------------------
;subfun 8 ,9, 0ah, 0bh not imlemented
;----------------------------------------------------------------
ClockWr_CMOS_exit0h:
		call	Write_CMOS_ah_from_al
Clock_exit_0h:
		xor	ah,ah
		sti			
		pop	di
		pop	ds
		retf	2
  
Clock_exit_:
		sti			
		pop	di
		pop	ds
		retf	2		
; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
Get_syst_timer:
		mov	dx,ds:timer_low	
		mov	cx,ds:timer_hi	
		xor	ax,ax		
		xchg	al,ds:timer_rolled
Clock_ret:					;CY =0
		sti			
		pop	di
		pop	ds
	 	retf	2
;--------------------------------------------------------------------  
Set_syst_timer:
		mov	timer_low,dx	
		mov	timer_hi,cx	
		xor	ah,ah		
		mov	timer_rolled,ah	
		jmp	short Clock_ret	
;--------------------------------------------------------------------  
Set_time_in_BCD:
		call	CheckOkRd_RTC_orSetInitV			
		mov	ah,0
		mov	al,dh
		call	Write_CMOS_ah_from_al	
		mov	ah,2
		mov	al,cl
		call	Write_CMOS_ah_from_al	
		mov	ah,4				;current hour
		mov	al,ch
		call	Write_CMOS_ah_from_al	
		mov	al,RTC_status_reg_B
		mov	ah,al
		call	Read_CMOS_al_in_al	
		and	al,7Eh			
		push	dx
		and	dl,1
		or	al,dl
		pop	dx
		jmp	ClockWr_CMOS_exit0h	
;""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Get_time_in_BCD:
		call	OK_to_read_RTC_A			
		jc	short Clock_exit_CY		
		mov	al,0			;current second
		call	Read_CMOS_al_in_al	
		mov	dh,al
		mov	al,2			;current minute
		call	Read_CMOS_al_in_al	
		mov	cl,al
		mov	al,RTC_status_reg_B
		call	Read_CMOS_al_in_al	
		mov	dl,al
		and	dl,1
		mov	al,4			 ;current hour
		call	Read_CMOS_al_in_al	
		mov	ch,al
		jmp	Clock_exit_0h		
;-------------------------------------------------------------------  
Get_date_in_BCD:
		call	OK_to_read_RTC_A			
		jc	short Clock_exit_CY		
		mov	al,7			;current date of month
		call	Read_CMOS_al_in_al	
		mov	dl,al
		mov	al,8			;current month
		call	Read_CMOS_al_in_al	
		mov	dh,al
		mov	al,9			;final two digits of the current year 
		call	Read_CMOS_al_in_al	
		mov	cl,al			;century
		mov	al,32h			
		call	Read_CMOS_al_in_al	
		mov	ch,al
		jmp	Clock_exit_0h		
  
Clock_exit_CY:
		sti			
		pop	di
		pop	ds
		retf	2		
;---------------------------------------------------------------  
Set_date_in_BCD:
		call	CheckOkRd_RTC_orSetInitV			
		mov	ah,7			;current date of month
		mov	al,dl
		call	Write_CMOS_ah_from_al	
		mov	ah,8			;current month
		mov	al,dh
		call	Write_CMOS_ah_from_al	
		mov	ah,9			;final two digits of the current year
		mov	al,cl
		call	Write_CMOS_ah_from_al	
		mov	ah,32h			;century
		mov	al,ch
		call	Write_CMOS_ah_from_al	
		mov	al,RTC_status_reg_B
		mov	ah,al
		call	Read_CMOS_al_in_al	
		and	al,7Fh
		jmp	ClockWr_CMOS_exit0h	
  
;---------------------------------------------------------------  
Set_Clock_alarm:
		mov	al,RTC_status_reg_B
		call	Read_CMOS_al_in_al	
		test	al,20h			
		jnz	short Clock_exit_CY1		
		call	CheckOkRd_RTC_orSetInitV			
		mov	ah,1
		mov	al,dh
		call	Write_CMOS_ah_from_al
		mov	ah,3
		mov	al,cl
		call	Write_CMOS_ah_from_al
		mov	ah,5
		mov	al,ch
		call	Write_CMOS_ah_from_al
		mov	al,RTC_status_reg_B
		mov	ah,al
		call	Read_CMOS_al_in_al
		or	al,20h		
		call	Write_CMOS_ah_from_al

		in	al,0A1h		
		and	al,0FEh
		jmp	short $+2	
		out	0A1h,al		

		jmp	Clock_exit_0h	

Clock_exit_CY1:
		xor	ax,ax		
		stc				; Set CY
		sti			
		pop	di
		pop	ds
		retf	2		
;---------------------------------------------------------------  
Clear_Clock_alarm:
		mov	al,RTC_status_reg_B
		mov	ah,al
		call	Read_CMOS_al_in_al
		and	al,5Fh		
		jmp	ClockWr_CMOS_exit0h
  
;---------------------------------------------------------------  
  
int_8_timer:   
		push	ds
		push	ax
		push	dx
		mov	ds,word ptr cs:seg_40h
		mov	ax,timer_low		
		mov	dx,timer_hi		
		call	add_skipped_ticks	;!!!!Use LINK area
		jc	short less_24		
		inc	timer_rolled		
less_24:
		mov	timer_low,ax		
		mov	timer_hi,dx		
		dec	dsk_motor_tmr		
		jnz	short do_int_1ch		
;		call	MOTOR_OFF

do_int_1ch:
		int	1Ch			;User Tick  (each 18.2ms)

		mov	al,20h			
		out	20h,al			
						
		pop	dx
		pop	ax
		pop	ds
		iret				; Interrupt return
  
;"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
  
int_70h_clock: 
		push	ax
		push	dx
		push	di
		push	ds
		mov	al,RTC_status_reg_B
		call	Read_CMOS_al_in_al
		mov	ah,al
		mov	al,RTC_status_reg_C
		call	Read_CMOS_al_in_al
		and	al,ah
		test	al,40h		
		jz	short int_70a	
		mov	ds,word ptr cs:seg_40h
		sub	timer_clk_low,3D0h * 10h
		sbb	timer_clk_hi,0	
		jnc	short int_70a	
		mov	timer_clk_flag,0
		lds	di,dword ptr @timer_wait_off 
		or	byte ptr [di],80h
		mov	di,ax
		mov	al,ah
		and	al,3Fh			
		mov	ah,RTC_status_reg_B
		call	Write_CMOS_ah_from_al	
		mov	ax,di
int_70a:
		test	al,20h			
		jz	short int_70b		
		int	4Ah			; RTC Alarm 

int_70b:
		mov	al,20h			
		out	0A0h,al			
						
		out	20h,al			
						
		pop	ds
		pop	di
		pop	dx
		pop	ax
		iret				
  
CheckOkRd_RTC_orSetInitV:
		call	OK_to_read_RTC_A		
		jnc	short possibl_read_RTC
		mov	ah,RTC_status_reg_A
		mov	al,26h		
		call	Write_CMOS_ah_from_al
		mov	ah,RTC_status_reg_B
		mov	al,82h
		call	Write_CMOS_ah_from_al
;---------------------------------------------	???????????
		mov	al,RTC_status_reg_C
		call	Read_CMOS_al_in_al
		mov	al,0Dh
		jmp	Read_CMOS_al_in_al		
;---------------------------------------------
  
OK_to_read_RTC_A:
		push	cx
		push	ax
		xor	cx,cx		
  
wait_ok_rd_RTC:
		sti			
		mov	al,RTC_status_reg_A
		call	Read_CMOS_al_in_al
		test	al,80h
		loopnz	wait_ok_rd_RTC		; Loop if zf=0, cx>0
  
		pop	ax
		jz	short ok_rd_RTC		
		xor	ax,ax			
		sti				
		stc				; Set CY
ok_rd_RTC:
		pop	cx
  
possibl_read_RTC:
		ret

;ah -addres ;al-data  
Write_CMOS_ah_from_al:
		push	dx
		mov	dx,70h
		xchg	ah,al	;al -addres ;ah - data
		cli				
		out	dx,al			
		xchg	ah,al	;ah -addres ;al-data
		inc	dx
		jmp	short $+2		
		out	dx,al			
		pop	dx
		ret
_Read_CMOS_al_in_al:
Read_CMOS_al_in_al:
		push	dx
		mov	dx,70h
		cli				
		out	dx,al			
		inc	dx
		jmp	short $+2		
		in	al,dx			
		pop	dx
		ret

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
add_skipped_ticks:	
;				Set CF if rolling not need
		push	ds
		add	ax,cs:skipped_ticks_lo	
		adc	dx,cs:skipped_ticks_hi	
		cmp	dx,24
		jc	short less_24_1
		jnz	ge_24_1

		cmp	ax,0B0h
		jc	less_24_1

ge_24_1:	sub	dx,24
		sub	ax,0B0h
		sbb	dx,0	; clc

less_24_1:	mov	cs:skipped_ticks_lo,0	
		mov	cs:skipped_ticks_hi,0
		pop	ds
		ret
_text	ends
end



