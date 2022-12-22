/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: locore.s,v 1.38 1994/02/04 02:16:57 karels Exp $
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
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */


/*
 * locore.s:	4BSD machine support for the Intel 386
 */

#include "assym.s"
#include "machine/psl.h"
#include "machine/pte.h"
#include "machine/vmlayout.h"
#include "specialreg.h"
#include "syscall.h"
#include "machine/limits.h"
#include "reboot.h"

#include "errno.h"

#include "machine/trap.h"
#include "machine/segments.h"
#include "../isa/isa.h"
#include "../isa/timerreg.h"
#include "../isa/rtc.h"

#if /*defined(GENERIC) &&*/ !defined(DEBUG)
#define DEBUG	/* force run-time startup debugging for GENERIC */
#endif

#ifdef DEBUG
#define CRT_MONO	(0xb0000 + (24*80*2))	/* start of last line */
#define CRT_COLOR	(0xb8000 + (24*80*2))
/*
 * Debugging stuff:
 * Print one character at specified column on last line of display,
 * then delay briefly.  PUT works while in physical mode;
 * VPUT works in virtual mode after atdevbase is set, and uses %ecx.
 * Do it in both the color and mono memory so it will be visible on
 * any machine.
 */
#define	PUT(c,col) \
	cmpl	$0,PHYS(_bootdebug); \
	je	7f; \
	movw	$(0x1f<<8)+(c),CRT_MONO+2*(col); \
	movw	$(0x1f<<8)+(c),CRT_COLOR+2*(col); \
	DEL(100000); \
7:

#define	VPUT(c,col) \
	cmpl	$0,_bootdebug; \
	je	7f; \
	movl	_atdevbase,%ecx;\
	addl	$CRT_MONO-IOM_BEGIN+2*(col),%ecx; \
	movw	$(0x1f<<8)+(c),(%ecx); \
	movl	_atdevbase,%ecx;\
	addl	$CRT_COLOR-IOM_BEGIN+2*(col),%ecx; \
	movw	$(0x1f<<8)+(c),(%ecx); \
	VDEL(100000); \
7:

#define	DEL(n) \
	cmpl	$0,PHYS(_bootdebug); \
	je	8f; \
	pushl %ecx; \
	movl	$(n),%ecx; \
9:	outb	%al,$0x80; \
	loop	9b; \
	popl %ecx; \
8:

#define	VDEL(n) \
	cmpl	$0,_bootdebug; \
	je	8f; \
	pushl %ecx; \
	movl	$(n),%ecx; \
9:	outb	%al,$0x80; \
	loop	9b; \
	popl %ecx; \
8:

#else /* DEBUG */
#define	PUT(c,col)	/* nothing */
#define	VPUT(c,col)	/* nothing */
#define	DEL(n)		/* nothing */
#define	VDEL(n)		/* nothing */
#endif /* DEBUG */

	.globl	SYSTEM			# for gdb -k
	.set	SYSTEM,KERNBASE		# virtual address of system start

#define	PHYS(x)	((x) - KERNBASE)	/* physical location of text/data */

/*
 * Convert a virtual address adrs into a physical address in reg;
 * the address must be in the statically-loaded part of the kernel,
 * which is loaded around the I/O hole.
 */
#define CONVADRS(adrs, reg) \
	lea	adrs - KERNBASE, reg; \
	cmpl    $IOM_BEGIN, reg;        \
	jb	8f; \
	addl	$IOM_SIZE, reg; \
8:

/* IBM "compatible" nop - sensitive macro on "fast" 386 machines */
#define	NOP	;

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 */
	.globl	_PTmap, _PTD, _PTDpde
	.set	_PTmap, _PTMAP
	.set	_PTD, _PTMAP + (PTDPTDI * NBPG)
	.set	_PTDpde, _PTD + (SZ_PDE * PTDPTDI)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
	.globl	_APTmap, _APTD, _APTDpde
	.set	_APTmap, _APTMAP
	.set	_APTD, _APTMAP + (APTDPTDI * NBPG)
	.set	_APTDpde, _PTD + (SZ_PDE * APTDPTDI)

/*
 * Access to each processes kernel stack is via a region of
 * per-process address space (at the beginning), immediatly above
 * the user process stack.
 */
	.set	_kstack, _KSTACK
	.globl	_kstack
	.set	KSTKPTEOFF, (NBPG / SZ_PDE) - UPAGES

#ifdef GPROF
#define	ENTRY(name) \
	.globl _/**/name; _/**/name: pushl %ebp; movl %esp,%ebp; \
	.data; .align 2; LP/**/name: .long 0; \
	.text; movl $LP/**/name,%eax; call mcount; leave; 1:
#define	ALTENTRY(name, rname) \
	ENTRY(name); jmp 1f
#else
#define	ENTRY(name) \
	.globl _/**/name; _/**/name:
#define	ALTENTRY(name, rname) \
	.globl _/**/name; _/**/name:
#endif

/*
 * Initialization
 */
	.data
	.globl	_cpu,_cold,_boothowto,_bootdev,_cyloffset,_atdevbase,_atdevphys
_cpu:		.long	0	# are we 386, 386sx, or 486
_cold:		.long	1	# cold till we are not
_atdevbase:	.long	0	# location of start of iomem in virtual
_atdevphys:	.long	0	# location of device mapping ptes (phys)

	.globl	_IdlePTD, _KPTphys
_IdlePTD:	.long	0
_KPTphys:	.long	0

	.globl	_maxmem
/*
 * MAXMEM is the maximum amount of memory to configure, in pages.
 */
#ifndef MAXMEM
#define	MAXMEM	((256 * 1024 * 1024) / NBPG)	/* 256 MB / NBPG */
#endif
_maxmem:	.long	MAXMEM

#ifdef CHECKMEMWRAP
/*
 * We check for memory wrap-around at each power of two between
 * 1 MB and maxmem.
 */
	.globl	_memwrap
_memwrap:	.long	0
#endif

	.align	2
	.space 512		# tmp stack
tmpstk:

	.text
	.globl	start
 #start:
	.set start,0
	movw	$0x1234,%ax
	movw	%ax,0x472	# warm boot
	jmp	1f
	.space	0x500		# skip over warm boot shit

	/*
	 * The following are in text space for ease of access
	 * before we turn on virtual memory, so that we can use PHYS().
	 */
	.align	2
_bootdebug:	.long	0	# debugging flag for startup, from boothowto
_bootparamp:	.long	0	# location of boot parameters on boot stack
	.space 64		# safety zone
limstk:				# bottom of startstk to fill
	.space 448		# startup stack
startstk:

	.globl	_cpu_id
_cpu_id: .space	4		# eax value from cpuid instruction

	/*
	 * Parameters are passed on bootstrap's stack
	 * as (howto, bootdev, cyloffset, bootparams).
	 * Note: (%esp) is return address of boot.
	 */
1:
	movl	4(%esp),%eax
	CONVADRS(_boothowto, %ebx)
	movl	%eax,(%ebx)
	andl	$RB_KDB,%eax		/* extract debugging flag */
	movl	%eax,PHYS(_bootdebug)	/* for easy access as we go */
	PUT(0x41,0)	/* 'A' */
	movl	8(%esp),%eax
	CONVADRS(_bootdev, %ebx)
	movl	%eax,(%ebx)
	movl	12(%esp),%eax
	CONVADRS(_cyloffset, %ebx)
	movl	%eax,(%ebx)
	leal	16(%esp),%eax
	cmpl	$BOOT_MAGIC,B_MAGIC(%eax)
	jne	1f
	movl	%eax,PHYS(_bootparamp)
