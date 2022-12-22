/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ahaboot.s,v 1.7 1993/04/14 20:17:30 karels Exp $
 */

/*
 * Boot block for an Adaptec 1542[BC] SCSI host adapter.
 * The host adapter is assumed to be at I/O port 0x330 with its BIOS enabled.
 * (We don't actually call the BIOS, but we assume that it configured
 * the DMA channel for us.)
 * Currently only target 0, LUN 0 is supported.
 */

#include "reboot.h"
#include "i386/include/bootblock.h"
#include "i386/isa/isa.h"
#include "i386/isa/ahareg.h"

#define	BOOTADDR	(SMRELOC)	/* where we load the 15-sector boot */
#define	BOOTSTART	(SMRELOC+0x200)	/* where we start the bootstrap */
#define	BOOTBLOCKS	15
#define	BSIZE		512

#define	PROTENABLE	0x1	/* protection enable bit in cr0 */

#define	CODESEL		0x08	/* code segment selector */
#define	DATASEL		0x10	/* data segment selector */

#define	SD_MAJORDEV	4	/* from conf.c */
#define	AHA_DRIVER	0	/* from sd.c */

#define	CDB_CMD		0
#define	CMD_READ10	0x28
#define	CDB_LUN		1
#define	CDB_LBA		2
#define	CDB_XFERLEN	7

#define	BSWAP() \
	xchgb %al,%ah; \
	roll $16,%eax; \
	xchgb %al,%ah

#define	AHA_CONST(c, dst) \
	movl $((c>>16)|(c&0xff00)|((c&0xff)<<16)),dst

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

	/* save first 8K of memory into video memory, for dos emulation */
	movl $0,%esi
	movl $0xb8000+4096,%edi
	movl $8192,%ecx
	rep
	movsb

	/*
	 * Read the 15-sector label+bootstrap sequence.
	 * We set up the controller, then execute a SCSI READ command.
	 */

	/*
	 * Clear BSS.
	 */
	leal _edata,%edi
	leal _end,%ecx
	subl %edi,%ecx
	movb $0,%al
	rep
	stosb

	leal _ccb,%ebx			/* global variable */

#if 0 /* the Adaptec BIOS resets the adapter after reading a boot block */
	/*
	 * See if we can retrieve the target number
	 * of the device we were booted from.
	 */
	movw $AHA_STAT,%dx
	inb %dx,%al
	testb $AHA_S_INIT,%al		/* can we get the old state? */
	jnz 1f

	/* execute a host adapter setup command */
	pushl $_setup_data
	pushl $1
	pushl $_data_in_length
	pushl $AHA_SETUP_DATA
	call _ahafastcmd
	addl $16,%esp

	/* if we have exactly one mbox, we know which ccb was run */
	movl _setup_data+4,%eax
	cmpb $1,%al
	jne 1f
	movb $0,%al
	BSWAP()

	/* extract ccb pointer */
	movl (%eax),%eax
	cmpb $MBOX_I_COMPLETED,%al
	jne 1f
	movb $0,%al
	BSWAP()

	/* store control word (containing target) into our CCB */
	movb CCB_CTRL(%eax),%al
	movb %al,CCB_CTRL(%ebx)
	shlb $5,%al
	movb %al,CCB_CDB+CDB_LUN(%ebx)		/* save the LUN */

1:
#endif

	/*
	 * Initialize the host adapter.
	 * We check first for an mailbox lock,
	 * then set up the mailbox.
	 */
	pushl $_mbox_lock
	pushl $0
	pushl $0
	pushl $AHA_GET_LOCK
	call _ahafastcmd
	addl $16,%esp
	cmpb $0,_mbox_lock+1
	je 1f

	movb $0,_mbox_lock
	pushl $0
	pushl $2
	pushl $_mbox_lock
	pushl $AHA_SET_LOCK
	call _ahafastcmd
	addl $16,%esp

1:
	pushl $0
	pushl $4
	leal _mbox,%eax
	BSWAP()				/* render the address big-endian */
	movb $1,%al			/* mailbox count */
	movl %eax,_mbox_init_data
	pushl $_mbox_init_data
	pushl $AHA_MBOX_INIT
	call _ahafastcmd
	addl $16,%esp

	/*
	 * Initialize our CCB and read the block 0 DOS label.
	 */
	movb $10,CCB_CDBLEN(%ebx)
	movb $1,CCB_RQSLEN(%ebx)
	movb $(BSIZE >> 8),CCB_DATALEN+1(%ebx)
	AHA_CONST(BOOTADDR,CCB_DATA(%ebx))
	movb $CMD_READ10,CCB_CDB+CDB_CMD(%ebx)
	movb $1,CCB_CDB+CDB_XFERLEN+1(%ebx)

	call _start_scsi_cmd

	/*
	 * Look for an active BSD/386 partition in the DOS partition label.
	 * If no active BSDI partition is found, then pick the first
	 * BSDI partition.
	 */
	xorl %eax,%eax
	leal BOOTADDR,%esi
	cmpw $MB_SIGNATURE,510(%esi)
	jne 5f
	addl $MB_PARTOFF,%esi
	movl $4,%ecx

