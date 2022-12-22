;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;
page 94,108

title  ROM BIOS for RUNDOS

;==========================================================
;-------------------- GLOBAL VARIABLES --------------------
;==========================================================

;-------------------------------- variables from COMON AREA
public	_common_in_SNG
public	skipped_ticks_lo
public	skipped_ticks_hi
public	_adr_bufLPT1
;--------------------------------
public	first_label       ; beginning of all segmemts excluding _TEXT
public	last_label
;--------------------------------
public	start_my_rom
;--------------------------------
public	my_int11h
public	my_int12h
public	my_int_15h
public	my_int_19h
public	seg_40h
public	Config_tbl_size
;--------------------------------
public	_numFLs
public	_FL_types	;FL_types[4]
;-------------------------------- for C compiler
public	__acrtused
__acrtused equ	9876h
;==========================================================


;==========================================================
;-------------------- EXTERNALS ---------------------------
;==========================================================

;----------------------------------barbyc.c
extrn	_main:near
extrn	_hdisk_table:byte
;----------------------------------messgs.c
extrn	_messages:far
extrn	_InfoVGAInit:far
;----------------------------------int9.c
extrn	_int9:near
extrn	_int16:near
;----------------------------------int17.c
extrn	_int17:near
;----------------------------------barbt.asm
extrn	int_1Ah_RTC:near
extrn	int_8_timer:near
extrn	int_70h_clock:near
extrn	Read_CMOS_al_in_al:near
;----------------------------------bar15.asm
extrn	int_15h_servics:near
;----------------------------------bar15s.asm
extrn	int_75h_80287:near
extrn	int_76h:near
;==========================================================


;==========================================================
;----------------------- MACROS ---------------------------
;==========================================================

JMPF	macro 	x
	db	0eah
	dd	x
	endm

CORR	macro  w,n	
	mov	si,offset w
	mov	cx,n
	mov	di,si
	rep	movsb
	endm

TCORR	macro  d,s,n	
	mov	si,offset s
	mov	cx,n
	mov	di,offset d
	rep	movsb
	endm

UNEXP_HW_INT_ENTRY	macro	num, chip_num
unexp_hw_int_&num	label	far
	push	ax
	push	cx
	mov	ah,num
	jmp	unexp_hw_int_&chip_num
	endm

;==========================================================


;==========================================================
;------------------ ABSOLUTE CONSTANTS --------------------
;==========================================================

;-----------------------------------------
; Fixed entry points for many, many BIOSes

INT_2_ENTRY    equ	0F000E2C3h
INT_5_ENTRY    equ	0F000FF54h
INT_8_ENTRY    equ	0F000FEA5h
INT_9_ENTRY    equ	0F000E987h
INT_0EH_ENTRY  equ	0F000EF57h
INT_10H_ENTRY  equ	0F000F065h	 ; revectored by VGA BIOS to int 42h
INT_11H_ENTRY  equ	0F000F84Dh
INT_12H_ENTRY  equ	0F000F841h
int_13h_FLOPPY equ	0F000EC59h	 ; revectored by DISK BIOS to int 40h

INT_14H_ENTRY  equ	0F000E739h
INT_15H_ENTRY  equ	0F000F859h
INT_16H_ENTRY  equ	0F000E82Eh
INT_17H_ENTRY  equ	0F000EFD2h
INT_19H_ENTRY  equ	0F000E6F2h
INT_1AH_ENTRY  equ	0F000FE6Eh
;-----------------------------------------
l_int_2_ENTRY    equ	0E2C3h
l_int_5_ENTRY    equ	0FF54h
l_int_8_ENTRY    equ	0FEA5h
l_int_9_ENTRY    equ	0E987h
l_int_0EH_ENTRY  equ	0EF57h
l_int_10H_ENTRY  equ	0F065h	 ;to int42h now revectored by bios
l_int_11H_ENTRY  equ	0F84Dh
l_int_12H_ENTRY  equ	0F841h
;l_int_13h_FLOPPY equ	0EC59h	 ;to int40h now revectored by bios