1:
	movl	$PHYS(startstk),%esp	# bootstrap stack end location

	/* establish standard flags */
	pushl	$PSL_MBO
	popf

	/* determine cpu type */
	movl	%esp,%edx
	andl 	$~3,%esp
	pushfl				# stuff EFLAGS on the stack
	popl	%eax			# so we can grab them into AX
	movl	%eax,%ecx		# save a copy of orig EFLAGS in CX
	orl	$PSL_AC,%eax		# set AC bit (on 486+)
	xorl	$PSL_ID,%eax		# toggle ID bit (Pentium+)
	pushl	%eax			# stuff new EFLAGS on the stack
	popfl				# so we can hand them to the processor
	pushfl				# have it push them on the stack
	popl	%eax			# and grab the new EFLAGS into AX
	pushl	%ecx			# restore original EFLAGS
	popfl
 	movl	%edx,%esp
	CONVADRS(_cpu, %ebx)
 	movl	$CPU_386,(%ebx)		# by default, we're on a 386
	testl	$PSL_AC,%eax		# see if AC bit stayed (only on 486+)
	jz	1f			# is this really a 386?
	movl	$CPU_486,(%ebx)		# no, must be a 486+
	/*
	 * Check the PSL_ID bit, which can be toggled on Pentium
	 * and newer 486 chips; if so, the cpuid instruction is supported.
	 */
	andl	$PSL_ID,%eax		# see if ID bit stayed (only on newer)
	andl	$PSL_ID,%ecx
	cmpl	%eax,%ecx		# see if ID bit toggled
	je	1f			# if no cpuid instruction, done

	cmpl	$0,PHYS(_bootdebug)	# if bootdebug is set, do not try cpuid
	jne	1f

	/* try cpuid instruction, see what we get */
#define	cpuid	.byte 0x0f, 0xa2
	xorl	%eax,%eax		# input value to cpuid
	cpuid				# find highest input value supported
	cmpl	$1,%eax			# must be at least 1 (we want 1)
	jb	1f
	movl	$1,%eax			# input to cpuid: find ID
	cpuid
	movl	%eax,PHYS(_cpu_id)	# family/model/stepping
#if 0
	movl	%ebx,PHYS(_cpu_id+4)	# on Intel, "Genu" (G is low nibble)
	movl	%edx,PHYS(_cpu_id+8)	# on Intel, "ineI"
	movl	%ecx,PHYS(_cpu_id+12)	# on Intel, "ntel"
#endif
	andl	$0xf00,%eax		# extract family
	cmpl	$0x500,%eax		# family 5 is Pentium, 4 is 486
	jl	1f

	CONVADRS(_cpu, %ebx)
	movl	$CPU_586,(%ebx)		# must be a Pentium or later
1:
	PUT(0x42,1)	/* 'B' */

/*
 * Flush cache; on 486, use write-back-and-flush instruction,
 * or large copy to flush cache; assume 512K max cache for now.
 * We currently do both on 486, as external caches might not
 * implement the flush.
 */
#define	wbinvd	.byte 0x0F, 0x09
#define	FLUSHCACHE \
	CONVADRS(_cpu, %edi); \
	cmpl	$CPU_486,(%edi); \
	jne	9f; \
	wbinvd; \
9: \
	cld; \
	pushl	%ecx; \
	pushl	%esi; \
	movl	$0,%esi; \
	movl	$0,%edi; \
	movl	$512*1024/4,%ecx; \
	rep; movsl; \
	popl	%esi; \
	popl	%ecx

	/*
	 * Determine amount of memory present.  This is done in two
	 * parts: one part before the 640K "hole", carelessly done because
	 * we live there while running this code, and one part more carefully
	 * done above the "hole".  The second part is careful about
	 * address wrap-around and cache invalidation.
	 */

#ifdef CHECKMEMWRAP
/*
 * The following fails on some machines, which fault/reset when we touch
 * locations beyone the end of memory.
 */
	/* look for wrap-around address; assume power of 2 at/above IOM_END */
	movl	$IOM_END,%edx		# start after IO memory
	CONVADRS(_maxmem, %ebx)
	movl	(%ebx),%ecx		# limit wrap-around check to maxmem
	movl	0,%ebx			# save contents of location 0
	movl	$0,0			# 0 at location 0
	shll	$PGSHIFT,%ecx		# convert to bytes
1:
	movl	(%edx),%eax		# save old value
	movl	%edx,(%edx)		# mark each address with its address
	cmpl	$0,0			# see if location 0 changed
	jne	2f
	movl	%eax,(%edx)		# restore old value
	shll	$1,%edx			# next higher power of 2
	cmpl	%ecx,%edx		# through maxmem probe limit
	jae	3f
	jmp	1b
2:
	movl	%eax,(%edx)		# restore old value
3:
	movl	%ebx,0			# restore location 0
	CONVADRS(_memwrap, %ebx)
	movl	%edx,(%ebx)		# and save in memwrap
#endif /* CHECKMEMWRAP */

	PUT(0x43,2)	/* 'C' */
	/* count up memory before I/O "hole" memory */
	xorl	%edx,%edx		# start with base memory at 0x0
	movl	$IOM_BEGIN/NBPG,%ecx	# look every 4K up to 640K
1:	movl	(%edx),%ebx		# save contents of location to check
	movl	$0xa55a5aa5,(%edx)	# write test pattern

	inb	$0x84,%al		# flush write buffer
	/* flush stupid cache here! (with bcopy (0,0,512*1024) ) */

	cmpl	$0xa55a5aa5,(%edx)	# does not check for rollover
	je	2f
	movl	%edx,%ebx		# %ebx points to first bad page
	jmp	L_memdone
2:
	movl	%ebx,(%edx)		# restore memory
	addl	$NBPG,%edx
	loop	1b

	/*
	 * Now count up memory after I/O "hole" memory.
	 * First, determine how far to look, based in initialized value
	 * of maxmem and CMOS extended memory value.
	 * We would prefer to ignore the CMOS, as the CMOS value is
	 * limited to 64 MB, but some machines fault/reset if we touch
	 * locations beyond the end of memory.
	 */

	PUT(0x44,3)	/* 'D' */
#ifdef CHECKMEMWRAP
	CONVADRS(_memwrap, %ebx)
	movl	(%ebx),%ecx		# probe up to min(maxmem, btop(memwrap))
	shrl	$PGSHIFT,%ecx		# convert to pages
#else /* CHECKMEMWRAP */

	/*
	 * Check amount of extended memory known by CMOS, returned in KB.
	 * Would rather use calls to PHYS(_rtcin), but this fails on
	 * some machines (???).
	 */
	movb	$RTC_EXTHI,%al
	outb	%al,$0x70
	xorl	%ecx,%ecx
	inb	$0x71,%al
	movb	%al,%ch
	movb	$RTC_EXTLO,%al
	outb	%al,$0x70
	jmp	9f; 9:			/* delay */
	inb	$0x71,%al
	movb	%al,%cl
#define	LOG2_1024	10		/* log2(1024) */
	shrl	$(PGSHIFT-LOG2_1024),%ecx	# convert KB to pages
	addl	$(IOM_END/NBPG),%ecx
#endif /* CHECKMEMWRAP */
	CONVADRS(_maxmem, %ebx)
	cmpl	(%ebx),%ecx
	jb	1f
	movl	(%ebx),%ecx
1:
	subl	$(IOM_END/NBPG),%ecx	# number of pages to check
	movl	$IOM_END,%edx		# starting at this location
	movl	$0,0			# 0 at location 0, for wraparound test

	xorl	%ebx,%ebx		# set %ebx to end of memory when found
	cmpl	$0,%ecx			# if no pages, can't test
	jne	1f
	movl	%edx,%ebx		# the first is the last
1:

	PUT(0x45,4)	/* 'E' */
	/*
	 * loop through memory a page at a time, marking first word of page
	 * with test pattern after saving previous value on stack.
	 * When stack gets full, flush cache and then check the test values.
	 * We do the test in batches as we have no fast way to flush
	 * an external cache on a 386 (although we do on the 486).
	 * Note: the only reason to save and restore old values
	 * is to preserve the contents of the msgbuf, which the BIOS
	 * generally clears for us free of charge.
	 */

L_outer:
	cmpl	$0,%ebx			# have we found the end?
	jne	L_memdone

	/*
	 * Save the address corresponding to the first value pushed
	 * onto the stack, so that L_checkpages can tell when it has
	 * cleared the stack.  We always check at least one page
	 * before reaching L_checkpages, even if we overrun limstk
	 * by one word.
	 */
	movl	%edx,%esi		# first address for this section

L_inner:
	pushl	(%edx)			# save contents of location to check
	movl	$0xa55a5aa5,(%edx)	# write test pattern
	inb	$0x84,%al		# flush write buffer
	cmpl	$0xa55a5aa5,(%edx)	# quick check for systems w/o cache
	jne	L_memfail		# at or beyond the end of memory
	cmpl	$0,0			# see if location 0 changed
	jne	L_memwrap		# if so, we wrapped around

	cmpl	$PHYS(limstk),%esp	# is stack filled with pages?
	jbe	L_checkpages		# yes, check them

	addl	$NBPG,%edx		# continue to next page
	loop	L_inner

	/* fall through when last page has been written */
	movl	%edx,%ebx		# the current address is beyond mem
	subl	$NBPG,%edx		# edx was bumped without stack push
	PUT(0x3e,40)	/* '>' */
	jmp 	L_checkpages

L_memwrap:
	PUT(0x57,40)	/* 'W' */
	jmp	Lxxx

L_memfail:
	PUT(0x21,40)	/* '!' */
Lxxx:
	movl	%edx,%ebx		# the current address is beyond mem
	/* fall through and clear stack */

	/*
	 * Iterate through a set of pages, testing whether they
	 * are present by checking them for the test pattern.
	 * We flush the cache (possibly the hard way) before checking.
	 * %edx points at the highest page to test.  The stack
	 * contains the old values to restore for the set of pages,
	 * highest page at the top.  The value of %esi is the address
	 * of the first value pushed, so that we know when to quit.
	 * If we find a page that doesn't exist, put its address in %ebx;
	 * as we iterate backwards, the address of the first nonexistent
	 * page will end up in %ebx.
	 */
L_checkpages:
	inb	$0x84,%al		# flush write buffer
	FLUSHCACHE
	movl	%edx,%eax		# save current location
1:
	cmpl	$0xa55a5aa5,(%edx)	# check for test pattern
	je	2f
	movl	%edx,%ebx		# at or beyond the end of memory
2:
	popl	(%edx)			# restore previous value
	cmpl	%edx,%esi		# check if stack has been cleared
	je	3f			# if so, return
	subl	$NBPG,%edx		# pop goes to previous page
	jmp	1b			# continue
3:
	movl	%eax,%edx		# restore current location to test
	jmp	L_outer			# "return" to outer loop

L_memdone:
	shrl	$PGSHIFT,%ebx		# %ebx is now first memory size
	CONVADRS(_maxmem, %eax)
	movl	%ebx,(%eax)

/*
 * Virtual address space of kernel:
 * text | data | bss | page dir | proc0 kernel stack | usr stk map | Sysmap
 *	size in pages:	1               UPAGES	       1             1
 *
 * Additional pages of "Sysmap" are allocated later; we do one here,
 * enough to map the kernel proper and get us going.
 */

	PUT(0x46,5)	/* 'F' */
/* find end of kernel image; save aligned space for bootparams at _end */
	movl	$PHYS(_end),%ecx
	movl	PHYS(_bootparamp),%eax
	cmpl	$0,%eax
	je	1f
	addl	$3,%ecx			# force alignment
	andl	$~3,%ecx
	addl	B_LEN(%eax),%ecx
1:
	addl	$NBPG-1,%ecx
	andl	$~(NBPG-1),%ecx

	cmpl	$(IOM_BEGIN - (1+UPAGES+1+1)*NBPG), %ecx
	jbe     2f
	cmpl    $IOM_BEGIN, %ecx
	ja      1f
	movl    $IOM_BEGIN, %ecx
1:	addl    $IOM_SIZE, %ecx
2:	movl	%ecx,%esi
	addl	$(1+UPAGES+1+1)*NBPG,%ecx

/* clear bss and memory for bootstrap pagetables. */
	CONVADRS(_edata, %edi)		# Clearing from _edata
	xorl	%eax,%eax		# pattern
	cld
	cmpl	$IOM_END, %edi		# Are both above IO hole?
	jae	1f
	cmpl	$IOM_BEGIN, %ecx	# Are both below?
	jbe	1f
	PUT(0x48, 20)	/* 'H' */
	pushl	%ecx			# Save top
	movl	$IOM_BEGIN, %ecx	# Clear to hole first
	subl	%edi, %ecx
	rep
	stosb
	movl	$IOM_END, %edi		# Now clear above hole
	popl	%ecx			# Restore top
1:
	subl    %edi,%ecx
	rep
	stosb

/* save boot params at _end if present */
	pushl	%esi
	movl	PHYS(_bootparamp),%esi
	cmpl	$0,%esi
	je	1f
	movl	$PHYS(_end),%edi
	addl	$3,%edi			# force alignment
	andl	$~3,%edi
	movl	%edi,%eax
	orl	$KERNBASE,%eax		# convert to virtual for later
	movl	%eax,PHYS(_bootparamp)	# new location of bootparams
	movl	B_LEN(%esi),%ecx
	cmpl	$IOM_BEGIN, %edi	# ...Just CONVADRS
	jb	2f
	addl	$IOM_SIZE, %edi
2:	movsb
	cmpl	$IOM_BEGIN, %edi
	jne	3f
	PUT(0x50, 21)	/* 'P' */
	movl	$IOM_END, %edi
3:	loop	2b
1:	popl	%esi

	CONVADRS(_IdlePTD, %ebx)
	movl	%esi,(%ebx)	 /* physical address of Idle Address space */
	PUT(0x47,6)	/* 'G' */

