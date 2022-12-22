/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: copy.s,v 1.3 1992/10/25 21:27:19 karels Exp $
 */

#include "assym.s"
#include "machine/vmlayout.h"
#include "machine/pte.h"

#ifdef GPROF
#define	ENTRY(name) \
	.globl _/**/name; .align 2; _/**/name: pushl %ebp; movl %esp,%ebp; \
	.data; .align 2; LP/**/name: .long 0; \
	.text; movl $LP/**/name,%eax; call mcount; leave; 1:
#define	ALTENTRY(name, rname) \
	ENTRY(name); jmp 1f
#else
#define	ENTRY(name) \
	.globl _/**/name; .align 2; _/**/name:
#define	ALTENTRY(name, rname) \
	.globl _/**/name; .align 2; _/**/name:
#endif

	.text

/*
 * Copy within kernel space.
 */

/*
 * Get parameters into the right registers for string instructions.
 */
#define	COPY_PROLOGUE() \
	pushl %esi; \
	pushl %edi; \
	movl 12(%esp),%esi;	/* source */ \
	movl 16(%esp),%edi;	/* destination */ \
	movl 20(%esp),%ecx	/* length */

#define	COPY_EPILOGUE() \
	popl %edi; \
	popl %esi; \
	ret

/*
 * Copy %ecx bytes from (%esi) to (%edi).
 * We assume initial word alignment and no overlapping.
 */
#define	COPY() \
	cld; \
	movl %ecx,%eax; \
	andl $3,%eax; \
	shrl $2,%ecx; \
	rep; movsl; \
	movl %eax,%ecx; \
	rep; movsb

/*
 * Copy routine for non-overlapping regions.
 * Move len bytes of data from src to dst.
 *
 * void bcopy(void *src, void *dst, size_t len);
 */
ENTRY(bcopy)
	COPY_PROLOGUE()
Lbc_doit:	
	COPY()
	COPY_EPILOGUE()

/*
 * Copy routine for regions that may overlap.
 * Move len bytes of data from src to dst even if src < dst < src + len.
 *
 * void ovbcopy(void *src, void *dst, size_t len);
 */
ENTRY(ovbcopy)
	COPY_PROLOGUE()
	cmpl %esi,%edi
	jbe Lbc_doit	/* if destination precedes source, copy forward */

	std		/* evidently reverse copying is sexually transmitted */
	addl %ecx,%esi
	decl %esi
	addl %ecx,%edi
	decl %edi
	movl %ecx,%eax
	andl $3,%ecx
	shrl $2,%eax
	rep; movsb
	movl %eax,%ecx
	subl $3,%esi	/* point at first byte in word, not last byte */
	subl $3,%edi
	rep; movsl
	cld		/* XXX necessary? */

	COPY_EPILOGUE()

/*
 * Compare two blocks of data b1 and b2 of length len.
 *
 * int bcmp(void *b1, void *b2, size_t len);
 */
ENTRY(bcmp)
	COPY_PROLOGUE()

	cld
	movl %ecx,%eax
	andl $3,%eax
	shrl $2,%ecx
	repe; cmpsl
	jne 1f
	movl %eax,%ecx
	repe; cmpsb
	jne 1f

	xorl %eax,%eax

2:
	COPY_EPILOGUE()

1:
	movl $1,%eax
	jmp 2b

/*
 * Zero out a block of data dst of len bytes.
 *
 * void bzero(void *dst, size_t len);
 */
ALTENTRY(blkclr, _bzero)
ENTRY(bzero)
	pushl %edi

	movl 8(%esp),%edi
	movl 12(%esp),%ecx

	cld
	movl %ecx,%edx
	andl $3,%edx
	shrl $2,%ecx
	xorl %eax,%eax
	rep; stosl
	movl %edx,%ecx
	rep; stosb

	popl %edi
	ret

/*
 * Copy a string src to a location dst, both in the kernel.
 * We never copy more than maxlen bytes.
 * If lenp is nonzero, we store the string length in *lenp.
 * We assume that the regions don't overlap, and that we need not
 * copy anything if the maximum length is too short.
 * We do the work by scanning for a null, then copying forward.
 * We return ENAMETOOLONG if there wasn't enough space, 0 otherwise.
 *
 * int copystr(char *src, char *dst, size_t maxlen, size_t *lenp);
 */