l_int_14H_ENTRY  equ	0E739h
l_int_15H_ENTRY  equ	0F859h
l_int_16H_ENTRY  equ	0E82Eh
l_int_17H_ENTRY  equ	0EFD2h
l_int_19H_ENTRY  equ	0E6F2h
l_int_1AH_ENTRY  equ	0FE6Eh
;-----------------------------------------
;DSK_INFO_1     equ	0F000EFC7h
;-----------------------------------------
NUMLOCK_is_OFF	equ	0
NUMLOCK_is_ON	equ	20h
;-----------------------------------------
BOOT_DISK	equ	0
;-----------------------------------------
AA55_in_boot	equ	1FEh
;==========================================================


;==========================================================
;---------------- OUR PRIVATE CONSTANTS -------------------
;==========================================================

;-----------------------------------------
BEGIN_IMAGE_REAL_FIRST_PG	equ	09F00h
;-----------------------------------------
INIT_SS		equ	09800h
INIT_SP		equ	08000h-2
;-----------------------------------------
MEM_SIZE	equ	640
;-----------------------------------------
INITIAL_NUMLOCK_STATUS	equ	NUMLOCK_is_OFF
;-----------------------------------------

;-----------------------------------------
; values depending on VGA adapter

int_10h_video	equ    0       ;C000:2307
Video_table_1d	equ    0       ;C000:7E11
Video_table_1f	equ    0       ;C000:59A0
Video_43h	equ    0       ;C000:55A0
Vga_save_ptr_at_a8     equ	0   ; C000:06bb
;-----------------------------------------
PORT_FOR_SASHA	equ	08000h
;-----------------------------------------
len_print_buf	equ	1024
l_error_text	equ	200
;==========================================================


;==========================================================
;----------------- start of CODE segment ------------------
;==========================================================

_text	segment	para public 'CODE'

;==========================================================
;---- fields for BARBY_MK.EXE for building ROM's image ----

bg_text	dw	last		  ;segment
	dw	first_label	  ;offset
	dw	last_label
	dw	first_label

;==========================================================
;-------------- variables for our int 13h -----------------

	org  BEGIN_OFFS - 100h

err_code	dw	0
r_ax		dw	0
r_bx		dw	0
r_cx		dw	0
r_dx		dw	0
r_bp		dw	0
r_si		dw	0
r_di		dw	0
r_ds		dw	0
r_es		dw	0
r_flags		dw	0

fun_on_entry	db	0
dsk_on_entry	db	0
;==========================================================


;==========================================================
;------------------ COMMON AREA (SNG) ---------------------
;-------------  256 bytes    at f000:6000   ---------------
;==========================================================

	org	BEGIN_OFFS

_common_in_SNG	label	dword

cli_flag	dd	0	; filled by kernel & monitor
pending_flag	dd	0	; only for kernel -- pending irq
addr_link_fi13	dd	err_code
skipped_ticks_lo dw	0
skipped_ticks_hi dw	0
addr_for_KB	dd	0	; not used now

num_printers	dd	3	; filled by monitor
num_COMs	dd	4	; filled by monitor
remote_terminal	dd	0	; filled by monitor

menu_stack	dd	stack_for_int10h  ; used by MENU, filled by ROM
menu_fun	dd	_MESSAGES	  ; used by MENU, filled by ROM

aimseg0		dd	_beg_im_p0
aimseg40	dd	_beg_seg40

_adr_bufLPT1	dd	lpt1buf	
		dd	lpt2buf
		dd	lpt3buf

int_10_parm	dd	0
reserv2		dd	8 dup(0)	; COMx buffers -- not used now

;------------------------------------------------
cs_ip		dd	rom_start

ss_sp		dw     INIT_SP		; INIT_SP  equ	08000h-2
		dw     INIT_SS		; INIT_SS  equ	09800h

		dw     0		; image of real page 0
		dw     BEGIN_IMAGE_REAL_FIRST_PG	

		dd	_hdisk_table		; parameter table
		dd	addr_ADDR_fl_PARAMS	; parameter table

		dd	_error_text	; used by MENU, filled by ROM

		dd	BEGIN_OFFS	; ROM offset inside seg F000

;==========================================================
;---------------- end of COMMON AREA ----------------------
;==========================================================


;==========================================================
;---------------- cold start of ROM BIOS ------------------
;==========================================================
; Start from monitor cs:ip F000:6100
;		     ss:sp 9800:8000
;		     cli + cld
; bootd at f000:6106 0 = A: 80h = C:
;-----------------------------------

	org	BEGIN_OFFS + 100h

assume	cs:_text,ds:nothing,es:nothing

;-----------------------------------
rom_start:
	jmp	far ptr under_BELCH_ini
	nop
;-----------------------------------
bootd	db	BOOT_DISK	  
;-----------------------------------

under_BELCH_ini:
	call	VIDEO_correction
;	jmpf	start_my_rom
	db	0eah
	dw	start_my_rom
	dw	_TEXT

;-----------------------------------

VIDEO_correction:  	;by image real p0 (ds) in future p0 (es)
	mov	ax,BEGIN_IMAGE_REAL_FIRST_PG
	mov	ds,ax
	mov	ax,img_p0
	mov	es,ax

	CORR	v_10,4
	CORR	v_1d,4
	CORR	v_1f,4
	CORR	v_43,4
	cmp	word ptr ds:v_6d[2],0F000h	; to what ROM int 6D points ?
	jae	VIDEO_corr_seg40
	CORR	v_6d,4		; correct int 6D only if its segment < F000

VIDEO_corr_seg40:
	mov	ax,BEGIN_IMAGE_REAL_FIRST_PG + 40h
	mov	ds,ax
	mov	ax,seg40
	mov	es,ax

	CORR	video_mode,<(offset @gen_io_ptr - offset video_mode)>
	CORR	video_rows,<(offset dsk_data_rate - offset  video_rows)>
	CORR	@video_sav_tbls,4

	ret

;==========================================================
;-------------- stack for MENU monitor calls --------------
; skip approx. 380h bytes
;-------------------------

	org	BEGIN_OFFS + 500h

stack_for_int10h	label word
;==========================================================


;==========================================================
;---------------- miscellaneous work DATA -----------------

lpt1buf	dw	LEN_PRINT_BUF,0
	db	LEN_PRINT_BUF dup(0)
lpt2buf	dw	LEN_PRINT_BUF,0
	db	LEN_PRINT_BUF dup(0)
lpt3buf	dw	LEN_PRINT_BUF,0
	db	LEN_PRINT_BUF dup(0)
;-----------------------------------
_error_text	db     l_error_text	dup(0)
;-----------------------------------
_numFLs		db	0
_FL_types	db	2,4,0,0		; FL_types[4]
;==========================================================

;==========================================================
;---- area to allocate all other segments excl. _TEXT -----

		align 16
first_label	label	byte
		dd	1024 dup (0)
last_label	label	byte
;==========================================================


;==========================================================
;--------------------- other SEGMENTS ---------------------
;==========================================================

;-----------------------------------
_data segment para public 'DATA'

	org (offset first_label - offset bg_text)

_data ends
;-----------------------------------
const	segment  word public 'CONST'
const	ends
;-----------------------------------
_bss	segment  word public 'BSS'
_bss	ends
;-----------------------------------
img_p0	segment	  para public
img_p0	ends
;-----------------------------------
seg40	segment	  para public

_beg_seg40	label	word
beg_seg40	label	word

	include	barby40.asm

end_seg40	label	word

seg40	ends
;-----------------------------------
last segment para public 'last'
last ends
;-----------------------------------
DGROUP	GROUP	CONST, _BSS, _DATA

;==========================================================
;--------------- end of other SEGMENTS --------------------
;==========================================================
;------------------- _TEXT continues -----------------------
;==========================================================