#define	fillkpt(prot)		\
1:	movl	%eax,(%ebx)	; \
	addl	$NBPG,%eax	; /* increment physical address */ \
	cmpl	$(IOM_BEGIN|(prot)),%eax; /* run into the hole? */ \
	jne	2f		; \
	PUT(0x54, 22)		; /* 'T' */ \
	movl	$(IOM_END|(prot)),%eax; /* jump over the hole */ \
2:	addl	$4,%ebx		; /* next pte */ \
	loop	1b		;

#define	ofillkpt		\
1:	movl	%eax,(%ebx)	; \
	addl	$NBPG,%eax	; /* increment physical address */ \
	addl	$4,%ebx		; /* next pte */ \
	loop	1b		;

/*
 * Map Kernel
 *
 * First step - build page tables
 * Map the kernel text read-only (useful on the 486)
 */

#ifdef KGDB		/* XXX make life simpler for breakpoints */
	movl	%esi,%ecx		# this much memory,
	shrl	$PGSHIFT,%ecx		# for this many pte s
	addl	$1+UPAGES+1,%ecx	# including our early context
	movl	$PG_KW|PG_V,%eax	#  having these bits set, page 0
	lea	((1+UPAGES+1)*NBPG)(%esi),%ebx	# physical addr of KPT in proc 0,
	CONVADRS(_KPTphys, %edx)
	movl	%ebx,(%edx)		#    in the kernel page table,
	fillkpt(PG_KW|PG_V)
#else
	lea	PHYS(_etext),%ecx	# text size
	shrl	$PGSHIFT,%ecx
	movl	$PG_KR|PG_V,%eax	# page 0, read-only
	leal	((1+UPAGES+1)*NBPG)(%esi),%ebx
	CONVADRS(_KPTphys, %edx)
	movl	%ebx,(%edx)
	fillkpt(PG_KR|PG_V)

	andl	$PG_FRAME,%eax		/* data and bss are r/w */
	movl	%esi,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	addl	$1+UPAGES+1,%ecx
	orl	$PG_KW|PG_V,%eax
	fillkpt(PG_KW|PG_V)
#endif
/* map I/O memory map */

	movl	$(IOM_SIZE/NBPG),%ecx	# for this many pte s,
	movl	$(IOM_BEGIN|PG_UW|PG_V),%eax
	CONVADRS(_atdevphys, %edx)
	movl	%ebx,(%edx)		#   remember phys addr of ptes
	ofillkpt

/* map proc 0's kernel stack into user page table page */

	lea	(1*NBPG)(%esi),%eax	# physical address in proc 0
	lea	(KERNBASE)(%eax),%edx
	cmpl	$IOM_BEGIN, %esi
	jb	1f
	subl	$IOM_SIZE, %edx
1:	CONVADRS(_proc0paddr, %ecx)
	movl	%edx,(%ecx)	  # remember VA for 0th process init
	movl	$UPAGES,%ecx		# for this many pte s,
	orl	$PG_V|PG_KW,%eax	#  having these bits set,
	lea	((1+UPAGES)*NBPG)(%esi),%ebx # physical addr of stk pt in proc 0
	addl	$(KSTKPTEOFF * SZ_PDE),%ebx
	ofillkpt

/*
 * Construct a page table directory (of page directory elements - pde's).
 * We fill in only one kernel entry here, and do the rest in pmap_bootstrap.
 */
	/* install a pde for temporary double map of bottom of VA */
	lea	((1+UPAGES+1)*NBPG)(%esi),%eax	# physical addr of kernel pt
	orl	$PG_KW|PG_V,%eax	# pde entry is valid
	movl	%eax,(%esi)		# which is where temp maps!

	/* first kernel pde */
	movl	%eax,(KPTDI_FIRST*4)(%esi)	# offset of first pde for kernel

	/* install a pde recursively mapping page directory as a page table! */
	movl	%esi,%eax		# phys address of ptd in proc 0
	orl	$PG_KW|PG_V,%eax	# pde entry is valid
	movl	%eax, PTDPTDI*4(%esi)	# which is where PTmap maps!

	/* install a pde to map kernel stack for proc 0 */
	lea	((1+UPAGES)*NBPG)(%esi),%eax	# physical addr of pt in proc 0
	orl	$PG_KW|PG_V,%eax	# pde entry is valid
	movl	%eax,KSTKPTDI*4(%esi)	# which is where kernel stack maps!

	PUT(0x48,7)	/* 'H' */
	/* load base of page directory, and enable mapping */
	movl	%esi,%eax		# phys address of ptd in proc 0
 	orl	$I386_CR3PAT,%eax
	movl	%eax,%cr3		# load ptd addr into mmu
	movl	%cr0,%eax		# get control word
	orl	$CR0_PG|CR0_PE,%eax	# and let's page!
	movl	%eax,%cr0		# NOW!

	pushl	$begin			# jump to high mem
	ret

begin: /* now running relocated at KERNBASE where the system is linked to run */

	movl	_atdevphys,%edx	# get pte PA
	subl	_KPTphys,%edx	# remove base of ptes, now have phys offset
	shll	$PGSHIFT-2,%edx  # corresponding to virt offset
	addl	$KERNBASE,%edx	# add virtual base
	movl	%edx, _atdevbase

	/* set up bootstrap stack */
	movl	$_kstack+UPAGES*NBPG-4*12,%esp	# bootstrap stack end location
	VPUT(0x49,8)	/* 'I' */
	xorl	%eax,%eax		# mark end of frames
	movl	%eax,%ebp
	movl	_proc0paddr, %eax
	movl	%esi, PCB_CR3(%eax)

	/* process boot parameters */
	pushl	_bootparamp
	call	_checkbootparams
	addl	$4,%esp

	lea	(1+UPAGES+1+1)*NBPG(%esi),%esi	# skip past stack.
	pushl	%esi
	
	VPUT(0x4a,9)	/* 'J' */
	DEL(1000000)
	call	_init386		# wire 386 chip for kernel operation
	
	movl	$0,_PTD
	call 	_main
	popl	%esi

	.globl	__ucodesel,__udatasel
	movzwl	__ucodesel,%eax
	movzwl	__udatasel,%ecx
	# build outer stack frame
	pushl	%ecx		# user ss
	pushl	$USRSTACK	# user esp
	pushl	%eax		# user cs
	pushl	$0		# user ip
	movw	%cx,%ds
	movw	%cx,%es
	# movw	%ax,%fs		# double map cs to fs
	# movw	%cx,%gs		# and ds to gs
	lret	# goto user!

	pushl	$lretmsg1	/* "should never get here!" */
	call	_panic
lretmsg1:
	.asciz	"lret: toinit\n"

	.globl	_icode
	.globl	_szicode

/*
 * Icode is copied out to process 1 to exec /sbin/init.
 * If the exec fails, process 1 exits.
 */
	.align	2
_icode:
	pushl	$0		/* envp */
	# pushl	$argv-_icode	# would like to subtract two symbols, but ...
	movl	$argv,%eax
	subl	$_icode,%eax
	pushl	%eax

	# pushl	$init-_icode
	movl	$init,%eax
	subl	$_icode,%eax
	pushl	%eax
	pushl	%eax	# dummy out rta

	movl	%esp,%ebp
	movl	$SYS_execve,%eax
	lcall	$LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	pushl	%eax
	movl	$SYS_exit,%eax
	pushl	%eax	# dummy out rta
	lcall	$LSEL(L43BSDCALLS_SEL,SEL_UPL),$0

