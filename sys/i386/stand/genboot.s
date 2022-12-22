/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: genboot.s,v 1.1 1993/11/07 23:11:18 karels Exp $
 */

/*
 * sys/i386/stand/genbootblk.c
 *
 * Generic boot block using BIOS disk driver.
 *
 * April 21, 1992
 * Pace Willisson
 * pace@blitz.com
 *
 * Placed in the public domain with NO WARRANTIES, not even the
 * implied warranties for MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.
 *
 * This boot block starts by switching into protected mode, in order to
 * make it easier to use the normal 32 bit assembler.  To do actual
 * disk reads and writes, it switches back to real mode and uses BIOS
 * interrupts 0x13 (disk) and 0x10 (console).  The registers are 
 * preserved accross the switch, so you can just generate the BIOS 
 * arguments into the registers where they will be needed.  The exception
 * is that DS and ES always point to "RELOC", which is where the boot
 * program will get loaded.
 *
 * The actual work done by this bootblock is:  read block 0 of the disk
 * to get the DOS fdisk table.  Search for a parition marked ACTIVE and
 * with the correct partion type.  If none is found, then print a message
 * and loop.  Otherwise, read in 16 blocks starting at the beginning of
 * the specified partition.  (We assume that partitions start on cylinder
 * boundaries, even though the fdisk table is powerful enough to start
 * them anywhere.)  Next, check that the second block just read in contains
 * a BSD style disk label.  If not, print a message and loop.  Finally,
 * jump to RELOC+0x400 to start the second level boot.
 *
 * Temporarily, if no fdisk label is found, assume that unix uses the whole
 * disk, and boot from the first 16 sectors.  This will go away when existing
 * disks have been repartitioned.
 */

/* Compilation flags should define:
 *
 * One of:
 *   FLOPPY
 *   HARD
 *
 * Optional:
 *   UNIT  0 or 1
 */

#include "reboot.h"

/* RELOC should be defined with a -D flag to cc */
#if FLOPPY + HARD != 1
#error "One of FLOPPY or HARD must be defined"
#endif

#ifndef UNIT
#define UNIT 0
#endif

/* this should be removed when all of the old disks have been repartitioned */
#define MISSING_FDISK_OK


#if HARD
#define BIOS_UNIT (0x80 + UNIT)
#else
#define BIOS_UNIT (UNIT)
#endif

#define MY_FDISK_TYPE 0x9f

#define	WD_MAJORDEV	0	/* from conf.c */

#define SECOND_LEVEL_BOOT_START (RELOC + 0x400)

/* Number of bytes to read.  The total read must not cross a track boundary.
 * We could do the conventional 8192 bytes on all devices except 5.25 inch
 * floppies, but then we would need more compilation flags in this program.
 * So far, all of the second level boots fit in 15 sectors.
 */
#ifdef old
#define NSECTORS 15 /* by 512 byte seconds is 7680 bytes */
#else
#define NSECTORS 16 /* by 512 byte seconds is 8192 bytes */
#endif

#define REAL_STACK_SIZE 256
.comm realregs,REAL_STACK_SIZE
.comm real_int_num, 1

/* this code is linked at 0x7c00 and is loaded there by the BIOS */

bootbase:
	/* interrupts off */
	cli

	/* the .short here contains the address and data size overrides,
	 * so we can write a 32 bit instruction and have it executed
	 * in 32 bit mode, even though the processor is running in real mode
	 */
 	.short 0x6667; movl $0x7c00,%esp
	.short 0x6667; movl $_main, %edi /* where to go once in 32 bit mode */

prot_jump:
	/* load gdt */
	.byte 0x2e,0x0f,0x01,0x16 /* lgdt %cs:$imm */
	.word gdtarg

	/* turn on protected mode */
	smsw	%ax
	orb	$1,%al
	lmsw	%ax

	/* flush prefetch queue and load %cs */
	.byte 0xea /* ljmp */
	.word prot
	.word 8
prot:
	/* now in 32 bit mode */
	movl	$0x10,%eax
	movl	%ax,%ds
	movl	%ax,%es
	movl	%ax,%ss

	jmp	*%edi
	/* goes to main the first time, then to the end of real_int */

real_int:
	.comm saved_esp,4
	movl %esp, saved_esp

	movl $ realregs + REAL_STACK_SIZE, %esp

	/* push registers on a 16 bit style stack */
	.byte 0x66; pusha
	.byte 0x66; pushl $ (RELOC / 16) /* real mode ds */
	.byte 0x66; pushl $ (RELOC / 16) /* real mode es */

	/* patch in the desired interrupt number */
	movb real_int_num, %al
	movb %al, int_instruction + 1
	/* a future processor may need some kind of code flush here */

	/* Switch to use16 code segment.  I can't find anything in the
	 * Intel manuals that says you have to do this, but if you don't
	 * then it winds up in some kind of mutant 32 bit real mode
	 * after clearing the PE bit in cr0.
	 */
	.byte 0xea /* ljmp */
	.long switch_to_use16
	.word 0x18
switch_to_use16:

	/* turn off protected mode */
	movl %cr0, %eax
	andb $0xfe, %al
	movl %eax, %cr0

	/* flush prefetch queue and load real mode cs */
	.byte 0xea /* ljmp */
	.word int_real
	.word 0
