;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;

@rs232_port_1	dw	3F8h			; RS-232 port addresses
@rs232_port_2	dw	0
@rs232_port_3	dw	0
@rs232_port_4	dw	0
@prn_port_1	dw	3BCh			; Printer port addresses
@prn_port_2	dw	0
@prn_port_3	dw	0
BIOS_data_seg	dw	0			; Extended BIOS data (PS/2)or printer
						;   Printer 4 (PC,XT,AT & compatibles)
equip_bits	dw	?			; pp00 sss0 ffvv rrmF 

init_test_flag	db	0B0h			; Initialization test

main_ram_size	dw	MEM_SIZE		; Base memory size 0-1Meg, 1K steps

chan_io_size	dw	0			; Channel i/o size

;iniatial NUMLOCK is OFF	;   7   6   5   4    3   2   1   0
				; ins- cap num scrl alt ctl lef rig
keybd_flags_1	db	INITIAL_NUMLOCK_STATUS
keybd_flags_2	db	0	
keybd_alt_num	db	0	

keybd_q_head	dw	1eh
keybd_q_tail	dw	1eh
keybd_queue	dw	16 dup(0)

;===============================================================
dsk_recal_stat	db 0	;  Recalibrate floppy drive bits	 [1]
			;     3       2       1       0
			;  drive-3 drive-2 drive-1 drive-0

			;  bit 7 = interrupt flag
			;
dsk_motor_stat	db 0    ; Motor running status & disk write
			;  bit 7=1 disk write in progress
			;  bits 6&5 = drive selected 0 to 3
			;     3       2       1       0
			;  drive-3 drive-2 drive-1 drive-0
			;  --------- 1=motor on-----------

dsk_motor_tmr	db 0    ; Motor timer, at 0, turn off motor   [0fch]
dsk_ret_code	db 0    ; Controller return code
			;  00h = ok
			;  01h = bad command or parameter
			;  02h = can't find address mark
			;  03h = can't write, protected dsk
			;  04h = sector not found
			;  08h = DMA overrun
			;  09h = DMA attempt over 64K bound
			;  10h = bad CRC on disk read
			;  20h = controller failure
			;  40h = seek failure
			;  80h = timeout, no response
;Seven Status bytes - disk controller chip
dsk_status_1	db	0	
dsk_status_2	db	0
dsk_status_3	db	0
dsk_status_4	db	0
dsk_status_5	db	0
dsk_status_6	db	2
dsk_status_7	db	2

; =====================  VIDEO  =======================================

video_mode	db	3	; Present display mode(see int 10h)
video_columns	dw	50h	; Number of columns

video_buf_siz	dw	1000h	; Video buffer size in bytes
;???????????????????????????????????????????????????? why 0
video_segment	dw	0	; Segment of active video memory
				;   MDA=0B000h, CGA=0B800h, etc.

vid_curs_pos0	dw	184Fh	; Cursor position page 0
				;   bits 15-8=row, bits 7-0=column
vid_curs_pos1	dw	0	; Cursor position page 1
vid_curs_pos2	dw	0	; Cursor position page 2
vid_curs_pos3	dw	0	; Cursor position page 3
vid_curs_pos4	dw	0	; Cursor position page 4
vid_curs_pos5	dw	0	; Cursor position page 5
vid_curs_pos6	dw	0	; Cursor position page 6
vid_curs_pos7	dw	0	; Cursor position page 7

vid_curs_mode	dw	0d0eh	; Active cursor, start & end lines
				;   bits 12 to 8 for starting line
				;   bits 4  to 0 for ending line
video_page	db	0	; Present page

@video_port	dw	3D4h	; Video controller base I/O address

video_mode_reg	db	29h	; Hardware mode register bits
video_color	db	30h	; Color set in CGA modes
; =====================================================================

;=====================================================================================
;=================================================================
Adapters_Initialize	equ 0fff0h		;prosto tak ??????????
;----------------------------------------------------
@gen_io_ptr	dw	Adapters_Initialize	; ROM initialization pointer
@gen_io_seg	dw	0F000h			; ROM i/o segment
;=================================================================

gen_int_occured	db	0		; Unused interrupt occurred
;----------------------------------------------------------
timer_low	dw	7C3Dh		; Timer, low word, cnts every 55 ms
timer_hi	dw	12h		; Timer, high word
timer_rolled	db	0		; Timer overflowed, non-zero when
					;  more than 24 hours have elapsed
;----------------------------------------------------------
keybd_break	db	0		; Bit 7 set if break key depressed
warm_boot_flag	dw	0		; Boot (reset) type 1234h=warm boot, no memory test
					;  4321h=boot & save memory