;==========================================================
;---------------- warm start of ROM BIOS ------------------
;==========================================================

start_my_rom:
my_int_19h:

;--- disable interrupts and set stack
	cli
	mov	ax,INIT_SS
	mov	ss,ax
	mov	sp,INIT_SP

;--- set interript vectors (from image)
	lds	si,dword ptr cs:addr_im_p0  ; address of image of seg at 0
	mov	cx,0200h
	xor	di,di
	mov	es,di
	rep	movsw

;--- fill ROM's data segment at 40h (from image)
	mov	si,40h
	mov	es,si
	lds	si,dword ptr cs:addr_im_seg40  ; address of image of seg at 40
	mov	cx,(offset end_seg40 - offset beg_seg40+1)/2
	xor	di,di
	rep	movsw

;--- tell monitor that memory is initialized
	mov	al,0
	mov	dx,PORT_FOR_SASHA
	out	dx,al

;--- initialize monitors's (!) video software and menu system
	call	far ptr _InfoVGAInit

;--- set initial video mode -- text 80x25
	mov	es:video_mode,2	; to ensure VGA will set mode fully
	mov	ax,3
	int	10h

;--- mask off all irq exept of timer, keyboard and slave 8259
	mov	al,0F8h
	out	21h,al
	mov	al,0FFh
	out	0A1h,al

;--- equipment & so on,	make changes in real page 0 at 0
	call	mk_env		

;--- boot operating system

int_19h_bootup:
	les	bx,dword ptr cs:addr_0000_7c00	
	mov	dl,cs:bootd
	xor	dh,dh
	cmp	dl,0
	jnz	to_BOOTING_from_HD
;----------------------------------------------	for booting from a:
	mov	ax,0h
	int	13h			
;----------------------------------------------

	mov	cx,1
	mov	ax,201h
	int	13h			
	jnc	short ok_rt0s1HD	
	jmp	short to_BOOTING_from_HD	

;-----------------------------------
addr_im_p0   	  dd	_beg_im_p0	
addr_im_seg40     dd	_beg_seg40
;-----------------------------------

to_BOOTING_from_HD:
	mov	dx,80h
	mov	cx,1
	mov	ax,201h
	int	13h			
	jnc	short ok_rt0s1HD	
	jmp	short to_int18h		
ok_rt0s1HD:
	cmp	word ptr es:AA55_in_boot[bx],0AA55h
	jne	to_int18h
	jmp	dword ptr cs:addr_0000_7c00

to_int18h:
	int	18h			; ROM BASIC
	jmp	int_19h_bootup

;-----------------------------------
; Read CMOS for initial settings
mk_env:	
 	call	_main
	ret

;==========================================================
;----------- unused and some other INT handlers -----------
;==========================================================

assume ds:seg40

;-----------------------------------

	UNEXP_HW_INT_ENTRY	0Ah,1
	UNEXP_HW_INT_ENTRY	0Bh,1
	UNEXP_HW_INT_ENTRY	0Ch,1
	UNEXP_HW_INT_ENTRY	0Dh,1
	UNEXP_HW_INT_ENTRY	0Fh,1

	UNEXP_HW_INT_ENTRY	71h,2
	UNEXP_HW_INT_ENTRY	72h,2
	UNEXP_HW_INT_ENTRY	73h,2
	UNEXP_HW_INT_ENTRY	74h,2
	UNEXP_HW_INT_ENTRY	77h,2

unexp_hw_int_1:
	sub	ah,8
	mov	cl,ah
	mov	ah,1
	shl	ah,cl
	mov	al,0Bh		; read ISR from 8259 master
	out	20h,al
	in	al,20h
	test	al,ah		; is the corresponding bit set
	jz	unexp_soft_int
	mov	al,20h
	out	20h,al
	jmp	unexp_int_flag

