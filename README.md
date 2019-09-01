# pjd-os-source

This repo contains the source code for PJD OS

Source code for the bootloader:

	BITS 16

	jmp short bootloader_start
	nop

OEMLabel		db "PJDOSBOOT"
BytesPerSector		dw 512
SectorsPerCluster	db 1
ReservedForBoot		dw 1
NumberOfFats		db 2
RootDirEntries		dw 224

LogicalSectors		dw 2880
MediumByte		db 0F0h
SectorsPerFat		dw 9
SectorsPerTrack		dw 18
Sides			dw 2
HiddenSectors		dd 0
LargeSectors		dd 0
DrieNo			dw 0
Signature		db 41
VolumeID		dd 00000000h
VolumeLabel		db "PJDOS"
FileSystem		db "FAT12"

bootloader_start:
	mov ax, 07C0h
	add ax, 544
	cli
	mov ss, ax
	mov sp, 4096
	sti
	mov ax, 07C0h
	mov ds, ax
	mov byte [bootdev], dl
	mov eax, 0

floppy_ok:
	mov ax, 19
	call l2hts
	mov si, buffer
	mov bx, ds
	mov es, bx
	mov bx, si
	mov ah, 2
	mov al, 14
	pusha

read_root_dir:
	popa
	pusha
	stc
	int 13h
	jnc search_dir
	call reset_floppy
	jnc read_root_dir
	jmp reboot

search_dir:
	popa
	mov ax, ds
	mov es, ax
	mov di, buffer
	mov cx, word [RootDirEntries]
	mov ax, 0

next_root_entry:
	xchg cx, dx
	mov si, kern_filename
	mov cx, 11
	rep cmpsb
	je found_file_to_load
	add ax, 32
	mov di, buffer
	add di, ax
	xchg dx, cx
	loop next_root_entry
	mov si, file_not_found
	call print_string
	jmp reboot

found_file_to_load:
	mov ax, word [es:di+0FH]
	mov word [cluster], ax
	mov ax, 1
	call l2hts
	mov di, buffer
	mov bx, di
	mov ah, 2
	mov al, 9
	pusha

read_fat:
	popa
	pusha
	stc
	int 13h
	jnc read_fat_ok
	call reset_floppy
	jnc read_fat
	mov si, disk_error
	call print_string
	jmp reboot

read_fat_ok:
	popa
	mov ax, 2000h
	mov es, ax
	mov bx, 0
	mov ah, 2
	mov al, 1
	push ax

load_file_sector:
	mov ax, word [cluster]
	add ax, 31
	call l2hts
	mov ax, 2000h
	mov es, ax
	mov bx, word [pointer]
	pop ax
	push ax
	stc
	int 13h
	jnc calculate_next_cluster
	call reset_floppy
	jmp load_file_sector

calculate_next_cluster:
	mov ax, [cluster]
	mov dx, 0
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	mov si, buffer
	add si, ax
	mov ax, word [ds:si]
	or dx, dx
	jz even

odd:
	shr ax, 4
	jmp short next_cluster_cont

even:
	and ax, 0FFFh

next_cluster_cont:
	mov word [cluster], ax
	cmp ax, 0FF8h
	jae end
	add word [pointer], 512
	jmp load_file_sector

end:
	pop ax
	mov dl, byte [bootdev]
	jmp 2000h:0000h
	push ax

reboot:
	mov ax, 0
	int 16h
	mov ax, 0
	int 19h

print_string:
	pusha
	mov ah, 0Eh

.repeat:
	lodsb
	cmp al, 0
	je .done
	int 10h
	jmp short .repeat

.done:
	popa
	ret

reset_floppy:
	push ax
	push dx
	mov ax, 0
	mov dl, byte [bootdev]
	stc
	int 13h
	pop dx
	pop ax
	ret

l2hts:

	push bx
	push ax
	mov bx, ax
	mov dx, 0
	div word [SectorsPerTrack]
	add dl, 01h
	mov cl, dl
	mov ax, bx
	mov dx, 0
	div word [SectorsPerTrack]
	mov dx, 0
	div word [Sides]
	mov dh, dl
	mov ch, al
	pop ax
	pop bx
	mov dl, byte [bootdev]
	ret


	kern_filename	db "KERNEL  BIN"
	disk_error	db "Error 3:Unknown disk error!", 0
	file_not_found	db "Error 1:PJD OS Kernel not found!", 0

	bootdev		db 0
	cluster		dw 0
	pointer		dw 0

	times 510-($-$$) db 0
	dw 0AA55h
BITS 16

	%DEFINE PJDOS_VER '1.0'
	%DEFINE PJDOS_API_VER 5

	disk_buffer	equ	24576

