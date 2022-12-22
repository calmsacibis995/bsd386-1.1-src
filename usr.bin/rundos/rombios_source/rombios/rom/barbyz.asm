;
; Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
; The Berkeley Software Design Inc. software License Agreement specifies
; the terms and conditions for redistribution.
;

extrn	start_my_rom:far
extrn	int_2h:near
extrn	_int5:near
extrn	int_8_timer:near
extrn	_int9:near
extrn	int_0eh:near
extrn	_int10:near
extrn	my_int11h:near
extrn	my_int12h:near
extrn	_int14:near
extrn	my_int_15h:near
extrn	_int16:near
extrn	_int17:near
extrn	my_int_19h:near
extrn	int_1ah_rtc:near


;----------------------------------------------------------------------
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
;--------------------------------------------------------------------------
;DSK_INFO_1     equ	0f000EFC7h

l_disk_info	equ	0EFC7h
l_cga_parms	equ	0F0A4h
l_crt_char_gen	equ	0FA6Eh
l_orig_vectors	equ	0FEF3h
l_iret_cmd	equ	0FF53h

;---------------------------------------------------------

_text	segment	para common 'CODE'

	org	l_INT_2_ENTRY		; 0E2C3h
	jmp	int_2h

	org	l_INT_5_ENTRY		; 0FF54h
	jmp	_int5

	org	l_INT_8_ENTRY		; 0FEA5h
	jmp	int_8_timer

	org	l_INT_9_ENTRY		; 0E987h
	jmp	_int9

	org	l_INT_0eh_ENTRY		; 0EF57h
	jmp	int_0eh

	org	l_INT_10h_ENTRY		; 0F065h
	jmp	_int10

	org	l_INT_11h_ENTRY		; 0F84Dh
	jmp	my_int11h

	org	l_INT_12h_ENTRY		; 0F841h
	jmp	my_int12h

	org	l_INT_14h_ENTRY		; 0E739h
	jmp	_int14

	org	l_INT_15h_ENTRY		; 0F859h
	jmp	my_int_15h

	org 	l_INT_16h_ENTRY		; 0E82Eh
	jmp	_int16

	org	l_INT_17h_ENTRY		; 0EFD2h
	jmp	_int17

	org	l_INT_19h_ENTRY		; 0E6F2h
	jmp	my_int_19h

	org	l_INT_1ah_ENTRY		; 0FE6Eh
	jmp	int_1ah_rtc

	org	l_cga_parms		; 0F0A4h -- INT_1Dh

	org	l_disk_info		; 0EFC7h -- INT_1Eh

;---------------------------------------------------------------------
; CHARACTER GENERATOR GRAPHICS FOR 320X200 AND 640X200 GRAPHICS      :
;---------------------------------------------------------------------
	org	l_crt_char_gen		; 0FA6Eh -- INT_1Fh