unexp_hw_int_2:
	sub	ah,70h
	mov	cl,ah
	mov	ah,1
	shl	ah,cl
	mov	al,0Bh		; read ISR from 8259 master
	out	0A0h,al
	in	al,0A0h
	test	al,ah		; is the corresponding bit set
	jz	unexp_soft_int
	mov	al,20h
	out	0A0h,al
	out	20h,al
	jmp	unexp_int_flag

unexp_soft_int:
	mov	ah,0FFh
unexp_int_flag:
	push	ds
	mov	ds,word ptr cs:seg_40h
	mov	ds:gen_int_occured,ah
	pop	ds
	pop	cx
	pop	ax
	iret
;-----------------------------------
int_unused:
	iret

;-----------------------------------
int_return:
	iret				

;-----------------------------------
my_int_18h:    	
	mov	al,18h
	mov	dx,PORT_FOR_SASHA
	out	dx,al
	cli
	jmp	$
	iret
;==========================================================

seg_40h		db	40h
addr_0000_7c00	dw	7C00h	; }  one
seg_0000	dw	0	; } dword


;==========================================================
;------ INT 11 (equipment), INT 12 (memory size) and ------
;-------- INT 15 (miscellaneous service) handlers ---------
;==========================================================

my_int11h:
	push	ds
	mov	ds,word ptr cs:seg_40h
	mov	ax,equip_bits
	pop	ds
	iret

;-----------------------------------
my_int12h:
	push	ds
	mov	ds,word ptr cs:seg_40h
	mov	ax,main_ram_size
	pop	ds
	iret

;-----------------------------------
;--- there is only ah=C0 (get configuration) service here
my_int_15h:
	cmp	ah,0C0h
	jne	short to__int15h
	les	bx,dword ptr cs:addr_Config_tbl
	xor	ah,ah		;CY = 0 !
	retf	2
to__int15h:
	jmp	int_15h_servics

assume ds:nothing
;==========================================================


;==========================================================
;--------------- System Configuration Table ---------------
;==========================================================

addr_Config_tbl	dd	Config_tbl_size	  ;original at F000:E6F5 ??????????

;-----------------------------------
Config_tbl_size	dw	8		; Size of table in bytes
Config_model	db	0FCh		; Model type
			;   0F8h = 80386 model 70-80 types
			;   0FCh = 80286 model 50-60 types
			;          also most 80286/80386
			;          compatibles
			;   0FAh = 8088/86 model 25-30 type
Config_sub_mode	db	0		; Sub-Model type
Config_BIOS_rev	db	0		; BIOS revsion number
Config_features	db	70h		; Feature information
			;   bit 7=1, hard disk uses DMA 3
			;   bit 6=1, dual interrupt chips
			;   bit 5=1, has real-time-clock
			;   bit 4=1, int 15h, ah=4Fh is
			;            supported (keyboard)
			;   bit 3=1, external wait support
			;   bit 2=1, has extended BIOS RAM
			;   bit 1=1, micro-channel
			;   bit 0=1, unused
Config_info_byt	db	0, 0, 0, 8	; Information bytes (future use)
;==========================================================


;==========================================================
;-------------------- INT 13 handler ----------------------
;==========================================================
my_int_13h:
	mov	byte ptr cs:fun_on_entry,ah
	mov	byte ptr cs:dsk_on_entry,dl
	;
	pushf	
	mov	word ptr cs:r_ax,ax
	pop	ax  ; value for flags
	mov	word ptr cs:r_flags,ax
	mov	word ptr cs:r_bx,bx
	mov	word ptr cs:r_cx,cx
	mov	word ptr cs:r_dx,dx
	mov	word ptr cs:r_bp,bp
	mov	word ptr cs:r_si,si
	mov	word ptr cs:r_di,di
	mov	word ptr cs:r_ds,ds
	mov	word ptr cs:r_es,es

;-----------------------------------
to_sasha:
	mov	dx,PORT_FOR_SASHA
	mov	al,13h
	out	dx,al