2:
	cmpb $MBS_BSDI,4(%esi)
	jne 3f
	cmpb $MBA_ACTIVE,(%esi)
	je 4f
	cmpl $0,%eax			/* first BSDI partition? */
	jne 3f
	movl 8(%esi),%eax		/* mbpart.start */
	cmpb $MBA_NOTACTIVE,(%esi)
	jne 5f
3:
	addl $16,%esi
	loop 2b
	jmp 5f

4:
	movl 8(%esi),%eax		/* mbpart.start */

	/*
	 * Read the 15-sector label+bootstrap (finally).
	 */
5:
	cmpl $15,%eax			/* start at sector 15? */
	jne 6f
	xorl %eax,%eax			/* for us, 15 means 0 (bootany hack) */
6:
	movl %eax,%esi			/* save it for the bootstrap */
	incl %eax
	BSWAP()

	movb $((BOOTBLOCKS*BSIZE) >> 8),CCB_DATALEN+1(%ebx)
	movl %eax,CCB_CDB+CDB_LBA(%ebx)
	movb $BOOTBLOCKS,CCB_CDB+CDB_XFERLEN+1(%ebx)

	call _start_scsi_cmd

	/*
	 * Call the bootstrap.
	 */
	pushl %esi
	movzbl CCB_CTRL(%ebx),%eax
	shrl $5,%eax
	shll $B_CONTROLLERSHIFT,%eax
	orl $MAKEBOOTDEV(SD_MAJORDEV, AHA_DRIVER, 0, 0, 0),%eax
	pushl %eax
	pushl $RB_AUTOBOOT		/* howto */
	pushl $BOOTSTART		/* jump to an absolute address */
	ret

/*
 * ahafastcmd(int cmd, u_char *out, int outlen, u_char *in);
 * We clobber EAX, ECX and EDX; we save ESI amd EDI.
 */
	.globl _ahafastcmd
_ahafastcmd:
	pushl %esi
	pushl %edi

	movw $AHA_STAT,%dx
0:
	inb %dx,%al
	testb $AHA_S_IDLE,%al
	jz 0b

	movb $AHA_C_IRST,%al
	outb %al,%dx

	movl 12(%esp),%eax
	movl 16(%esp),%esi
	movl 20(%esp),%ecx
	incl %ecx			/* add 1 for command byte */
	movl 24(%esp),%edi

	/* load command and parameters */
1:
	movw $AHA_DATA,%dx
	outb %al,%dx

	movw $AHA_STAT,%dx
2:
	inb %dx,%al
	testb $AHA_S_CDF,%al
	jnz 2b

	lodsb (%esi)			/* this reads one extra byte... */
	loop 1b

	/* collect results */
3:
	movw $AHA_INTR,%dx
	inb %dx,%al
	testb $AHA_I_HACC,%al
	jnz 4f

	movw $AHA_STAT,%dx
	inb %dx,%al
	testb $AHA_S_DF,%al
	jz 3b

	movw $AHA_DATA,%dx
	inb %dx,%al
	stosb (%edi)
	jmp 3b

4:
	popl %edi
	popl %esi
	ret

/*
 * start_scsi_cmd(void);
 * We assume EBX contains a pointer to our (single) CCB.
 * We clobber EAX and EDX.
 */
	.globl _start_scsi_cmd
_start_scsi_cmd:
	/* clear the previous interrupt */
	movw $AHA_STAT,%dx
	movb $AHA_C_IRST,%al
	outb %al,%dx
0:
	inb %dx,%al
	testb $AHA_S_CDF,%al
	jne 0b

	/* stuff the mailbox with the (byte-swapped) CCB address */
	movl %ebx,%eax
	BSWAP()
	movb $MBOX_O_START,%al
	movl %eax,_mbox

	/* poke the adapter */
	movb $AHA_START_SCSI_CMD,%al
	movw $AHA_DATA,%dx
	outb %al,%dx
	movw $AHA_INTR,%dx
1:
	inb %dx,%al
	testb $AHA_I_ANY,%al
	jz 1b

	/* acknowledge the completed command */
	movb $MBOX_I_FREE,_mbox+4

	ret

#if 0
	.globl _data_in_length
_data_in_length:
	.byte 20
#endif

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
ebootblkcode:

	.org 510
	.word MB_SIGNATURE

	/* bss */
	.comm _mbox,8
	.comm _ccb,28
	.comm _mbox_init_data,4
#if 0
	.comm _setup_data,20
#endif
	.comm _mbox_lock,4
