/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eahaboot.s,v 1.5 1992/12/14 23:19:03 karels Exp $
 */

/*
 * Boot block for an Adaptec 1742A EISA SCSI host adapter.
 */

#include "reboot.h"
#include "i386/include/bootblock.h"
#include "i386/eisa/eisa.h"
#include "i386/eisa/eahareg.h"

#define	BOOTADDR	(SMRELOC)	/* where we load the 15-sector boot */
#define	BOOTSTART	(SMRELOC+0x200)	/* where we start the bootstrap */
#define	BOOTBLOCKS	15
#define	BSIZE		512

#define	PROTENABLE	0x1	/* protection enable bit in cr0 */

#define	CODESEL		0x08	/* code segment selector */
#define	DATASEL		0x10	/* data segment selector */

#define	SD_MAJORDEV	4	/* from conf.c */
#define	EAHA_DRIVER	1	/* from sd.c */

#define	CDB_CMD		0
#define	CMD_READ10	0x28
#define	CDB_LBA		2
#define	CDB_XFERLEN	7

	.text

/*
 * For this first little bit, we execute in 8086 'real' mode.
 * We have to be careful to use only 16-bit instructions
 * until we enable 32-bit protected mode.
 */

	/*
	 * C&G p604: 'Clearing IF is not really needed, since the 80386
	 * reset sequence clears IF also, but it is shown here for emphasis.'
	 */
	cli

	/*
	 * Load the Global Descriptor Table.
	 * Since the assembler won't generate a 16-bit LGDT,
	 * we just use the 32-bit address escape prefix.
	 * Using the CS segment prefix means we don't need to initialize DS.
	 */
	aword
	lgdt %cs:GDTaddr

	/*
	 * Turn on protected mode.
	 */
	smsw %ax
	orw $PROTENABLE,%ax
	lmsw %ax		/* we're in protected mode forever now */
	jmp 0f			/* flush the instruction queue */
0:

	/*
	 * Jump to 32-bit protected code.
	 * We use the 32-bit operand escape,
	 * since the assembler won't generate a 16-bit LJMP.
	 */
	word
	ljmp $CODESEL,$start32	/* jump to 32-bit protected code */

/*
 * We have reached 32-bit mode.
 * Set up selectors and get ready to do some real work.
 */
start32:
	movw $DATASEL,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movl $BOOTADDR,%esp	/* stack grows down from 15-sector boot */

	/* save first 8 KB of memory into video memory, for DOS emulation */
	movl $0,%esi
	movl $0xb8000+4096,%edi
	movl $8192,%ecx
	rep
	movsb

	/* clear BSS */
	leal _edata,%edi
	leal _end,%ecx
	subl %edi,%ecx
	movb $0,%al
	rep
	stosb

	/*
	 * Hunt for the controller slot.
	 * We search EISA space for a matching EISA ID.
	 * We assume that the first controller we find
	 * is the one we booted from (as does higher level code).
	 */
	movb $8,%cl
	xorl %edi,%edi 		/* build bootdev in EDI */
	movl $0x1000+EISA_PROD_ID_BASE,%edx 
0:
	inb %dx,%al		/* fetch the ID for this slot */
	incl %edx
	rorl %cl,%eax
	inb %dx,%al
	incl %edx
	rorl %cl,%eax
	inb %dx,%al
	incl %edx
	rorl %cl,%eax
	inb %dx,%al
	rorl %cl,%eax
	leal _eisa_ids,%esi	/* initialize pointer to list of IDs */
1:
	cmpl %eax,(%esi)	/* try a candidate */
	je 2f			/* if equal, we have a winner */
	addl $4,%esi
	cmpl $0,(%esi)		/* end of list? */
	jne 1b
	addw $0x1000-3,%dx	/* no candidates matched */
	jmp 0b