;-----------------------------------
ok_in_monitor:

	mov	bx,word ptr cs:r_bx
	mov	cx,word ptr cs:r_cx
	mov	dx,word ptr cs:r_dx
	mov	bp,word ptr cs:r_bp
	mov	si,word ptr cs:r_si
	mov	di,word ptr cs:r_di
	mov	ds,word ptr cs:r_ds
	mov	es,word ptr cs:r_es
	mov	ax,word ptr cs:r_flags
	push	ax  		; value for flags
no_fun_8:
	popf
  	sti
	mov	ax,word ptr cs:r_ax
	retf	2
;==========================================================


;==========================================================
;---------------------- DISK tables -----------------------
;==========================================================

afld_t0		dd	fld_t1
addr_ADDR_fl_PARAMS   LABEL DWORD
afld_t1		dd	fld_t1
afld_t2		dd	fld_t2
afld_t3		dd	fld_t3
afld_t4		dd	fld_t4

;--------------------------------------------------
;--------------- floppy disk tables ---------------

tabl_fl_par	label	byte

;-----------------------------------
fld_t1	label	byte
db	0DFh, 02h, 25h, 2, 9, 2Ah,0FFh, 50h,0F6h, 0Fh, 08h,	27h, 40h

;-----------------------------------
;-- type2                                                                          
fld_t2		label	byte
DSK_INFO_1	label	byte

First_Specify_Byte        db	0dfh	;0    1 byte      First Specify Byte
Second_Specify_Byte       db    2       ;1    1 byte      Second Specify Byte
Motor_off_ticks		  db	25h	;2    1 byte      Number of Timer Ticks to Wait Prior to
			                ;                 Turning Diskette Drive Motor Off
Bytes_Per_Sector          db	2      	;3    1 byte      Number of Bytes Per Sector
                                        ;  00H = 128 Bytes Per Sector
                                        ;  01H = 256 Bytes Per Sector
                                        ;  02H = 512 Bytes Per Sector
                                        ;  03H = 1024 Bytes Per Sector
Sectors_Per_Track         db	15      ;4    1 byte      Sectors Per Track
Gap                       db	1bh     ;5    1 byte      Gap Length
Dtl                       db	0ffh	;6    1 byte      Dtl (Data Lenght)
Gap_for_Format            db	054h    ;7    1 byte      Gap Lenght for Format
Fill_Byte_for_Format      db	0f6h    ;8    1 byte      Fill Byte for Format
Head_Settle_Time          db	0fh    	;9    1 byte      Head Settle Time (Milliseconds)
Motor_Startup_Time        db    08h   	;10   1 byte      Motor Startup Time (1/8 Seconds)
                                        ; For Example: 8 = 1 Second Wait
			  db	4fh,0	                                        ;
;-----------------------------------
fld_t3	label	byte
db	0DFh, 02h, 25h, 2, 9, 2Ah,0FFh, 50h,0F6h, 0Fh, 08h,     4Fh, 80h
;-----------------------------------
fld_t4	label	byte	;3" 1,44 Mb   t4
db	0DFh, 02h, 25h, 2,18, 1Bh,0FFh, 6Ch,0F6h, 0Fh, 08h	,4fh,00
;--------------------------------------------------


;--------------------------------------------------
;---------------- hard disk tables ----------------

;---- HD1 parm (int 41h) ---- type = 2 
HD1_parm 	label	word
		dw	615	      ;  Number of cylinders, hdsk_type_0
		db 	4	      ;  Number of heads
		dw	0	      ;  Low write current cyl begin *
		dw	300	      ;  Write pre-compensation cylinder
		db	0, 0, 0, 0 ,0
		dw	615	      ;	hdsk_parkng_cyl
		db	17	      ;	hdsk_sectr_trac
		db	0	      ;	hdsk_unused	

;===========	HD2 parm  ===========(int 46h)	type = 1
HD2_parm 	label	word