os_call_vectors:
	jmp os_main
	jmp os_move_cursor
	jmp os_clear_screen
	jmp os_print_horiz_line
	jmp os_print_newline
	jmp os_wait_for_key
	jmp os_check_for_key
	jmp os_int_to_string
	jmp os_speaker_tone
	jmp os_speaker_off
	jmp os_load_file
	jmp os_pause
	jmp os_fatal_error
	jmp os_draw_background
	jmp os_string_length
	jmp os_string_uppercase
	jmp os_string_lowercase
	jmp os_input_string
	jmp os_string_copy
	jmp os_dialog_box
	jmp os_string_join
	jmp os_get_file_list
	jmp os_string_compare
	jmp os_string_chomp
	jmp os_string_strip
	jmp os_string_truncate
	jmp os_bcd_to_int
	jmp os_get_time_string
	jmp os_get_api_version
	jmp os_file_selector
	jmp os_get_date_string
	jmp os_send_via_serial
	jmp os_get_via_serial
	jmp os_find_char_in_string
	jmp os_get_cursor_pos
	jmp os_print_space
	jmp os_dump_string
	jmp os_print_digit
	jmp os_print_1hex
	jmp os_print_2hex
	jmp os_print_4hex
	jmp os_long_int_to_string
	jmp os_long_int_negate
	jmp os_set_time_fmt
	jmp os_set_date_fmt
	jmp os_show_cursor
	jmp os_hide_cursor
	jmp os_dump_registers
	jmp os_string_strincmp
	jmp os_write_file
	jmp os_file_exists
	jmp os_create_file
	jmp os_remove_file
	jmp os_rename_file
	jmp os_get_file_size
	jmp os_input_dialog
	jmp os_list_dialog
	jmp os_string_reverse
	jmp os_string_to_int
	jmp os_draw_block
	jmp os_get_random
	jmp os_string_charchange
	jmp os_serial_port_enable
	jmp os_sint_to_string
	jmp os_string_parse
	jmp os_run_basic
	jmp os_port_byte_out
	jmp os_port_byte_in

os_main:
	cli
	mov ax, 0
	mov ss, ax
	mov sp, 0FFFFh
	sti

	cld
	mov ax, 2000h
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ax, 1003h
	mov bx, 0
	int 10h

	call os_seed_random

	mov ax, autorun_bin_file_name
	call os_file_exists
	jc no_autorun_bin

	mov cx, 32768
	call os_load_file
	jmp execute_bin_program

no_autorun_bin:
	mov ax, autorun_bas_file_name
	call os_file_exists
	jc option_screen
	mov cx, 32768
	call os_load_file
	call os_clear_screen
	mov ax, 32768
	call os_run_basic
	jmp app_selectorax
	push ax

option_screen:
	mov ax, os_init_msg
	mov bx, os_version_msg
	mov cx, 10011111b
	call os_draw_background

	mov ax, dialog_string_1
	mov bx, dialog_string_2
	mov cx, dialog_string_3
	mov dx, 1
	call os_dialog_box

	cmp ax, 1
	jne near app_selector

	call os_clear_screen
	call os_command_line

	jmp option_screen

	os_init_msg			db 'Welcome to PJD OS', 0
	os_version_msg			db 'Version ', PJDOS_VER, 0

	dialog_string_1			db 'PJD OS is made by ZeroCool32;', 0
	dialog_string_2			db 'Please select an interface: OK for the', 0
	dialog_string_3			db 'program menu, Cancel for command line.', 0

app_selector:
	mov ax, os_init_msg
	mov bx, os_version_msg
	mov cx, 10011111b
	call os_draw_background

	call os_file_selector

	jc option_screen

	mov si, ax
	mov di, kern_file_name
	call os_string_compare
	jc no_kernel_execute

	push si
	mov bx, si
	mov ax, si
	call os_string_length

	mov si, bx
	add si, ax

	dec si
	dec si
	dec si

	mov di, bin_ext
	mov cx, 3
	rep cmpsb
	jne not_bin_extension
	pop si

	mov ax, si
	mov cx, 32768
	call os_load_file

execute_bin_program:
	call os_clear_screen
	mov ax, 0
	mov bx, 0
	mov cx, 0
	mov dx, 0
	mov si, 0
	mov di, 0

	call 32768

	call os_clear_screen
	jmp app_selector

no_kernel_execute:
	mov ax, kerndlg_string_1
	mov bx, kerndlg_string_2
	mov cx, kerndlg_string_3
	mov dx, 0
	call os_dialog_box
	jmp app_selector

not_bin_extension:
	pop si
	push si

	mov bx, si
	mov ax, si
	call os_string_length

	mov si, bx
	mov si, ax

	dec si
	dec si
	dec si

	mov di, bas_ext
	mov cx, 3
	rep cmpsb
	jne not_bas_extension

	pop si

	mov ax, si
	mov cx, 32768
	call os_load_file

	call os_clear_screen

	mov ax, 32768
	call os_run_basic

	mov si, basic_finished_msg
	call os_print_string
	call os_wait_for_key

	call os_clear_screen
	jmp app_selector

not_bas_extension:
	pop si

	mov ax, ext_string_1
	mov bx, ext_string_2
	mov cx, 0
	mov dx, 0
	call os_dialog_box

	jmp app_selector
	kern_file_name			db 'KERNEL.BIN', 0

	autorun_bin_file_name		db 'AUTORUN.BIN', 0
	autorun_bas_file_name		db 'AUTORUN.BAS', 0

	bin_ext				db 'BIN'
	bas_ext				db 'BAS'

	kerndlg_string_1		db 'Error 1:Cannot load PJD OS kernel!', 0
	kerndlg_string_2		db 'kernel.bin is required to run PJD OS and', 0
	kerndlg_string_3		db 'is not a regular program.', 0

	ext_string_1			db 'Error 2:Invalid filename extension! You can', 0
	ext_string_2			db 'only execute .bin or .bas programs.', 0

	basic_finished_msg		db '>>> BASIC program finished. -- press a key.', 0

	fmt_12_24	db 0
	fmt_date	db 0, '/'





buffer:				





Source code for the kernel:

