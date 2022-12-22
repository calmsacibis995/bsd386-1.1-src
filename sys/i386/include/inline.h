/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: inline.h,v 1.2 1994/01/05 20:41:47 karels Exp $
 */

/*
 * Functions to provide access to special i386 instructions
 * or inline implementations of common operations.
 */

#ifndef __INLINE_H
#define __INLINE_H

/* prototypes are here so that they precede any macro versions */
int	inb __P((u_int port));
void	outb __P((u_int port, u_char data));

#ifdef	__GNUC__

/*
 * The following pair of macros can be used instead of splmem (aka splimp)
 * and splx, in pairs, for sections that need to block memory allocation
 * for a very short time.
 */
#define	splmem_fast()	({ extern u_int	cpl; asm("cli"); cpl; })
#define	splxmem_fast(s)	asm("sti")

/*
 * Because spltty is heavily used during serial I/O when already at spltty(),
 * we provide a shortcut to eliminate work when we already have tty interrupts
 * blocked.
 */
#define	spltty() \
	({ extern u_int	cpl, ttymask; int _cpl = cpl; \
	(_cpl | ttymask) == _cpl ? _cpl : _spltty(); \
})

#define	inb(port) \
({ \
	register int _inb_result; \
\
	asm volatile ("xorl %%eax,%%eax; inb %%dx,%%al" : \
	    "=a" (_inb_result) : "d" (port)); \
	_inb_result; \
})

#define	inw(port) \
({ \
	register int _inb_result; \
\
	asm volatile ("xorl %%eax,%%eax; inw %%dx,%%ax" : \
	    "=a" (_inb_result) : "d" (port)); \
	_inb_result; \
})

#define	inl(port) \
({ \
	register u_long _inb_result; \
\
	asm volatile ("inl %%dx,%%ax" : "=a" (_inb_result) : "d" (port)); \
	_inb_result; \
})

#define	outb(port, data) \
	asm volatile ("outb %%al,%%dx" : : "a" (data), "d" (port))
#define outw(port, data) \
	asm volatile ("outw %%ax,%%dx" : : "a" (data), "d" (port))
#define outl(port, data) \
	asm volatile ("outl %%eax,%%dx" : : "a" (data), "d" (port))


#define	INLINE_NTOH	/* for endian.h */

#define	ntohs(s) \
(({ \
        register int rv; \
\
	asm volatile ("xchgb %%al,%%ah" : "=a" (rv) : "0" (s)); \
	rv; \
}) & 0xffff)

#define	htons(s) ntohs(s)

#define	ntohl(l) \
({ \
        register u_long rv; \
\
	asm volatile ("xchgb %%al,%%ah; roll $16,%%eax; xchgb %%al,%%ah" : \
	    "=a" (rv) : "0" (l) ); \
	rv; \
})

#define htonl(l)	ntohl(l)


#define	imin(a, b) \
({ \
	int _a = (a), _b = (b); \
\
	_a < _b ? _a : _b; \
})

#define	imax(a, b) \
({ \
	int _a = (a), _b = (b); \
\
	_a > _b ? _a : _b; \
})

#define	min(a, b) \
({ \
	unsigned int _a = (a), _b = (b); \
\
	_a < _b ? _a : _b; \
})

#define	max(a, b) \
({ \
	unsigned int _a = (a), _b = (b); \
\
	_a > _b ? _a : _b; \
})

#define	lmin(a, b) \
({ \
	long _a = (a), _b = (b); \
\
	_a < _b ? _a : _b; \
})

#define	lmax(a, b) \
({ \
	long _a = (a), _b = (b); \
\
	_a > _b ? _a : _b; \
})

#define	ulmin(a, b) \
({ \
	unsigned long _a = (a), _b = (b); \
\
	_a < _b ? _a : _b; \
})

#define	ulmax(a, b) \
({ \
	unsigned long _a = (a), _b = (b); \
\
	_a > _b ? _a : _b; \
})

#endif	/* !__GNUC__ */

#endif /* _INLINE_H */