hdsk_cylinders	dw	306		;  Number of cylinders, hdsk_type_0
hdsk_heads	db	4		;  Number of heads
hdsk_lo_wrt_cyl	dw	0		;  Low write current cyl begin *
hdsk_precompcyl	dw	128		;  Write pre-compensation cylinder
hdsk_err_length	db	0		;  Error correction burst length *
hdsk_misl_bits	db	0		;  Miscellaneous bit functions:
hdsk_timeout	db	0		;  Normal timeout *
hdsk_fmt_timout	db	0		;  Format timeout *
hdsk_chk_timout	db	0		;  Check timeout  *
hdsk_parkng_cyl	dw	305		;  Parking cylinder number
hdsk_sectr_trac	db	17		;  Number of sectors per track
hdsk_unused	db	0		;  Unused
;					;     * indicates XT machines only
;==========================================================

;==========================================================
;------------------ end of _TEXT segment ------------------
;==========================================================
_text	ends
;==========================================================


;==========================================================
;------------- SEGMENT of interrupt VECTORS ---------------
;==========================================================

img_p0	segment	 para public

; (UPPER CASE names are CONSTANTS)

_beg_im_p0	label	dword

;-----------------------------------int 0 -- 3
	dd	int_unused
	dd	int_unused
	dd		INT_2_ENTRY
	dd	int_unused
;-----------------------------------int 4 -- 7
	dd	int_unused
	dd		INT_5_ENTRY
	dd	int_unused
	dd	int_unused
;-----------------------------------int 8 -- F (hardware)
	dd		INT_8_ENTRY
	dd		INT_9_ENTRY
	dd	unexp_hw_int_0Ah
	dd	unexp_hw_int_0Bh
	dd	unexp_hw_int_0Ch
	dd	unexp_hw_int_0Dh
	dd		INT_0EH_ENTRY
	dd	unexp_hw_int_0Fh
;-----------------------------------int 10 -- 1F
v_10	dd	int_10h_video
	dd		INT_11H_ENTRY
	dd		INT_12H_ENTRY
	dd	my_int_13h     
	dd		INT_14H_ENTRY
	dd		INT_15H_ENTRY
	dd		INT_16H_ENTRY
	dd		INT_17H_ENTRY
	dd	my_int_18h
	dd		INT_19H_ENTRY
	dd		INT_1AH_ENTRY
	dd	int_return		; 1B
	dd	int_return		; 1C
v_1d	dd	Video_table_1d		; video parms
	dd		DSK_INFO_1		;PTR
v_1f	dd	Video_table_1f		; CGA font
;-----------------------------------int 20 -- 3F (DOS)
	dd	20h dup (int_unused)
;-----------------------------------int 40 -- 47
;??????????????????????????????????????????????????????????????????
	dd	int_unused;	INT_13H_FLOPPY	; FFF  old int13h ?
;??????????????????????????????????????????????????????????????????
	dd	HD1_parm
	dd		INT_10H_ENTRY	; int 42 = older int 10
v_43	dd	Video_43h
	dd	int_unused
	dd	int_unused
	dd	HD2_parm	; FFF or no FFF question ???????????
	dd	int_unused

;-----------------------------------int 48 -- 5F
	dd	18h dup (int_unused)
;-----------------------------------int 60 -- 67
	dd	8 dup (0)
;-----------------------------------int 68 -- 6F
	dd	5 dup (int_unused)
v_6d	dd	int_unused	; used by some VGAs
	dd	2 dup (int_unused)
;-----------------------------------int 70 -- 77 (hardware)
	dd	int_70h_clock
	dd	unexp_hw_int_71h
	dd	unexp_hw_int_72h
	dd	unexp_hw_int_73h
	dd	unexp_hw_int_74h
	dd	int_75h_80287
	dd	int_76h
	dd	unexp_hw_int_77h
;-----------------------------------int 78 -- FE
	dd	87h dup(0)
;-----------------------------------int FF
	dw	BEGIN_OFFS
	dw	_TEXT
;==========================================================
img_p0	ends
;==========================================================
;--------- end of SEGMENT of interrupt VECTORS ------------
;==========================================================


;==========================================================
end