ENTRY(copystr)
	COPY_PROLOGUE()
	movl %ecx,%edx
	movl %esi,%edi

	cld
	xorb %al,%al
	repne; scasb		/* find the end of the string */

	jnz Lcs_toolong		/* if we didn't find it, we lose */

	subl %ecx,%edx
	movl %edx,%ecx
	movl 16(%esp),%edi

	COPY()

	movl 24(%esp),%eax
	testl %eax,%eax
	je Lcs_exit
	movl %edx,(%eax)	/* store length copied */
	xorl %eax,%eax

Lcs_exit:
	COPY_EPILOGUE()

Lcs_toolong:
	movl $ENAMETOOLONG,%eax
	jmp Lcs_exit


/*
 * Copy from user space.
 */

#define	SET_COPY_FAULT(f) \
	movl _curpcb,%edx; \
	movl $f,PCB_ONFAULT(%edx)

#define	CLEAR_COPY_FAULT() \
	movl _curpcb,%edx; \
	movl $0,PCB_ONFAULT(%edx)

#if VM_MIN_ADDRESS != 0
#define	VALIDATE_ADDRESS(r,f) \
	cmpl $VM_MAXUSER_ADDRESS,r; \
	jae f; \
	cmpl $VM_MIN_ADDRESS,r; \
	jb f
#else
#define	VALIDATE_ADDRESS(r,f) \
	cmpl $VM_MAXUSER_ADDRESS,r; \
	jae f
#endif

#define	CHECK_ADDRESS_WRAP(ptr,len,sum,f) \
	movl ptr,sum; \
	addl len,sum; \
	cmpl ptr,sum; \
	jb f

/*
 * Fetch a byte from the user's address space.
 * Return -1 on error.
 *
 * int fubyte(void *), fuibyte(void *);
 */
ALTENTRY(fuibyte, _fubyte)
ENTRY(fubyte)
	movl 4(%esp),%eax
	SET_COPY_FAULT(Lfb_fault)
	VALIDATE_ADDRESS(%eax,Lfb_fault)
	movzbl (%eax),%eax

Lfb_done:
	CLEAR_COPY_FAULT()
	ret

Lfb_fault:
	movl $-1,%eax
	jmp Lfb_done

/*
 * Fetch a short from the user's address space.
 *
 * int fusword(void *);
 */
ENTRY(fusword)
	movl 4(%esp),%eax
	SET_COPY_FAULT(Lfw_fault)
	VALIDATE_ADDRESS(%eax,Lfw_fault)
	leal 1(%eax),%ecx	/* don't worry about wrapping */
	VALIDATE_ADDRESS(%ecx,Lfw_fault)
	movzwl (%eax),%eax
	jmp Lfw_done

/*
 * Fetch a word from the user's address space.
 * Returns -1 on error (not a lot of help!).
 *
 * int fuword(void *), fuiword(void *);
 */
ALTENTRY(fuiword, _fuword)
ENTRY(fuword)
	movl 4(%esp),%eax
	SET_COPY_FAULT(Lfw_fault)
	VALIDATE_ADDRESS(%eax,Lfw_fault)
	leal 3(%eax),%ecx	/* don't worry about wrapping */
	VALIDATE_ADDRESS(%ecx,Lfw_fault)
	movl (%eax),%eax
	jmp Lfw_done

Lfw_done:
	CLEAR_COPY_FAULT()
	ret

Lfw_fault:
	movl $-1,%eax
	jmp Lfw_done

/*
 * Copy len bytes from an address src in user space to
 * an address dst in kernel space.
 *
 * int copyin(void *src, void *dst, size_t len);
 */
ENTRY(copyin)
	COPY_PROLOGUE()
	SET_COPY_FAULT(Lci_fault)
	VALIDATE_ADDRESS(%esi,Lci_fault)
	CHECK_ADDRESS_WRAP(%esi,%ecx,%eax,Lci_fault)
	decl %eax	/* address of last byte */
	VALIDATE_ADDRESS(%eax,Lci_fault)
	COPY()
	xorl %eax,%eax

Lci_exit:
	CLEAR_COPY_FAULT()
	COPY_EPILOGUE()

Lci_fault:
	movl $EFAULT,%eax
	jmp Lci_exit

/*
 * Copyinstr() is very similar to copystr(),
 * except the source lies in user space.
 *
 * int copyinstr(char *src, char *dst, size_t maxlen, size_t *lenp);
 */
ENTRY(copyinstr)
	COPY_PROLOGUE()
	SET_COPY_FAULT(Lcis_fault)
	VALIDATE_ADDRESS(%esi,Lcis_fault)

	/*
	 * Take care not let the string scan
	 * run off the end of user space.
	 */
	CHECK_ADDRESS_WRAP(%esi,%ecx,%eax,1f)
	cmpl $VM_MAXUSER_ADDRESS,%eax
	jbe 2f
1:
	movl $VM_MAXUSER_ADDRESS,%ecx
	subl %esi,%ecx
2:

	movl %ecx,%edx
	movl %esi,%edi

	cld
	xorb %al,%al
	repne; scasb		/* find the end of the string */

	jnz Lcis_toolong	/* if we didn't find it, we lose */

	subl %ecx,%edx
	movl %edx,%ecx
	movl 16(%esp),%edi

	COPY()

	movl 24(%esp),%eax
	testl %eax,%eax
	je Lcis_exit
	movl %edx,(%eax)	/* store length copied */
	xorl %eax,%eax

Lcis_exit:
	CLEAR_COPY_FAULT()
	COPY_EPILOGUE()

Lcis_toolong:
	movl $ENAMETOOLONG,%eax
	jmp Lcis_exit

Lcis_fault:
	movl $EFAULT,%eax
	jmp Lcis_exit


/*
 * Copy to user space.
 */

/*
 * Copy a byte to user space.
 * Return -1 on error.
 *
 * int subyte(void *, int), suibyte(void *, int);
 */
ALTENTRY(suibyte, _subyte)
ENTRY(subyte)
	SET_COPY_FAULT(Lsb_fault)

	movl 4(%esp),%edx
	VALIDATE_ADDRESS(%edx,Lsb_fault)

	cmpl $CPU_386,_cpu
	jg Lsb_doit		/* the 486 can generate a COW fault */

	/*
	 * If the page is read-only, force a fault.
	 */
	movl %edx,%ecx 
	shrl $PGSHIFT,%ecx
	leal _PTmap(,%ecx,4),%ecx
	movb (%ecx),%al
	andb $PG_V|PG_PROT,%al
	cmpb $PG_V|PG_URKR,%al
	jne Lsb_doit
	andb $~PG_V,(%ecx)	/* we lose -- invalidate the page */
	movl %cr3,%eax		/* and flush the TLB */
	movl %eax,%cr3

Lsb_doit:
	movb 8(%esp),%al
	movb %al,(%edx)
	xorl %eax,%eax

Lsb_exit:
	CLEAR_COPY_FAULT()
	ret

Lsb_fault:
	movl $-1,%eax
	jmp Lsb_exit

/*
 * Copy a short to user space.
 *
 * int susword(void *, int);
 */
ENTRY(susword)
	SET_COPY_FAULT(Lsb_fault)

	movl 4(%esp),%edx
	VALIDATE_ADDRESS(%edx,Lsb_fault)

	cmpl $CPU_386,_cpu
	jg Lss_doit		/* the 486 can generate a COW fault */

	/*
	 * If the page(s) is/are read-only, force a fault.
	 */
	movl %edx,%ecx
	shrl $PGSHIFT,%ecx
	leal 1(%edx),%eax	/* ending addr; don't worry about wrapping */
	shrl $PGSHIFT,%eax
	cmpl %ecx,%eax		/* end on same page as start: only 1 page */
	leal _PTmap(,%ecx,4),%ecx	/* doesn't modify ZF */
	je Lss_chk2		/* if only one page, skip first test */
	movb (%ecx),%al
	andb $PG_V|PG_PROT,%al
	cmpb $PG_V|PG_URKR,%al
	jne Lss_chk2
	andb $~PG_V,(%ecx)	/* we lose -- invalidate the page */
	movl %cr3,%eax		/* and flush the TLB */
	movl %eax,%cr3
	leal 4(%ecx),%ecx	/* next pte */
Lss_chk2:
	movb (%ecx),%al		/* check first/second page */
	andb $PG_V|PG_PROT,%al
	cmpb $PG_V|PG_URKR,%al
	jne Lss_doit
	andb $~PG_V,(%ecx)	/* we lose -- invalidate the page */
	movl %cr3,%eax		/* and flush the TLB */
	movl %eax,%cr3

Lss_doit:
	movw 8(%esp),%ax
	movw %ax,(%edx)
	xorl %eax,%eax