init:
	.asciz	"/sbin/init"
	.align	2
argv:
	.long	init+6-_icode		# argv[0] = "init" ("/sbin/init" + 6)
	.long	eicode-_icode		# argv[1] follows icode after copyout
	.long	0
eicode:

_szicode:
	.long	_szicode-_icode

	.globl	_sigcode, _szsigcode
_sigcode:
	movl	12(%esp),%eax	# unsure if call will dec stack 1st
	call	%eax
	xorl	%eax,%eax	# smaller movl $103,%eax
	movb	$103,%al	# sigreturn()
	lcall	$LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	hlt			# never gets here

_szsigcode:
	.long	_szsigcode-_sigcode

ENTRY(__udivsi3)
	movl 4(%esp),%eax
	xorl %edx,%edx
	divl 8(%esp)
	ret

ENTRY(__divsi3)
	movl 4(%esp),%eax
	xorl %edx,%edx
	cltd
	idivl 8(%esp)
	ret

ENTRY(inb)
	movl	4(%esp),%edx
	subl	%eax,%eax	# clr eax
	inb	%dx,%al
	NOP
	ret

ENTRY(inw)
	movl	4(%esp),%edx
	subl	%eax,%eax	# clr eax
	inw	%dx,%ax
	NOP
	ret


ENTRY(rtcin)
	movl	4(%esp),%eax
	outb	%al,$0x70
	subl	%eax,%eax	# clr eax
	inb	$0x71,%al
	ret
	
ENTRY(rtcout)
	movl	4(%esp),%eax
	outb	%al,$0x70
	NOP	
	movb	8(%esp),%al
	outb	%al,$0x71
	NOP
	ret

ENTRY(outb)
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	NOP
	outb	%al,%dx
	inb	$0x84,%al
	NOP
	ret

ENTRY(outw)
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	NOP
	outw	%ax,%dx
	inb	$0x84,%al
	NOP
	ret

	#
	# fillw (pat,base,cnt)
	#

ENTRY(fillw)
	pushl	%edi
	movl	8(%esp),%eax
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	cld
	rep
	stosw
	popl	%edi
	ret

	# insb(port,addr,cnt)
ENTRY(insb)
	pushl	%edi
	movw	8(%esp),%dx
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	cld
	NOP
	rep
	insb
	NOP
	movl	%edi,%eax
	popl	%edi
	ret

	# insw(port,addr,cnt)
ENTRY(insw)
	pushl	%edi
	movw	8(%esp),%dx
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	cld
	rep; insw
	movl	%edi,%eax
	popl	%edi
	ret

	# insl(port,addr,cnt)
ENTRY(insl)
	pushl	%edi
	movw	8(%esp),%dx
	movl	12(%esp),%edi
	movl	16(%esp),%ecx
	cld
	rep; insl
	movl	%edi,%eax
	popl	%edi
	ret

        # outsb(port,addr,cnt)
ENTRY(outsb)
        pushl   %esi
        movw    8(%esp),%dx
        movl    12(%esp),%esi
        movl    16(%esp),%ecx
        cld
        rep
        outsb
        movl    %esi,%eax
        popl    %esi
        ret

	# outsw(port,addr,cnt)
ENTRY(outsw)
	pushl	%esi
	movw	8(%esp),%dx
	movl	12(%esp),%esi
	movl	16(%esp),%ecx
	cld
	rep; outsw
	movl	%esi,%eax
	popl	%esi
	ret

	# outsl(port,addr,cnt)
ENTRY(outsl)
	pushl	%esi
	movw	8(%esp),%dx
	movl	12(%esp),%esi
	movl	16(%esp),%ecx
	cld
	rep; outsl
	movl	%esi,%eax
	popl	%esi
	ret

	# lgdt(*gdt, ngdt)
	# .globl	_gdt
	.data
xxx:	.word 31
	.long 0
	.text
ENTRY(lgdt)
	movl	4(%esp),%eax
	movl	%eax,xxx+2
	movl	8(%esp),%eax
	movw	%ax,xxx
	lgdt	xxx
	jmp	1f
	NOP
1:	movw	$0x10,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$0x30,%ax
	movw	%ax,%ss
	movl	(%esp),%eax
	pushl	%eax
	movl	$8,4(%esp)
	lret

	# lidt(*idt, nidt)
	.data
yyy:	.word	255
	.long	0 
	.text
ENTRY(lidt)
	movl	4(%esp),%eax
	movl	%eax,yyy+2
	movl	8(%esp),%eax
	movw	%ax,yyy
	lidt	yyy
	ret

	# lldt(sel)
ENTRY(lldt)
	movl	4(%esp),%eax
	lldt	%eax
	ret

	# ltr(sel)
ENTRY(ltr)
	movl	4(%esp),%eax
	ltr	%eax
	ret

	# lcr3(cr3)
ALTENTRY(load_cr3, _lcr3)
ENTRY(lcr3)
	inb	$0x84,%al	# check wristwatch
	movl	4(%esp),%eax
 	orl	$I386_CR3PAT,%eax
	movl	%eax,%cr3
	inb	$0x84,%al	# check wristwatch
	ret

	# tlbflush()
ENTRY(tlbflush)
	inb	$0x84,%al	# check wristwatch
	movl	%cr3,%eax
 	orl	$I386_CR3PAT,%eax
	movl	%eax,%cr3
	inb	$0x84,%al	# check wristwatch
	ret

	# lcr0(cr0)
ALTENTRY(load_cr0, _lcr0)
ENTRY(lcr0)
	movl	4(%esp),%eax
	movl	%eax,%cr0
	ret

	# rcr0()
ENTRY(rcr0)
	movl	%cr0,%eax
	ret

	# rcr2()
ENTRY(rcr2)
	movl	%cr2,%eax
	ret

	# rcr3()
ALTENTRY(_cr3, _rcr3)
ENTRY(rcr3)
	movl	%cr3,%eax
	ret

	# ssdtosd(*ssdp,*sdp)
ENTRY(ssdtosd)
	pushl	%ebx
	movl	8(%esp),%ecx
	movl	8(%ecx),%ebx
	shll	$16,%ebx
	movl	(%ecx),%edx
	roll	$16,%edx
	movb	%dh,%bl
	movb	%dl,%bh
	rorl	$8,%ebx
	movl	4(%ecx),%eax
	movw	%ax,%dx
	andl	$0xf0000,%eax
	orl	%eax,%ebx
	movl	12(%esp),%ecx
	movl	%edx,(%ecx)
	movl	%ebx,4(%ecx)
	popl	%ebx
	ret

/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */

	.globl	_whichqs,_qs,_cnt,_panic
	.comm	_prevproc,4
	.comm	_runrun,4

