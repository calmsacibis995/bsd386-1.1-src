/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/tcpdump/grot/bpf/net/bpf_compat.h,v 1.1.1.1 1993/03/08 17:46:20 polk Exp $ (LBL)
 */

/*
 * Some hacks for compatibility across SunOS and 4.4BSD.  We emulate
 * malloc and free with mbuf clusters.  We store a pointer to
 * the mbuf in the first word of the mbuf and return 8 bytes
 * passed the start of data (for double word alignment).
 * We cannot just use offsets because clusters are not at a fixed
 * offset from the associated mbuf.  Sorry for this kludge.
 */
#define malloc(size, type, canwait) bpf_alloc(size, canwait)
#define free(cp, type) m_free(*(struct mbuf **)(cp - 8))
#define M_WAITOK M_WAIT
/* This mapping works for our purposes. */
#define ERESTART EINTR