Lss_exit:
	CLEAR_COPY_FAULT()
	ret

#if 0
/*
 * Copy a word to user space.
 *
 * int suword(void *, int), suiword(void *, int);
 */
ALTENTRY(suiword, _suword)
ENTRY(suword)
	/* not currently used */
#endif

/*
 * The following code is complicated on the 386 because
 * the MMU doesn't generate write faults in supervisor mode.
 * To get around the problem, we invalidate read-only user pages
 * before writing to them.
 *
 * int copyout(void *src, void *dst, size_t len);
 */
ENTRY(copyout)
	COPY_PROLOGUE()

	SET_COPY_FAULT(Lco_486_fault)
	VALIDATE_ADDRESS(%edi,Lco_486_fault)
	CHECK_ADDRESS_WRAP(%edi,%ecx,%eax,Lco_486_fault)
	decl %eax
	VALIDATE_ADDRESS(%eax,Lco_486_fault)

	cmpl $CPU_386,_cpu
	jle Lco_386

	/*
	 * On the 486, we assume that WP is set in CR0 and
	 * therefore we will correctly take write faults.
	 */
	COPY()
	xorl %eax,%eax
1:
	CLEAR_COPY_FAULT()
	COPY_EPILOGUE()

Lco_486_fault:
	movl $EFAULT,%eax
	jmp 1b

Lco_386:
	/*
	 * We copy by pages, to make write fault probing easier.
	 * ECX contains the count up to the next page boundary;
	 * EDX contains the total count.
	 * EBX contains the address of the PTE for this page.
	 */

	pushl %ebx

	SET_COPY_FAULT(Lco_fault)

	movl %ecx,%edx
	xorl %edi,%eax		/* EAX contains dest + count - 1 from above */
	testl $~(NBPG-1),%eax	/* do we finish on the same page? */
	je 1f			/* yes, count is correct */
	movl %edi,%eax		/* no, calculate distance to next page */
	andl $NBPG-1,%eax
	movl $NBPG,%ecx
	subl %eax,%ecx
1:
	subl %ecx,%edx

	movl %edi,%ebx		/* get address of relevant PTE */
	shrl $PGSHIFT,%ebx
	leal _PTmap(,%ebx,4),%ebx
 
Lco_loop:
	/*
	 * Check for a valid, read-only page.
	 * Note that we may fault on a page table page here.
	 */
	movb (%ebx),%al
	andb $PG_V|PG_PROT,%al
	cmpb $PG_V|PG_URKR,%al
	jne 1f
	andb $~PG_V,(%ebx)	/* we lose -- invalidate the page */
	movl %cr3,%eax		/* and flush the TLB */
	movl %eax,%cr3
1:

	COPY()

	testl %edx,%edx
	je Lco_break

	movl %edx,%ecx		/* get next count */
	cmpl $NBPG,%edx		/* use min(EDX,NBPG) */
	jb 1f
	movl $NBPG,%ecx
1:
	subl %ecx,%edx

	addl $4,%ebx		/* point at next PTE */

	jmp Lco_loop

Lco_break:
	xorl %eax,%eax
Lco_exit:
	CLEAR_COPY_FAULT()
	popl %ebx
	COPY_EPILOGUE()

Lco_fault:
	movl $EFAULT,%eax
	jmp Lco_exit

#if 0	/* not currently used */
/*
 * To copy out a string, we measure its length
 * and then call copyout().
 * XXX Are we really obligated to copy the data
 * XXX if there's insufficient space?
 */
ENTRY(copyoutstr)
	pushl %edi

	movl 12(%esp),%edi	/* destination */
	movl 16(%esp),%ecx	/* length */
	movl %ecx,%edx

	cld
	xorb %al,%al
	repne; scasb		/* find the end of the string */

	subl %ecx,%edx
	movl %ecx,%edi
	pushl %edx
	pushl 16(%esp)
	pushl 24(%esp)
	call _copyout		/* call copyout to do the dirty work */

	testl %eax,%eax
	jne Lcos_exit		/* there was an error */

	testl %edi,%edi
	je 2f
	movl $ENAMETOOLONG,%eax
	jmp Lcos_exit		/* not enough room for the data */
2:

	movl 24(%esp),%eax
	testl %eax,%eax
	je Lcos_exit
	movl %ecx,(%eax)	/* patch string length, if given */
	xorl %eax,%eax

Lcos_exit:
	popl %edi
	ret
#endif