int_real:
	.short 0x6667; movl $0, %ebx
	mov %bx, %ss
	pop %es
	pop %ds
	popa

	sti
int_instruction:
	int $1 /* interrupt number patched above */
	cli
 skip:
	pusha
	push %es
	push %ds

	.short 0x6667; movl $real_int_return, %edi
	jmp prot_jump

real_int_return:
	/* now in 32 bit mode: pop stack in 16 bit mode */
	.byte 0x66; popa

	movl	saved_esp, %esp
	ret

_main:
#if 0 /* not tested recently */
	movb $ BIOS_UNIT, %dl
	movb $8, %ah
	movb $0x13, real_int_num
	call real_int
	movw %cx, disk_params
	movw %dx, disk_params+2
#endif

	/* save first 8K of memory into video memory, for dos emulation */
	movl	$0,%esi
	movl	$0xb8000+4096, %edi
	movl	$8192, %ecx
	rep
	movsb

#ifdef HARD
	/* if this is a hard disk boot, then read block 0, 
	 * which contains the fdisk table
	 */
	movw $0, %bx			/* offset from start of RELOC */
	movw $0x0201, %ax		/* opcode / number of sectors */
	movw $0x0001, %cx		/* sector number */
	movw $ BIOS_UNIT, %dx	/* head number / unit number */
	movb $0x13, real_int_num
	call real_int

	movl $error_reading_fdisk, msgp
	cmpb $0, %ah
	jnz domsg

	movl $ RELOC + 446, %ebx
	cmpw $0xaa55, 64(%ebx)
	jnz nolabel

	movl $4, %ecx
searchloop:
	cmpb $ MY_FDISK_TYPE, 4(%ebx)
	jnz next

	cmpb $0x80, (%ebx)
	jz found

	cmpb $0, (%ebx)
	jnz nolabel

next:
	addl $16, %ebx
	decl %ecx
	jnz searchloop

nolabel:
#ifdef MISSING_FDISK_OK
	jmp no_fdisk
#else
	movl $no_fdisk_label, msgp
	jmp domsg
#endif

found:
	movb 1(%ebx), %dh
	movzbl 2(%ebx),%ecx
	shll $2,%ecx
	movb 3(%ebx),%cl
	pushl %ecx			/* cyloffset to bootstrap */
	movw 2(%ebx), %cx		/* sec/cyl to bios */
	jmp readin

#endif /* HARD */

no_fdisk:
	/* This is the start of the floppy boot.  We also get here
	 * if we want to boot a hard disk that has no fdisk label.
	 *
	 * Set up for boot at cyl 0, head 0, sec 1
	 */
	movb $0, %dh /* head */
	movw $1, %cx /* cyl / sec */
	pushl $0			/* cyloffset to bootstrap */

	/* fall into readin */

readin:
	movw $ BIOS_UNIT, %dx		/* head number / unit number */
	movw $0, %bx			/* buffer offset */
	movw $ 0x200 + NSECTORS, %ax	/* read opcode and sector count */
	movb $0x13, real_int_num
	call real_int

	movl $error_reading_boot, msgp
	cmpb $0, %ah
	jnz domsg

	/* Check for BSD disklabel magic number.  (We can't use the
	 * define in sys/disklabel.h because it has a cast and the
	 * assembler doesn't understand.)
	 */
	movl $no_bsd_label, msgp
	cmpl $0x82564557, RELOC + 0x200
	jnz domsg

	/* cyloffset was pushed above */
	pushl $MAKEBOOTDEV(WD_MAJORDEV, 0, 0, UNIT, 0)	/* bootdev */
	pushl $RB_AUTOBOOT				/* howto */
	pushl $SECOND_LEVEL_BOOT_START
	ret	/* main (howto, dev, off) */
	/* NOTREACHED */

.comm msgp, 4

domsg:
	/* first, set the attribute of the current charcter */
	movw $0x0920, %ax
	movw $0x0007, %bx
	movw $1, %cx
	movb $0x10, real_int_num
	call real_int

	movl msgp, %eax
	movb (%eax), %al
	cmpb $0, %al
	jz domsg_done

	/* now, output the actual character and update the cursor position */
	movb $0xe, %ah
	movb $0, %bh
	movb $0x10, real_int_num
	call real_int

	incl msgp
	jmp domsg

domsg_done:
	jmp domsg_done


gdt:	.byte 0, 0, 0, 0, 0, 0, 0, 0
	.byte 0xff, 0xff, 0, 0, 0, 0x9f, 0xcf, 0 /* code segment */
	.byte 0xff, 0xff, 0, 0, 0, 0x93, 0xcf, 0 /* data segment */
	.byte 0xff, 0xff, 0, 0, 0, 0x9f, 0x0f, 0 /* 16 bit code segment */
gdt_end:

gdtarg:	.short gdt_end - gdt - 1
	.long gdt /* might be ok to be .short */

error_reading_fdisk:
error_reading_boot:
	.ascii "rderr\0"
no_fdisk_label:
	.ascii "no fdisk label\0"
no_bsd_label:
	.ascii "no bsd label\0"

ebootblkcode:
	. = 510
	.byte 0x55
	.byte 0xaa
ebootblk: 			/* MUST BE EXACTLY 0x200 BIG FOR SURE */