/*
 * Setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrq)
	movl	4(%esp),%eax
	cmpl	$0,P_RLINK(%eax)	# should not be on q already
	je	set1
	pushl	$set2
	call	_panic
set1:
	movzbl	P_PRI(%eax),%edx
	shrl	$2,%edx
	btsl	%edx,_whichqs		# set q full bit
	shll	$3,%edx
	addl	$_qs,%edx		# locate q hdr
	movl	%edx,P_LINK(%eax)	# link process on tail of q
	movl	P_RLINK(%edx),%ecx
	movl	%ecx,P_RLINK(%eax)
	movl	%eax,P_RLINK(%edx)
	movl	%eax,P_LINK(%ecx)
	ret

set2:	.asciz	"setrq"

/*
 * Remrq(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
	movl	4(%esp),%eax
	movzbl	P_PRI(%eax),%edx
	shrl	$2,%edx
	btrl	%edx,_whichqs		# clear full bit, panic if clear already
	jb	rem1
	pushl	$rem3
	call	_panic
rem1:
	pushl	%edx
	movl	P_LINK(%eax),%ecx	# unlink process
	movl	P_RLINK(%eax),%edx
	movl	%edx,P_RLINK(%ecx)
	movl	P_RLINK(%eax),%ecx
	movl	P_LINK(%eax),%edx
	movl	%edx,P_LINK(%ecx)
	popl	%edx
	movl	$_qs,%ecx
	shll	$3,%edx
	addl	%edx,%ecx
	cmpl	P_LINK(%ecx),%ecx	# q still has something?
	je	rem2
	shrl	$3,%edx			# yes, set bit as still full
	btsl	%edx,_whichqs
rem2:
	movl	$0,P_RLINK(%eax)	# zap reverse link to indicate off list
	ret

rem3:	.asciz	"remrq"
sw0:	.asciz	"swtch"

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
ENTRY(Idle)
idle:
	movl	_curproc,%eax
	movl	%eax,_prevproc		# remember which proc is mapped
	movl	$0,_curproc		# don't bill idle time to previous proc
1:
	call	_spl0
	cmpl	$0,_whichqs
	jne	sw1
	hlt		# wait for interrupt
	jmp	1b

badsw:
	pushl	$sw0
	call	_panic
	/*NOTREACHED*/

/*
 * Swtch()
 */
ENTRY(swtch)

	incl	_cnt+V_SWTCH

	/* switch to new process. first, save context as needed */

	movl	_curproc,%ecx
swtch_entry:
	movl	P_ADDR(%ecx),%ecx


	movl	(%esp),%eax		# Hardware registers
	movl	%eax, PCB_EIP(%ecx)
	movl	%ebx, PCB_EBX(%ecx)
	movl	%esp, PCB_ESP(%ecx)
	movl	%ebp, PCB_EBP(%ecx)
	movl	%esi, PCB_ESI(%ecx)
	movl	%edi, PCB_EDI(%ecx)

/* the following section is used on with an NPX enabled */
	.globl	_npxproc
	movl	%cr0, %eax
	testb	$CR0_EM, %al
	jne	1f
#ifdef notyet
	fwait				/* paranoid, get exceptions now */
#else
	fnsave	PCB_SAVEFPU(%ecx)
	movl	$0, _npxproc
#endif
	orb 	$CR0_EM,%al		/* disable fpu */
	movl	%eax,%cr0
1:
/* end only with NPX */

	movl	_CMAP2,%eax		# save temporary map PTE
	movl	%eax,PCB_CMAP2(%ecx)	# in our context

	# movw	_cpl, %ax
	# movw	%ax, PCB_IML(%ecx)	# save ipl

	/* save is done, now choose a new process or idle */
sw1:
	cli				# XXX?
	movl	_whichqs,%edi
2:
	bsfl	%edi,%eax		# find a full q
	jz	idle			# if none, idle
	/* XX update whichqs? */
swfnd:
	btrl	%eax,%edi		# clear q full status
	jnb	2b			# if it was clear, look for another
	movl	%eax,%ebx		# save which one we are using

	shll	$3,%eax
	addl	$_qs,%eax		# select q
	movl	%eax,%esi

#ifdef	DIAGNOSTIC
	cmpl	P_LINK(%eax),%eax # linked to self? (e.g. not on list)
	je	badsw			# not possible
#endif

	movl	P_LINK(%eax),%ecx	# unlink from front of process q
	movl	P_LINK(%ecx),%edx
	movl	%edx,P_LINK(%eax)
	movl	P_RLINK(%ecx),%eax
	movl	%eax,P_RLINK(%edx)

	cmpl	P_LINK(%ecx),%esi	# q empty
	je	3f
	btsl	%ebx,%edi		# nope, set to indicate full
3:
	movl	%edi,_whichqs		# update q status

	movl	$0,%eax
	movl	%ecx,_curproc
	movl	%eax,_want_resched

#ifdef	DIAGNOSTIC
	cmpl	%eax,P_WCHAN(%ecx)
	jne	badsw
	cmpb	$SRUN,P_STAT(%ecx)
	jne	badsw
#endif

	movl	%eax,P_RLINK(%ecx) /* isolate process to run */
	movl	P_ADDR(%ecx),%edx
	movl	%edx,_curpcb
	movl	PCB_CR3(%edx),%ebx
#ifdef CHKCR3
	cmpl	$0,_chkcr3_enabled
	je	1f
	/* make sure we aren't committing suicide */
	pusha
	nop
	push	%ecx
	push	%ebx
	call	_chkcr3
	pop	%ebx
	pop	%ecx
	popa
1:
#endif

	/* switch address space */
	movl	%esp,%ecx
	movl	$tmpstk,%esp
 	orl	$I386_CR3PAT,%ebx
	inb	$0x84,%al	# flush write buffers
	movl	%ebx,%cr3	# context switch address space
#ifdef notdef
	movl	(%ecx),%eax	# touch stack, fault if not there
	movl	%eax,(%ecx)
	movl	%ecx,%esp
#endif

	/* restore context */
	movl	PCB_EBX(%edx), %ebx
	movl	PCB_ESP(%edx), %esp
	movl	PCB_EBP(%edx), %ebp
	movl	PCB_ESI(%edx), %esi
	movl	PCB_EDI(%edx), %edi
	movl	PCB_EIP(%edx), %eax
	movl	%eax, (%esp)

	movl	PCB_CMAP2(%edx),%eax	# get temporary map
	movl	%eax,_CMAP2		# reload temporary map PTE

	movl	PCB_LDTDEFCALL(%edx),%eax	/* install the default ... */
	movl	%eax,_ldt+(LDEFCALLS_SEL*8)	/* ... system call descriptor */
	movl	(PCB_LDTDEFCALL+4)(%edx),%eax
	movl	%eax,_ldt+((LDEFCALLS_SEL*8)+4)

	# pushl	PCB_IML(%edx)
	# call	_splx
	# popl	%eax

	movl	%edx,%eax		# return (1);
	ret

/*
 * swtch_exit() -- Crude, temporary hack so that curproc may be left unset
 * while this process makes its last context switch.  We can't permit
 * curproc to be set, since hardclock looks at it and does awful things.
 */
ENTRY(swtch_exit)
	incl	_cnt+V_SWTCH
	movl	4(%esp),%ecx
	jmp	swtch_entry

/*
 * struct proc *swtch_to_inactive(p) ; struct proc *p;
 *
 * At exit of a process, move off the address space of the
 * process and onto a "safe" one. Then, on a temporary stack
 * return and run code that disposes of the old state.
 * Since this code requires a parameter from the "old" stack,
 * pass it back as a return value.
 */
