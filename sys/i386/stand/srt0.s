/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: srt0.s,v 1.7 1994/02/04 04:09:10 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)srt0.c	5.3 (Berkeley) 4/28/91
 */

/*
 * Startup code for standalone system
 * Non-relocating version -- for programs which are loaded by boot
 * Relocating version for boot
 * Small relocating version for "micro" boot
 */

#include "i386/include/limits.h"
#include "sys/reboot.h"

#ifdef DEBUG
#define CRT     (0xb8000+(24*80*2))	/* start of last line of display */
#define	PUT(c,col) \
	movw	$(0x1f<<8)+(c),CRT+2*(col)
#else
#define	PUT(c,col)	/* void */
#endif /* DEBUG */

	.globl	_end
	.globl	_edata
	.globl	_main
	.globl	__rtt
	.globl	_exit
	.globl	_bootdev
	.globl	_cyloffset

	.globl	__zero			# artificial value for entry point
	.set	__zero,0

#ifdef SMALL
	/* where the disklabel goes if we have one */
	.globl	_disklabel
_disklabel:
	.space 512
#endif

entry:	.globl	entry
	.globl	start

	PUT(0x47,6)

#if	defined(REL) && !defined(SMALL)

#define	KBDATA	0x60		/* kbd data port */
#define	KBSTAT	0x64		/* kbd status/cmd port */
#define	KBS_IFULL	0x02	/* kbd input buffer full, not ready */

#define	WAIT_IRDY \
1:	inb	$KBSTAT,%al; \
	andb	$KBS_IFULL,%al; \
	jnz	1b

	/* enable gate a20! yecchh!! */
	WAIT_IRDY
	movb	$0xd1,%al
	outb	%al,$KBSTAT
	WAIT_IRDY
	movb	$0xdf,%al
	outb	%al,$KBDATA

	/* send a NOP to 8042 to wait for A20 command to complete */
	WAIT_IRDY
	movb    $0xff,%al
	NOP
	outb    %al,$KBSTAT
	NOP
	WAIT_IRDY


	/* relocate program and enter at symbol "start" */

	#movl	$entry-BRELOC,%esi	# from beginning of ram
	movl	$__zero,%esi
	#movl	$entry,%edi		# to relocated area
	movl	$BRELOC,%edi		# to relocated area
	# movl	$_edata-BRELOC,%ecx	# this much
	movl	$64*1024,%ecx
	cld
	rep
	movsb
	# relocate program counter to relocation base
	pushl	$start
	ret
#endif

start:

	/* setup stack pointer */

#ifdef REL
	/*
	 * Copy our arguments to a safe place.
	 * Note that boot blocks don't use CALL.
	 */
#ifndef SMALL
	popl	%ecx		/* pop EIP word left by CALL */
#endif
	popl	%edx
	popl	%ebx
	popl	%eax
#ifdef SMALL
	movl	$SMRELOC-4,%esp
#else
	/* copy boot parameters if present (bios disk parameters should be) */
	movl	%esp,%esi		# params should be at top of old stack
	movl	$BRELOC-4,%esp		# new stack
	cmpl	$BOOT_MAGIC,B_MAGIC(%esi)
	jne	1f
	movl	B_LEN(%esi),%ecx	# length of parameters
	subl	%ecx,%esp		# make space on stack
	movl	%esp,%edi
	cld
	rep
	movsb
1:
#endif
	pushl	%eax
	pushl	%ebx
	pushl	%edx

#else /* REL */
	/* save old stack state */
	movl	%esp,savearea
	movl	%ebp,savearea+4
	movl	$SMRELOC-0x2400,%esp	/* Why move sp? */
#endif /* REL */

	PUT(0x48,7)
#ifdef SMALL
	/* retrieve BIOS disk parameters before we trash too much */
	.globl	_getbiosgeom
	call	_getbiosgeom
#endif
	/* clear memory as needed */

	movl	%esp,%esi
#ifdef	REL

	/*
	 * Clear Bss and up to 64K heap
	 */
	movl	$64*1024,%ebx
	movl	$_end,%eax	# should be movl $_end-_edata but ...
	subl	$_edata,%eax
	addl	%ebx,%eax
	pushl	%eax
	pushl	$_edata
	call	_bzero

	/*
	 * Clear 64K of stack
	 */
	movl	%esi,%eax
	subl	%ebx,%eax
	subl	$5*4,%ebx
	pushl	%ebx
	pushl	%eax
	call	_bzero
#else
	movl	$_edata,%edx
	movl	%esp,%eax
	subl	%edx,%eax
	pushl	%edx
	pushl	%esp
	call	_bzero
#endif
	movl	%esi,%esp
	PUT(0x49,8)

	pushl	$0
	popf

#ifndef	SMALL
	call	_kbdreset	/* resets keyboard */
#endif

	call	_main
	jmp	1f

	.data
_bootdev:	.long	0
_cyloffset:	.long	0
savearea:	.long	0,0	# sp & bp to return to
	.text
#ifndef	SMALL
	.globl _getchar
#endif
	.globl _wait

__rtt:
#ifndef	SMALL
	call	_getchar
#else
_exit:
	pushl	$1000000
	call	_wait
	popl	%eax
#endif
	movl	$-7,%eax
	jmp	1f
#ifndef	SMALL
_exit:
	call	_getchar
#endif
	movl	4(%esp),%eax
1:
#ifdef	REL
#ifndef SMALL
	call	_reset_cpu
#endif
	movw	$0x1234,%ax
	movw	%ax,0x472	# warm boot
	movl	$0,%esp		# segment violation
	ret
#else
	movl	savearea,%esp
	movl	savearea+4,%ebp
	ret
#endif

	.globl	_inb
_inb:	movl	4(%esp),%edx
	subl	%eax,%eax	# clr eax
	inb	%dx,%al
	ret

	.globl	_outb
_outb:	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outb	%al,%dx
	ret

#ifndef SMALL
	.globl	_rtcin
_rtcin:	movl	4(%esp),%eax
	outb	%al,$0x70
	subl	%eax,%eax	# clr eax
	inb	$0x71,%al
	ret
#endif

	.globl ___udivsi3
___udivsi3:
	movl 4(%esp),%eax
	xorl %edx,%edx
	divl 8(%esp)
	ret

	.globl ___divsi3
___divsi3:
	movl 4(%esp),%eax
	xorl %edx,%edx
	cltd
	idivl 8(%esp)
	ret

	#
	# bzero (base,cnt)
	#

	.globl _bzero
_bzero:
	pushl	%edi
	movl	8(%esp),%edi
	movl	12(%esp),%ecx
	movb	$0x00,%al
	cld
	rep
	stosb
	popl	%edi
	ret

	#
	# bcopy (src,dst,cnt)
	# NOTE: does not (yet) handle overlapped copies
	#

	.globl	_bcopy
_bcopy:
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cld
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	# insw(port,addr,cnt)
	.globl	_insw
_insw:
	pushl	%edi
	movw	8(%esp),%dx
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	cld
	.byte 0x66,0xf2,0x6d	# rep insw
	movl	%edi,%eax
	popl	%edi
	ret

	# outsw(port,addr,cnt)
	.globl	_outsw
_outsw:
	pushl	%esi
	movw	8(%esp),%dx
	movl	12(%esp),%esi
	movl	16(%esp),%ecx
	cld
	.byte 0x66,0xf2,0x6f	# rep outsw
	movl	%esi,%eax
	popl	%esi
	ret

