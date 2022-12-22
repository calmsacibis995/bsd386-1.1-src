/*	BSDI $Id: in_cksum.c,v 1.2 1993/11/05 17:40:57 karels Exp $	*/

/*
 *	in_cksum.c by Steve Nuchia with help from Bakul Shah.
 *	Based in part on the "portable" in_cksum from the Tahoe
 *	release and owing a strong debt to the authors of RFC1071.
 *
 *	Experimental!
 *
 *	Released without restrictions, 1993  swn@sccsi.com
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 * from tahoe:	in_cksum.c	1.2	86/01/05
 *	@(#)in_cksum.c	1.3 (Berkeley) 1/19/91
 */

#include "param.h"
#include "sys/mbuf.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"

/*
 * Checksum routine for Internet Protocol family headers.
 * External entry point is based on mbufs.
 */

#define CLC     asm("clc")
#define ADD(n)  asm("adcl " #n "(%2), %0": "=r"(sum): "0"(sum), "r"(w))
#define MOP     asm("adcl $0, %0":         "=r"(sum): "0"(sum))

inline u_long
#ifdef CKSUMDEBUG
in_cksumbuf2(w, len)
#else
in_cksumbuf(w, len)
#endif
	register void *w;
	register int len;
{
	register u_long sum;
	int jog;

	if (((u_long) w & 1) && len >= 1) {
		sum = (*(u_char *)w) << 8;
		w += 1;
		jog = 1;
		len--;
	} else {
		sum = 0;
		jog = 0;
	}

	if (((u_long) w & 2) && len >= 2) {
		sum += *(u_short *)w;
		w += 2;
		len -= 2;
	}

	while (len >= 128) {
		CLC;
		ADD(0);   ADD(4);   ADD(8);   ADD(12);
		ADD(16);  ADD(20);  ADD(24);  ADD(28);
		ADD(32);  ADD(36);  ADD(40);  ADD(44);
		ADD(48);  ADD(52);  ADD(56);  ADD(60);
		ADD(64);  ADD(68);  ADD(72);  ADD(76);
		ADD(80);  ADD(84);  ADD(88);  ADD(92);
		ADD(96);  ADD(100); ADD(104); ADD(108);
		ADD(112); ADD(116); ADD(120); ADD(124);
		MOP;
		MOP;
		w += 128;
		len -= 128;
	}
	while (len >= 16) {
		CLC;
		ADD(0);  ADD(4);  ADD(8);  ADD(12);
		MOP;
		MOP;
		w += 16;
		len -= 16;
	}
	sum = ((u_short) sum) + (sum >> 16);

	while (len >= 2) {
		sum += *(u_short *)w;
		w += 2;
		len -= 2;
	}
	if (len >= 1)
		sum += *(u_char *) w;
	if (jog)
		sum <<= 8;

	do
		sum = ((u_short) sum) + (sum >> 16);
	while (sum >> 16);

	return (sum);
}

in_cksum(m, len)
	struct mbuf *m;
	int len;
{
	u_long sum = 0; /* accumulated across mbufs */
	register u_long msum; /* accumulated within mbuf */
	register u_short *w;
	register int mlen = 0;
	int odd = 0, jog = 0; /* keep track of where we are */

#ifdef CKSUMDEBUG
	int ocksum = in_cksum_c(m, len);
	int olen = len, osum;
	static int recur;
	struct mbuf *om = m;
	int s = splimp();

	recur++;
#endif

	for ( ; m && len; m = m->m_next) {
		if ((mlen = min(m->m_len, len)) == 0)
			continue;
		len -= mlen;
		w = mtod(m, u_short *);

		msum = in_cksumbuf(w, mlen);

		msum = ((u_short) msum) + (msum >> 16);
		if (odd)
			msum <<= 8;

		sum += msum;
		odd ^= mlen & 1;
	}
	if (len)
		printf("cksum: out of data\n");

#ifdef CKSUMDEBUG
	osum = sum;
#endif

	do
		sum = ((u_short) sum) + (sum >> 16);
	while (sum >> 16);

#ifdef CKSUMDEBUG
	sum ^= 0xffff;
	if (sum != ocksum) {
	    printf ( "CK: len = %d, o = %04x, n = %04x\n", olen,
				    ~ocksum & 0xffff, ~sum & 0xffff );
	    printf ( "CK: unadjusted sum was %06x\n", osum );
	    for ( m = om; olen && m; m = m->m_next )
	    {
		odd = min(m->m_len, olen);
		olen -= odd;
		if ( recur < 3 )
		    printf ( "CK mbuf at %x len %d, o = %04x, n = %04x\n",
			    m->m_data, odd,
			    ~in_cksum_c(m, odd) & 0xffff,
			    ~in_cksum(m, odd, "recursive1") & 0xffff );

		if ( recur < 2 && olen && m->m_next )
		{
		    odd += min(olen, m->m_next->m_len);
		    in_cksum ( m, odd, "recursive2" );  /* will report the problem */
		}
	    }
	}
	recur--;
	splx(s);
	return ( ocksum );
#endif

	return (sum ^ 0xffff);
}


#ifdef CKSUMDEBUG
/*
 * (old) Checksum routine for Internet Protocol family headers.
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 * 
 * This implementation is 386 version.
 */

#undef	ADDCARRY
#define ADDCARRY(sum)  {				\
			if (sum & 0xffff0000) {		\
				sum &= 0xffff;		\
				sum++;			\
			}				\
		}
in_cksum_c(m, len)
	register struct mbuf *m;
	register int len;
{
	union word {
		char	c[2];
		u_short	s;
	} u;
	register u_short *w;
	register int sum = 0;
	register int mlen = 0;

	for (;m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w = mtod(m, u_short *);
		if (mlen == -1) {
			/*
			 * The first byte of this mbuf is the continuation
			 * of a word spanning between this mbuf and the
			 * last mbuf.
			 */

			/* u.c[0] is already saved when scanning previous 
			 * mbuf.
			 */
			u.c[1] = *(u_char *)w;
			sum += u.s;
			ADDCARRY(sum);
			w = (u_short *)((char *)w + 1);
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;

		if (len < mlen)
			mlen = len;
		len -= mlen;

		/*
		 * add by words.
		 */
		while ((mlen -= 2) >= 0) {
			if ((int)w & 0x1) {
				/* word is not aligned */
				u.c[0] = *(char *)w;
				u.c[1] = *((char *)w+1);
				sum += u.s;
				w++;
			} else
				sum += *w++;
			ADDCARRY(sum);
		}
		if (mlen == -1)
			/*
			 * This mbuf has odd number of bytes. 
			 * There could be a word split betwen
			 * this mbuf and the next mbuf.
			 * Save the last byte (to prepend to next mbuf).
			 */
			u.c[0] = *(u_char *)w;
	}
	if (len)
		printf("cksum: out of data\n");
	if (mlen == -1) {
		/* The last mbuf has odd # of bytes. Follow the
		   standard (the odd byte is shifted left by 8 bits) */
		u.c[1] = 0;
		sum += u.s;
		ADDCARRY(sum);
	}
	return (~sum & 0xffff);
}


in_cksumbuf(w, len)
	register void *w;
	register int len;
{
	struct mbuf mb;
	int a, b;

    mb.m_next = 0;
    mb.m_data = w;
    mb.m_len = len;
    a = in_cksum_c(&mb, len);
    b = in_cksumbuf2 ( w, len ) ^ 0xffff;
    if ( a != b )
	printf ( "cksumbuf [%d] o = %04x, n = %04x\n", len, a, b );
    return ( a );
}
#endif