crt_char_gen    label   byte
        DB        000H,000H,000H,000H,000H,000H,000H,000H ;   0   0  BLANK
        DB        07EH,081H,0A5H,081H,0BDH,099H,081H,07EH ;   1   1  
        DB        07EH,0FFH,0DBH,0FFH,0C3H,0E7H,0FFH,07EH ;   2   2  
        DB        06CH,0FEH,0FEH,0FEH,07CH,038H,010H,000H ;   3   3  
        DB        010H,038H,07CH,0FEH,07CH,038H,010H,000H ;   4   4  
        DB        038H,07CH,038H,0FEH,0FEH,07CH,038H,07CH ;   5   5  
        DB        010H,010H,038H,07CH,0FEH,07CH,038H,07CH ;   6   6  
        DB        000H,000H,018H,03CH,03CH,018H,000H,000H ;   7   7  
        DB        0FFH,0FFH,0E7H,0C3H,0C3H,0E7H,0FFH,0FFH ;   8   8  
        DB        000H,03CH,066H,042H,042H,066H,03CH,000H ;   9   9  TAB
        DB        0FFH,0C3H,099H,0BDH,0BDH,099H,0C3H,0FFH ;  10   A  

        DB        00FH,007H,00FH,07DH,0CCH,0CCH,0CCH,078H ;  11   B  
        DB        03CH,066H,066H,066H,03CH,018H,07EH,018H ;  12   C  
        DB        03FH,033H,03FH,030H,030H,070H,0F0H,0E0H ;  13   D  CR
        DB        07FH,063H,07FH,063H,063H,067H,0E6H,0C0H ;  14   E  
        DB        099H,05AH,03CH,0E7H,0E7H,03CH,05AH,099H ;  15   F  
        DB        080H,0E0H,0F8H,0FEH,0F8H,0E0H,080H,000H ;  16  10  
        DB        002H,00EH,03EH,0FEH,03EH,00EH,002H,000H ;  17  11  
        DB        018H,03CH,07EH,018H,018H,07EH,03CH,018H ;  18  12  
        DB        066H,066H,066H,066H,066H,000H,066H,000H ;  19  13  
        DB        07FH,0DBH,0DBH,07BH,01BH,01BH,01BH,000H ;  20  14  
        DB        03EH,063H,038H,06CH,06CH,038H,0CCH,078H ;  21  15  
        DB        000H,000H,000H,000H,07EH,07EH,07EH,000H ;  22  16  
        DB        018H,03CH,07EH,018H,07EH,03CH,018H,0FFH ;  23  17  
        DB        018H,03CH,07EH,018H,018H,018H,018H,000H ;  24  18  
        DB        018H,018H,018H,018H,07EH,03CH,018H,000H ;  25  19  
        DB        000H,018H,00CH,0FEH,00CH,018H,000H,000H ;  26  1A  ^Z
        DB        000H,030H,060H,0FEH,060H,030H,000H,000H ;  27  1B  
        DB        000H,000H,0C0H,0C0H,0C0H,0FEH,000H,000H ;  28  1C  
        DB        000H,024H,066H,0FFH,066H,024H,000H,000H ;  29  1D  
        DB        000H,018H,03CH,07EH,0FFH,0FFH,000H,000H ;  30  1E  
        DB        000H,0FFH,0FFH,07EH,03CH,018H,000H,000H ;  31  1F  
        DB        000H,000H,000H,000H,000H,000H,000H,000H ;  32  20  SPACE
        DB        030H,078H,078H,030H,030H,000H,030H,000H ;  33  21  !
        DB        06CH,06CH,06CH,000H,000H,000H,000H,000H ;  34  22  "
        DB        06CH,06CH,0FEH,06CH,0FEH,06CH,06CH,000H ;  35  23  #
        DB        030H,07CH,0C0H,078H,00CH,0F8H,030H,000H ;  36  24  $
        DB        000H,0C6H,0CCH,018H,030H,066H,0C6H,000H ;  37  25  %
        DB        038H,06CH,038H,076H,0DCH,0CCH,076H,000H ;  38  26  &
        DB        060H,060H,0C0H,000H,000H,000H,000H,000H ;  39  27  '
        DB        018H,030H,060H,060H,060H,030H,018H,000H ;  40  28  (
        DB        060H,030H,018H,018H,018H,030H,060H,000H ;  41  29  )
        DB        000H,066H,03CH,0FFH,03CH,066H,000H,000H ;  42  2A  *
        DB        000H,030H,030H,0FCH,030H,030H,000H,000H ;  43  2B  +
        DB        000H,000H,000H,000H,000H,030H,030H,060H ;  44  2C  ,
        DB        000H,000H,000H,0FCH,000H,000H,000H,000H ;  45  2D  -
        DB        000H,000H,000H,000H,000H,030H,030H,000H ;  46  2E  .
        DB        006H,00CH,018H,030H,060H,0C0H,080H,000H ;  47  2F  /
        DB        07CH,0C6H,0CEH,0DEH,0F6H,0E6H,07CH,000H ;  48  30  0
        DB        030H,070H,030H,030H,030H,030H,0FCH,000H ;  49  31  1
        DB        078H,0CCH,00CH,038H,060H,0CCH,0FCH,000H ;  50  32  2
        DB        078H,0CCH,00CH,038H,00CH,0CCH,078H,000H ;  51  33  3
        DB        01CH,03CH,06CH,0CCH,0FEH,00CH,01EH,000H ;  52  34  4
        DB        0FCH,0C0H,0F8H,00CH,00CH,0CCH,078H,000H ;  53  35  5
        DB        038H,060H,0C0H,0F8H,0CCH,0CCH,078H,000H ;  54  36  6
        DB        0FCH,0CCH,00CH,018H,030H,030H,030H,000H ;  55  37  7
        DB        078H,0CCH,0CCH,078H,0CCH,0CCH,078H,000H ;  56  38  8
        DB        078H,0CCH,0CCH,07CH,00CH,018H,070H,000H ;  57  39  9
        DB        000H,030H,030H,000H,000H,030H,030H,000H ;  58  3A  :
        DB        000H,030H,030H,000H,000H,030H,030H,060H ;  59  3B  ;
        DB        018H,030H,060H,0C0H,060H,030H,018H,000H ;  60  3C  <
        DB        000H,000H,0FCH,000H,000H,0FCH,000H,000H ;  61  3D  =
        DB        060H,030H,018H,00CH,018H,030H,060H,000H ;  62  3E  >
        DB        078H,0CCH,00CH,018H,030H,000H,030H,000H ;  63  3F  ?
        DB        07CH,0C6H,0DEH,0DEH,0DEH,0C0H,078H,000H ;  64  40  @
        DB        030H,078H,0CCH,0CCH,0FCH,0CCH,0CCH,000H ;  65  41  A
        DB        0FCH,066H,066H,07CH,066H,066H,0FCH,000H ;  66  42  B
        DB        03CH,066H,0C0H,0C0H,0C0H,066H,03CH,000H ;  67  43  C
        DB        0F8H,06CH,066H,066H,066H,06CH,0F8H,000H ;  68  44  D
        DB        0FEH,062H,068H,078H,068H,062H,0FEH,000H ;  69  45  E
        DB        0FEH,062H,068H,078H,068H,060H,0F0H,000H ;  70  46  F
        DB        03CH,066H,0C0H,0C0H,0CEH,066H,03EH,000H ;  71  47  G
        DB        0CCH,0CCH,0CCH,0FCH,0CCH,0CCH,0CCH,000H ;  72  48  H
        DB        078H,030H,030H,030H,030H,030H,078H,000H ;  73  49  I
        DB        01EH,00CH,00CH,00CH,0CCH,0CCH,078H,000H ;  74  4A  J
        DB        0E6H,066H,06CH,078H,06CH,066H,0E6H,000H ;  75  4B  K
        DB        0F0H,060H,060H,060H,062H,066H,0FEH,000H ;  76  4C  L
        DB        0C6H,0EEH,0FEH,0FEH,0D6H,0C6H,0C6H,000H ;  77  4D  M
        DB        0C6H,0E6H,0F6H,0DEH,0CEH,0C6H,0C6H,000H ;  78  4E  N
        DB        038H,06CH,0C6H,0C6H,0C6H,06CH,038H,000H ;  79  4F  O
        DB        0FCH,066H,066H,07CH,060H,060H,0F0H,000H ;  80  50  P
        DB        078H,0CCH,0CCH,0CCH,0DCH,078H,01CH,000H ;  81  51  Q
        DB        0FCH,066H,066H,07CH,06CH,066H,0E6H,000H ;  82  52  R
        DB        078H,0CCH,0E0H,070H,01CH,0CCH,078H,000H ;  83  53  S
        DB        0FCH,0B4H,030H,030H,030H,030H,078H,000H ;  84  54  T
        DB        0CCH,0CCH,0CCH,0CCH,0CCH,0CCH,0FCH,000H ;  85  55  U
        DB        0CCH,0CCH,0CCH,0CCH,0CCH,078H,030H,000H ;  86  56  V
        DB        0C6H,0C6H,0C6H,0D6H,0FEH,0EEH,0C6H,000H ;  87  57  W
        DB        0C6H,0C6H,06CH,038H,038H,06CH,0C6H,000H ;  88  58  X
        DB        0CCH,0CCH,0CCH,078H,030H,030H,078H,000H ;  89  59  Y
        DB        0FEH,0C6H,08CH,018H,032H,066H,0FEH,000H ;  90  5A  Z
        DB        078H,060H,060H,060H,060H,060H,078H,000H ;  91  5B  [
        DB        0C0H,060H,030H,018H,00CH,006H,002H,000H ;  92  5C  \
        DB        078H,018H,018H,018H,018H,018H,078H,000H ;  93  5D  ]
        DB        010H,038H,06CH,0C6H,000H,000H,000H,000H ;  94  5E  ^
        DB        000H,000H,000H,000H,000H,000H,000H,0FFH ;  95  5F  _
        DB        030H,030H,018H,000H,000H,000H,000H,000H ;  96  60  `
        DB        000H,000H,078H,00CH,07CH,0CCH,076H,000H ;  97  61  a
        DB        0E0H,060H,060H,07CH,066H,066H,0DCH,000H ;  98  62  b
        DB        000H,000H,078H,0CCH,0C0H,0CCH,078H,000H ;  99  63  c
        DB        01CH,00CH,00CH,07CH,0CCH,0CCH,076H,000H ; 100  64  d
        DB        000H,000H,078H,0CCH,0FCH,0C0H,078H,000H ; 101  65  e
        DB        038H,06CH,060H,0F0H,060H,060H,0F0H,000H ; 102  66  f
        DB        000H,000H,076H,0CCH,0CCH,07CH,00CH,0F8H ; 103  67  g
        DB        0E0H,060H,06CH,076H,066H,066H,0E6H,000H ; 104  68  h
        DB        030H,000H,070H,030H,030H,030H,078H,000H ; 105  69  i
        DB        00CH,000H,00CH,00CH,00CH,0CCH,0CCH,078H ; 106  6A  j
        DB        0E0H,060H,066H,06CH,078H,06CH,0E6H,000H ; 107  6B  k
        DB        070H,030H,030H,030H,030H,030H,078H,000H ; 108  6C  l
        DB        000H,000H,0CCH,0FEH,0FEH,0D6H,0C6H,000H ; 109  6D  m
        DB        000H,000H,0F8H,0CCH,0CCH,0CCH,0CCH,000H ; 110  6E  n
        DB        000H,000H,078H,0CCH,0CCH,0CCH,078H,000H ; 111  6F  o
        DB        000H,000H,0DCH,066H,066H,07CH,060H,0F0H ; 112  70  p
        DB        000H,000H,076H,0CCH,0CCH,07CH,00CH,01EH ; 113  71  q
        DB        000H,000H,0DCH,076H,066H,060H,0F0H,000H ; 114  72  r
        DB        000H,000H,07CH,0C0H,078H,00CH,0F8H,000H ; 115  73  s
        DB        010H,030H,07CH,030H,030H,034H,018H,000H ; 116  74  t
        DB        000H,000H,0CCH,0CCH,0CCH,0CCH,076H,000H ; 117  75  u
        DB        000H,000H,0CCH,0CCH,0CCH,078H,030H,000H ; 118  76  v
        DB        000H,000H,0C6H,0D6H,0FEH,0FEH,06CH,000H ; 119  77  w
        DB        000H,000H,0C6H,06CH,038H,06CH,0C6H,000H ; 120  78  x
        DB        000H,000H,0CCH,0CCH,0CCH,07CH,00CH,0F8H ; 121  79  y
        DB        000H,000H,0FCH,098H,030H,064H,0FCH,000H ; 122  7A  z
        DB        01CH,030H,030H,0E0H,030H,030H,01CH,000H ; 123  7B  {
        DB        018H,018H,018H,000H,018H,018H,018H,000H ; 124  7C  |
        DB        0E0H,030H,030H,01CH,030H,030H,0E0H,000H ; 125  7D  }
        DB        076H,0DCH,000H,000H,000H,000H,000H,000H ; 126  7E  ~
        DB        000H,010H,038H,06CH,0C6H,0C6H,0FEH,000H ; 127  7F  

;---------------------------------------------------------------------

	org	l_orig_vectors		; 0FEF3h

	org	l_iret_cmd		; 0FF53h
	iret

	org	0fff0h
;	jmpf	start_my_rom
		db	0eah
		dw	start_my_rom
		dw	_TEXT
;------------------------------------------------------
;DATE STAMP
;------------------------------------------------------
	org	0fffdh
		db	0ffh,0fch ,0
_text	ends
end