;-----------------------------------------------------------------
hdsk_status_1	db	0		; Hard disk status
hdsk_count	db	?		; Number of hard disk drives
;-----------------------------------------------------------------
hdsk_head_ctrl	db	0		; Head control (XT only)
hdsk_ctrl_port	db	0		; Hard disk control port (XT only)
;-----------------------------------------------------------------
;  I/O PORT TIMEOUTS
prn_timeout_1	db	14h		; Countdown timer waits 
prn_timeout_2	db	14h		; for printer ports
prn_timeout_3	db	14h
prn_timeout_4	db	14h
rs232_timeout_1	db	1		; Countdown timer 
rs232_timeout_2	db	1		; waits for RS-232
rs232_timeout_3	db	1		; port to respond 
rs232_timeout_4	db	1
;-----------------------------------------------------------
@keybd_begin	dw	1Eh		; Ptr to beginning of keybd queue
@keybd_end	dw	3Eh		; Ptr to end of keyboard queue


; =====================  VIDEO  ========================================

video_rows	db	18h			; Rows of characters on display - 1
video_pixels	dw	10h			; Number of pixels per charactr * 8
video_options	db	60h   ; [62h]		; Display adapter options  256K
;--------------------------------------------------------------------------
video_switches	db	9h    ; [0bh]		; Switch setting bits from adapter

video_1_reservd	db	011h  ; [17h]		; Video reserved 1, EGA/VGA control
						;   bit 7 = 200 line mode
						;   bits 6,5 = unused
						;   bit 4 = 400 line mode
						;   bit 3 = no palette load
						;   bit 2 = mono monitor
						;   bit 1 = gray scale
						;   bit 0 = unused
video_2_reservd	db	0bh   ; [0dh]		; Video reserved 2    
; =====================================================================

;=====================================================================================
dsk_data_rate	db	0		; Last data rate for diskette
hdsk_status_2	db	0		; Hard disk status
hdsk_error	db	0		; Hard disk error
hdsk_int_flags	db	0		; Set for hard disk interrupt flag
hdsk_options	db	0		; Bit 0 = 1 when using 1 controller
					;  card for both hard disk & floppy
hdsk0_media_st	db	15h		; Media state for drive 0
hdsk1_media_st	db	4		; Media state for drive 1

	;     7      6      5      4	     3      2      1      0
	;  data xfer rate  two   media	  unused  -----state of drive-----
	;   00=500K bit/s  step  known	          bits floppy  drive state
	;   01=300K bit/s		          000=  360K in 360K, ?
	;   10=250K bit/s		          001=  360K in 1.2M, ?
	; 				          010=  1.2M in 1.2M, ?
	; 				          011=  360K in 360K, ok
	; 				          100=  360K in 1.2M, ok
	; 				          101=  1.2M in 1.2M, ok
	; 				          111=  state not defined

hdsk0_start_st	db	0		; Start state for drive 0
hdsk1_start_st	db	0		; Start state for drive 1
hdsk0_cylinder	db	0		; Track number for drive 0
hdsk1_cylinder	db	0		; Track number for drive 1

; =================================
; =    ADVANCED KEYBOARD DATA     =
; =================================

keybd_flags_3	db	10h		; Special keyboard type and mode
					;  bit 7 Reading ID of keyboard
					;      6 last char is 1st ID char
					;      5 force num lock
					;      4 101/102 key keyboard
					;      3 right alt key down
					;      2 right ctrl key down
					;      1 E0h hidden code last
					;      0 E1h hidden code last
					;
keybd_flags_4	db	12h		; Keyboard Flags (advanced keybd)


; =================================
; =  REAL-TIME CLOCK & LAN DATA   =
; =================================

@timer_wait_off	dw	0		; Ptr offset to wait done flag
@timer_wait_seg	dw	0		; Ptr segment to wait done flag
;---------------------------------------------------------------
timer_clk_low	dw	0		; Timer low word, 1 microsecond clk
timer_clk_hi	dw	0		; Timer high word
timer_clk_flag	db	0		; Timer flag 00h = post acknowledgd
					;            01h = busy
					;            80h = posted
;---------------------------------------------------------------
lan_1		db	0		; Local area network bytes (7)
lan_2		db	0
lan_3		db	0
lan_4		db	0
lan_5		db	0
lan_6		db	0
lan_7		db	0


; =====================  VIDEO  ========================================

@video_sav_tbls	dd   VGA_SAVE_PTR_at_a8	  ; Pointer to a save table 

;            SAVE TABLE			 ---->	  2ND SAVE TABLE 
;  offset type    pointer to		 |	;  offset type functions & pointers
;  컴컴컴 컴컴 컴컴컴컴컴컴컴컴컴컴	 |	;  컴컴컴 컴컴 컴컴컴컴컴컴컴컴컴컴
;    0     dd  Video parameters		 |	;    0     dw  Bytes in this table
;    4     dd  Parms save area	 --------	;    2     dd  Combination code tbl
;    8     dd  Alpha char set			;    6     dd  2nd alpha char set
;   0Ch    dd  Graphics char set		;   0Ah    dd  user palette tbl
;   10h    dd  2nd save ptr table		;   0Eh    dd  reserved (0:0)
;   14h    dd  reserved (0:0)			;   12h    dd  reserved (0:0)
;   18h    dd  reserved (0:0)			;   16h    dd  reserved (0:0)
;
; =====================================================================

		db	84 dup (0)
prn_scrn_stat_b	db	0