ENTRY(swtch_to_inactive)
	popl	%edx			# old pc
	popl	%eax			# arg, our return value
	movl	_IdlePTD,%ecx
	movl	%ecx,%cr3		# good bye address space
 #write buffer?
	movl	$tmpstk-4,%esp		# temporary stack, compensated for call
	jmp	%edx			# return, execute remainder of cleanup

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in swtch if altreturn is true.
 */
ENTRY(savectx)
	movl	4(%esp), %ecx
	# movw	_cpl, %ax
	# movw	%ax,  PCB_IML(%ecx)
	movl	(%esp), %eax	
	movl	%eax, PCB_EIP(%ecx)
	movl	%ebx, PCB_EBX(%ecx)
	movl	%esp, PCB_ESP(%ecx)
	movl	%ebp, PCB_EBP(%ecx)
	movl	%esi, PCB_ESI(%ecx)
	movl	%edi, PCB_EDI(%ecx)
/* the following section is used on with an NPX enabled */
	/*
	 * If the current process has used floating point,
	 * either the current savefpu area is valid, in which case
	 * we don't need to do anything here, or the caller has
	 * used floating point since the last context switch.
	 * In this case, the CR0_EM bit will be off, and we need
	 * to save the current state in the new pcb.
	 */
	movl	%cr0, %edx
	testb	$CR0_EM, %dl
	jne	1f
	fnsave	PCB_SAVEFPU(%ecx)
1:
/* end only with NPX */
	movl	_CMAP2, %edx		# save temporary map PTE
	movl	%edx, PCB_CMAP2(%ecx)	# in our context

	cmpl	$0, 8(%esp)
	je	1f
	movl	%esp, %edx		# relocate current sp relative to pcb
	subl	$_kstack, %edx		#   (sp is relative to kstack):
	addl	%edx, %ecx		#   pcb += sp - kstack;
	movl	%eax, (%ecx)		# write return pc at (relocated) sp@
	# this mess deals with replicating register state gcc hides
	movl	12(%esp),%eax
	movl	%eax,12(%ecx)
	movl	16(%esp),%eax
	movl	%eax,16(%ecx)
	movl	20(%esp),%eax
	movl	%eax,20(%ecx)
	movl	24(%esp),%eax
	movl	%eax,24(%ecx)
1:
	xorl	%eax, %eax		# return 0
	ret

ENTRY(mvesp)
	movl	%esp,%eax
	ret

/*
 * update profiling information for the user
 * addupc(pc, up, ticks) struct uprof *up;
 */

ENTRY(addupc)
	movl	4(%esp),%eax		/* pc */
	movl	8(%esp),%ecx		/* up */

	/* does sampled pc fall within bottom of profiling window? */
	subl	PR_OFF(%ecx),%eax 	/* pc -= up->pr_off; */
	jl	1f 			/* if (pc < 0) return; */

	/* construct scaled index */
	shrl	$1,%eax			/* reduce pc to a short index */
	mull	PR_SCALE(%ecx)		/* pc*up->pr_scale */
	shrdl	$15,%edx,%eax 		/* praddr >> 15 */
	cmpl	$0,%edx			/* if overflow, ignore */
	jne	1f
	andb	$0xfe,%al		/* praddr &= ~1 */

	/* within profiling buffer? if so, compute address */
	cmpl	%eax,PR_SIZE(%ecx)	/* if (praddr >= up->pr_size) return; */
	jle	1f
	addl	PR_BASE(%ecx),%eax	/* praddr += up->pr_base; */

	/* tally ticks to selected counter */
	movl	_curpcb,%ecx
	movl	$proffault,PCB_ONFAULT(%ecx) #in case we page/protection violate
	movl	12(%esp),%edx		/* ticks */
	addw	%dx,(%eax)
	movl	$0,PCB_ONFAULT(%ecx)
1:	ret

proffault:
	/* disable profiling if we get a fault */
	movl	$0,PR_SCALE(%ecx) /*	up->pr_scale = 0; */
	movl	_curpcb,%ecx
	movl	$0,PCB_ONFAULT(%ecx)
	ret

.data
	.globl	_cyloffset, _curpcb
_cyloffset:	.long	0
	.globl	_proc0paddr
_proc0paddr:	.long	0
LF:	.asciz "swtch %x"

#define	IDTVEC(name)	.align 4; .globl _X/**/name; _X/**/name:
#define	PANIC(msg)	xorl %eax,%eax; movl %eax,_waittime; pushl 1f; \
			call _panic; 1: .asciz msg
#define	PRINTF(n,msg)	pushal; nop; pushl 1f; call _printf; MSG(msg) ; \
			 popl %eax ; popal
#define	MSG(msg)	.data; 1: .asciz msg; .text

	.text

/*
 * Trap and fault vector routines
 */ 
#define	TRAP(a)		pushl $a ; jmp alltraps
#ifdef KGDB
#define	BPTTRAP(a)	pushl $a ; jmp bpttraps
#else
#define	BPTTRAP(a)	TRAP(a)
#endif

IDTVEC(div)
	pushl $0; TRAP(T_DIVIDE)
IDTVEC(dbg)
	pushl $0; BPTTRAP(T_TRCTRAP)
IDTVEC(nmi)
	pushl $0; TRAP(T_NMI)
IDTVEC(bpt)
	pushl $0; BPTTRAP(T_BPTFLT)
IDTVEC(ofl)
	pushl $0; TRAP(T_OFLOW)
IDTVEC(bnd)
	pushl $0; TRAP(T_BOUND)
IDTVEC(ill)
	pushl $0; TRAP(T_PRIVINFLT)
IDTVEC(dna)
	pushl $0; TRAP(T_DNA)
IDTVEC(dble)
	TRAP(T_DOUBLEFLT)
	/*PANIC("Double Fault");*/
IDTVEC(fpusegm)
	pushl $0; TRAP(T_FPOPFLT)
IDTVEC(tss)
	TRAP(T_TSSFLT)
	/*PANIC("TSS not valid");*/
IDTVEC(missing)
	TRAP(T_SEGNPFLT)
IDTVEC(stk)
	TRAP(T_STKFLT)
IDTVEC(prot)
	TRAP(T_PROTFLT)
IDTVEC(page)
	TRAP(T_PAGEFLT)
IDTVEC(rsvd)
	pushl $0; TRAP(T_RESERVED_15)
IDTVEC(fpu)
	pushl $0; TRAP(T_ARITHTRAP)
IDTVEC(align)
	pushl $0; TRAP(T_ALIGNFLT)
	/* 18 - 31 reserved for future exp */
IDTVEC(rsvd1)
	pushl $0; TRAP(T_RESERVED_18)
IDTVEC(rsvd2)
	pushl $0; TRAP(T_RESERVED_19)
IDTVEC(rsvd3)
	pushl $0; TRAP(T_RESERVED_20)
IDTVEC(rsvd4)
	pushl $0; TRAP(T_RESERVED_21)
IDTVEC(rsvd5)
	pushl $0; TRAP(T_RESERVED_22)
IDTVEC(rsvd6)
	pushl $0; TRAP(T_RESERVED_23)
IDTVEC(rsvd7)
	pushl $0; TRAP(T_RESERVED_24)
