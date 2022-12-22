/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: vmlayout.h,v 1.3 1993/11/11 23:42:45 karels Exp $
 */

/*
 * Virtual address space arrangement.  These definitions were previously
 * scattered among "machine/param.h", "machine/vmparam.h" and locore.s.
 *
 * On 386, both user and kernel share the address space,
 * similar to the VAX.  However, unlike the VAX, the division
 * of the address space into user and kernel areas is not fixed
 * by the hardware.
 *
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack. Immediately above the user stack
 * is a space (kstack) used for double-mapping the user structure,
 * which is UPAGES long and contains the kernel stack.
 *
 * Immediately after the kernel stack is the page-table map (PTmap),
 * the kernel address space, and the alternate page-table map (APTmap).
 *
 * This layout places the per-process user area first, followed
 * by the per-process kernel areas (kstack and page-table pages for
 * per-process user and kernel areas), and then the global kernel
 * addresses.  This arrangement is reflected in the page tables
 * for each process, and that in turn is reflected in the layout
 * of the page table directory (PDR), which maps the page tables.
 *
 * The page table directory is used by the system as a page-table page
 * in order to map the page tables (including itself) into virtual memory.
 * The page tables are mapped into the address space between the
 * user and kernel areas so that the per-process kernel addresses
 * including the page table area follow the per-process user addresses,
 * and the global kernel page table area falls just below the global
 * kernel area.
 *
 * The layout is defined here in terms of indices in the page table
 * directory.  Each page table directory entry (PDE) maps a page of page
 * table space, which in turn maps a 4 MB address region.
 */

#ifndef _VMLAYOUT_H_
#define _VMLAYOUT_H_

#if 0
/*
 * the following are not used (?)
 */
#define	I386_KPDES	8	/* KPT page directory size, not all used */
#define	I386_UPDES	NBPDR/sizeof(struct pde)-8 /* UPT page directory size */
#endif

/*
 * Layout of the PTD.  These constants are indices;
 * the PTD contains 1024 entries, 0x000 through 0x3ff.
 * Entries 0 through KSTKPTDI map the current user space plus kstack.
 * Entry PTDPTDI points to the current PTD, and thus maps the current
 * page tables.  Entries KPTDI_FIRST through KPTDI_LAST map the kernel
 * page table pages allocated at startup, and used in all processes.
 * The kernel page tables are sized at boot time.  They are never extended
 * in the current implementation; extension would require that the PTD
 * for each process be updated.
 */
#define	KSTKPTDI	0x3be		/* PTD index ending with kstack */
#define	PTDPTDI		0x3bf		/* PTD entry that points to PTD! */
#define	KPTDI_FIRST	0x3c0		/* starting index of kernel PDEs */
#define	APTDPTDI	0x3fe		/* PTD entry that points to APTD */
/* 0x3ff is currently unused */
#define	NKPTD_DFLT	3		/* default num. of kernel PDEs to use */
#define	NKPTD		(APTDPTDI - KPTDI_FIRST)	/* max kernel PDEs */
#define	KPTDI_LAST	(KPTDI_FIRST + NKPTD - 1) /* index of last kernel PDE */

/*
 * user/kernel map constants; derived from PTD layout.
 * _KSTACK ends at the end of the 4 MB area for KSTKPTDI,
 * and is UPAGES long; USRSTACK ends where _KSTACK begins.
 * (KSTACK should be removed, but cpu_fork does not yet relocate
 * the kernel stack when copying.)
 */
#define	USRTEXT		0
#ifdef LOCORE
#define	VM_MIN_ADDRESS	0
#else
#define	VM_MIN_ADDRESS	((vm_offset_t) 0)
#endif
#define	USRSTACK	(((KSTKPTDI + 1) << PDRSHIFT) - UPAGES * NBPG)
#define	_KSTACK		(((KSTKPTDI + 1) << PDRSHIFT) - UPAGES * NBPG)
#ifdef LOCORE
#define	VM_MAXUSER_ADDRESS	USRSTACK
#else
#define	VM_MAXUSER_ADDRESS	((vm_offset_t) USRSTACK)
#endif

/*
 * The PTD, used as a page table page, maps the page table space
 * (PTmap[], set in locore to _PTMAP).  User page tables come first.
 * User page tables end just before the (recursively mapped) PTD.
 * Kernel page tables begin after the PTD.  Finally, there is
 * an alternate pagemap area APTmap[] at _APTMAP, used to map
 * the page tables of another process temporarily.
 */
#define	_PTMAP			(PTDPTDI << PDRSHIFT)
#define	UPT_MIN_ADDRESS		((vm_offset_t) _PTMAP)
#define	UPT_MAX_ADDRESS		((vm_offset_t) (_PTMAP + (PTDPTDI << PGSHIFT)))
#define	VM_MAX_ADDRESS		UPT_MAX_ADDRESS

#define	VM_MIN_KERNEL_ADDRESS	UPT_MAX_ADDRESS
#define	KPT_MIN_ADDRESS	((vm_offset_t) (_PTMAP + (KPTDI_FIRST << PGSHIFT)))

#define	KERNBASE	(KPTDI_FIRST << PDRSHIFT)	/* VA of kernel */
#define	VM_MAX_KERNEL_ADDRESS	((vm_offset_t) ((KPTDI_LAST + 1) << PDRSHIFT))
#define	_APTMAP		(APTDPTDI << PDRSHIFT)

#endif /* _VMLAYOUT_H_ */