2:

	/*
	 * Extract our block address from the previous transaction.
	 * Sure hope registers haven't been reset or ECB memory trashed...
	 * We assume that the LUN is always 0 (higher level code does too).
	 */
	movb $EAHA_INTR,%dl
	xorl %eax,%eax
	inb %dx,%al
	andb $EI_TARGMASK,%al
	movl %eax,%edi				/* save target number in EDI */

	/*
	 * Initialize our ECB and read the block 0 DOS label.
	 */
	leal _ecb,%ebx
	movw $ECB_CMD,E_CMD(%ebx)
	movw $F1_DSB|F1_ARS,E_FLAG1(%ebx)	/* no contingent allegiance */
	movw $F2_DAT|F2_DIR,E_FLAG2(%ebx)
	movl $BOOTADDR,E_DATA(%ebx)
	movl $BSIZE,E_DATALEN(%ebx)
	movl $_estatus,E_STATUS(%ebx)
	movl $_sense,E_SENSE(%ebx)
	movb $18,E_SENSELEN(%ebx)
	movb $10,E_CDBLEN(%ebx)
	movb $CMD_READ10,E_CDB+CDB_CMD(%ebx)
	/* sector address defaults to zero */
	movw $(1<<8),E_CDB+CDB_XFERLEN(%ebx)	/* byte-swapped length */

	call _run_scsi_cmd

	/*
	 * Look for an active BSD/386 partition
	 * in the DOS label.
	 */
	xorl %eax,%eax
	leal BOOTADDR,%esi
	cmpw $MB_SIGNATURE,510(%esi)
	jne 3f
	addl $MB_PARTOFF,%esi
	movl $4,%ecx

0:
	cmpb $MBS_BSDI,4(%esi)
	jne 1f
	cmpb $MBA_ACTIVE,(%esi)
	je 2f
	cmpl $0,%eax			/* first BSDI partition? */
	jne 1f
	movl 8(%esi),%eax		/* mbpart.start */
	cmpb $MBA_NOTACTIVE,(%esi)
	jne 3f
1:
	addl $16,%esi
	loop 0b
	jmp 3f

2:
	movl 8(%esi),%eax		/* mbpart.start */
3:
	cmpl $15,%eax			/* start at sector 15? */
	jne 4f
	xorl %eax,%eax			/* for us, 15 means 0 (bootany hack) */
4:

	/* increment past boot block and byte-swap */
	movl %eax,%esi
	incl %eax
	xchgb %al,%ah
	roll $16,%eax
	xchgb %al,%ah

	/*
	 * Read the 15-sector label+bootstrap (finally).
	 */
	movl $BOOTADDR,E_DATA(%ebx)
	movl $BOOTBLOCKS*BSIZE,E_DATALEN(%ebx)
	movl %eax,E_CDB+CDB_LBA(%ebx)
	movw $(BOOTBLOCKS<<8),E_CDB+CDB_XFERLEN(%ebx)

	call _run_scsi_cmd

	/*
	 * Prepare arguments to the bootstrap and jump to it.
	 */
	pushl %esi		/* offset to boot partition */

	/*
	 * If we want to pass a target and adapter type
	 * to the higher level boot code,
	 * we have to build a bootdev cookie.
	 * Currently:
	 *	B_ADAPTOR: driver type (0 = aha, 1 = eaha, ...)
	 *	B_CONTROLLER: target number
	 *	B_UNIT: logical SCSI disk (always 0, sigh)
	 *	B_PARTITION: passed as 0, patched in bootxx
	 *	B_TYPE: always SD_MAJORDEV
	 */
	shll $B_CONTROLLERSHIFT,%edi	/* put target in the right place */
	orl $MAKEBOOTDEV(SD_MAJORDEV, EAHA_DRIVER, 0, 0, 0),%edi
	/* XXX assume partition 0 */
	pushl %edi
	pushl $RB_AUTOBOOT	/* howto */
	pushl $BOOTSTART	/* jump to an absolute address */
	ret