IDTVEC(rsvd8)
	pushl $0; TRAP(T_RESERVED_25)
IDTVEC(rsvd9)
	pushl $0; TRAP(T_RESERVED_26)
IDTVEC(rsvd10)
	pushl $0; TRAP(T_RESERVED_27)
IDTVEC(rsvd11)
	pushl $0; TRAP(T_RESERVED_28)
IDTVEC(rsvd12)
	pushl $0; TRAP(T_RESERVED_29)
IDTVEC(rsvd13)
	pushl $0; TRAP(T_RESERVED_30)
IDTVEC(rsvd14)
	pushl $0; TRAP(T_RESERVED_31)

alltraps:
	pushal
	nop
	push %ds
	push %es
	movw	$0x10,%ax
	movw	%ax,%ds
	movw	%ax,%es
calltrap:
	call	_trap
	pop %es
	pop %ds
	popal
	nop
	addl	$8,%esp			# pop type, code

	.globl _iret_from_trap
_iret_from_trap:
	iret

#ifdef KGDB
/*
 * This code checks for a kgdb trap, then falls through
 * to the regular trap code.
 */
bpttraps:
	pushal				# save trapframe
	nop
	push	%es
	push	%ds
	movw	$0x10,%ax		# switch to kernel data segment
	movw	%ax,%ds
	movw	%ax,%es
	xorl	%eax,%eax
	movw	52(%esp),%eax		# test code segment
	testl	$3,%eax			# was it user code?
	jne	calltrap		# yes, call trap
	cli				# no, lock out interrupts and call kgdb
	call	_kgdb_trap_glue		#   (interrupts are re-enabled by iret)
	/*
	 * Note: kgdb will do its own iret and never return.  However,
	 * if the debugger isn't there (i.e., it's improperly configured)
	 * then it can return --- in this case, we'll panic via trap.
	 */
	jmp	calltrap
#endif

/*
 * Call gate entry for syscall
 */

IDTVEC(syscall)
	pushfl	# only for stupid carry bit and more stupid wait3 cc kludge
	pushal	# only need eax,ecx,edx - trap resaves others
	nop
	movw	$0x10,%ax	# switch to kernel segments
	movw	%ax,%ds
	movw	%ax,%es
	call	_syscall
	movw	__udatasel,%ax	# switch back to user segments
	movw	%ax,%ds
	movw	%ax,%es
	popal
	nop
	popfl

	/*
	 * Note: if we ever change syscall to return with an iret,
	 * will will have to make sure we clear the PSL_NT first.
	 * lret ignores this bit, so we don't have to worry about
	 * it now.
	 */
	.globl _lret_from_syscall
_lret_from_syscall:
	lret

/*
 * Switch to vm86 mode.
 * We clean kernel stack each time.
 */

ENTRY(switch_to_vm86)
	cli
	movl	4(%esp), %esi			# from
	movl	8(%esp), %edi			# to
	movl	12(%esp), %ecx			# count
	movl	%edi, %esp			# new stack
	shrl	$2,%ecx	
	cld
	rep
	movsl
# below ala return from trap
	pop	%eax				# drop es
	pop	%eax				# drop ds
	pop	%edi
	pop	%esi
	pop	%ebp
	pop	%eax				# drop sp
	pop	%ebx
	pop	%edx
	pop	%ecx
	pop	%eax
	addl	$8,%esp			# pop type, code
	iret


ALTENTRY(htonl, _ntohl)
ENTRY(ntohl)
	movl	4(%esp),%eax
	xchgb	%al,%ah
	roll	$16,%eax
	xchgb	%al,%ah
	ret

ALTENTRY(htons, _ntohs)
ENTRY(ntohs)
	movzwl	4(%esp),%eax
	xchgb	%al,%ah
	ret

#ifndef HZ
/*
 * Implementation of microtime that reads timer 0 to determine
 * the time within the current tick.  Assumes 100 Hz clock.
 */
ENTRY(microtime)
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	$_time,%ebx

	cli				# disable interrupts

	movl	(%ebx),%edi		# sec = time.tv_sec
	movl	4(%ebx),%esi		# usec = time.tv_usec

	movl	$(TIMER_SEL0|TIMER_LATCH),%eax
	outb	%al,$TIMER_MODE		# latch timer 0's counter
	inb	$0x84,%al		# delay

	#	
	# Read counter value into ebx, LSB first
	#
	inb	$TIMER_CNTR0,%al
	movzbl	%al,%ebx
	inb	$TIMER_CNTR0,%al
	movb	%al,%bh

	cmpl	$11932,%ebx	# check for bogus values
	ja	3f		# if bogus, don't use it

	#
	# Now check for counter overflow.  This is tricky because the
	# timer chip doesn't let us atomically read the current counter
	# value and the output state (i.e., overflow state).  We have
	# to read the ICU interrupt request register (IRR) to see if the
	# overflow has occured.  Because we lack atomicity, we use
	# the (completely accurate) heuristic that we only check for
	# overflow if the value read is close to the interrupt period.
	# E.g., if we just checked the IRR, we might read a non-overflowing
	# value close to 0, experience overflow, then read this overflow
	# from the IRR, and mistakingly add a correction to the "close
	# to zero" value.
	#
	# We compare the counter value to heuristic constant 11890.
	# If the counter value is less than this, we assume the counter
	# didn't overflow between disabling interrupts above and latching
	# the counter value.  For example, we assume that the above 10 or so
	# instructions take less than 11932 - 11890 = 42 microseconds to
	# execute.
	#
	# Otherwise, the counter might have overflowed.  We check for this
	# condition by reading the interrupt request register out of the ICU.
	# If it overflowed, we add in one clock period.
	#
	cmpl	$11890,%ebx
	jle	2f
	movl	$0x0a,%eax	# tell ICU we want IRR
	outb	%al,$IO_ICU1
	inb     $0x84,%al

	inb	$IO_ICU1,%al	# read IRR in ICU
	testb	$1,%al		# is a timer interrupt pending?
	je	1f
	addl	$-11932,%ebx	# yes, subtract one clock period
1:
	movl	$0x0b,%eax	# tell ICU we want ISR 
	outb	%al,$IO_ICU1	#   (rest of kernel expects this)
	# inb     $0x84,%al # XXX why does outb() do this? 
2:
	sti			# enable interrupts

	movl	$11932,%eax	# subtract counter value from 11932 since
	subl	%ebx,%eax	#   it is a count-down value
	imull	$1000,%eax,%eax
	movl	$0,%edx		# zero extend eax for div
	movl	$1193,%ecx
	idivl	%ecx		# convert to usecs: mult by 1000/1193

	addl	%eax,%esi	# add counter usecs to time.tv_usec
	cmpl	$1000000,%esi	# carry in timeval?
	jl	3f
	subl	$1000000,%esi	# adjust usec
	incl	%edi		# bump sec
3:
	movl	16(%esp),%ecx	# load timeval pointer arg
	movl	%edi,(%ecx)	# tvp->tv_sec = sec
	movl	%esi,4(%ecx)	# tvp->tv_usec = usec

	popl	%ebx		# restore regs
	popl	%esi
	popl	%edi
	ret
#endif /* HZ */

#include "i386/isa/icu.s"