/*
 * Fire off an ECB stored in _ecb.
 * Assumes that EDX contains a port address in our slot.
 * Assumes that EDI contains the target number.
 * Clobbers EDX, ECX and EAX.
 */
	.globl _run_scsi_cmd
_run_scsi_cmd:

	/* clear interrupts */
	movb $EAHA_CTRL,%dl
	movb $EC_HRDY|EC_CLRINT,%al
	outb %al,%dx

	/* wait for controller to come ready */
	movb $EAHA_STAT,%dl
0:
	inb %dx,%al
	andb $ES_EMPTY|ES_BUSY,%al
	cmpb $ES_EMPTY,%al
	jne 0b

	/* stuff ECB address into outbox ports and punch ATTN */
	movb $EAHA_MBOUT,%dl
	movb $8,%cl
	leal _ecb,%eax
	outb %al,%dx
	incl %edx
	shrl %cl,%eax
	outb %al,%dx
	incl %edx
	shrl %cl,%eax
	outb %al,%dx
	incl %edx
	shrl %cl,%eax
	outb %al,%dx
	incl %edx		/* bump port to ATTN register */
	movl %edi,%eax		/* recover saved target number */
	orb $EA_START,%al	/* add in the adapter command */
	outb %al,%dx		/* and run that sucker */

	/* wait for completion */
	movb $EAHA_STAT,%dl
1:
	inb %dx,%al
	testb $ES_INTR,%al
	jz 1b

	/* if we failed (e.g. UNIT ATTENTION), try again */
	movb $EAHA_INTR,%dl
	inb %dx,%al
	andb $EI_STATMASK,%al
	cmpb $EI_SUCCESS,%al
	je 2f
	cmpb $EI_RETRY,%al
	jne _run_scsi_cmd
2:
	ret

	/*
	 * The contents of the Global Descriptor Table.
	 */

#define ATTR0(present, dpl, dtype, type) \
	(((present)<<7)+((dpl)<<5)+((dtype)<<4)+(type))
#define	ATTR1(granularity4k, default32, limit16_19) \
	(((granularity4k)<<7)+((default32)<<6)+(limit16_19))

	.align 2
GDT:
	/* the null segment (selector 0) */
	.long 0x0,0x0
	/* the code segment (selector 0x8) */
	.word 0xffff		/* limit 0:15 = everything */
	.word 0x0		/* base address 0:15 = 0 */
	.byte 0x0		/* base address 16:23 = 0 */
	.byte ATTR0(1,0,1,0xf)	/* present, dpl = 0, dtype = mem, type = R/X */
	.byte ATTR1(1,1,0xf)	/* 4k, 32-bit, limit 16:19 = everything */
	.byte 0x0		/* base address 24:31 = 0 */
	/* the data segment (selector 0x10) */
	.word 0xffff		/* limit 0:15 = everything */
	.word 0x0		/* base address 0:15 = 0 */
	.byte 0x0		/* base address 16:23 = 0 */
	.byte ATTR0(1,0,1,0x3)	/* present, dpl = 0, dtype = mem, type = R/W */
	.byte ATTR1(1,1,0xf)	/* 4k, 32-bit, limit 16:19 = everything */
	.byte 0x0		/* base address 24:31 = 0 */

GDTaddr:
	.word 23		/* sizeof (gdt) - 1 */
	.long GDT		/* &gdt */

	/*
	 * EISA expansion board IDs.
	 */
	.globl _eisa_ids
_eisa_ids:
	.long 0x00009004	/* ADP0000 */
	.long 0x01009004	/* ADP0001 */
	.long 0x02009004	/* ADP0002 */
	.long 0x00049004	/* ADP0400 */
	.long 0

ebootblkcode:

	.org 510
	.word MB_SIGNATURE

	/* bss */
	.comm _ecb,48
	.comm _estatus,32
	.comm _sense,18
