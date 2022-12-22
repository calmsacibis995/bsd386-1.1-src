/*
 * Copyright (c) 1990, 1991, 1992 Regents of the University of California.
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
 * @(#) $Header: /bsdi/MASTER/BSDI_OS/usr.bin/gdb/gdb/config/remote-sl.h,v 1.1.1.1 1992/08/27 17:03:46 trent Exp $ (LBL)
 */

/*
 * These definitions are factored out into an include file so
 * the kernel stub has access to them.
 */
#define FRAME_START		0xc1		/* Frame End */
#define FRAME_END		0xc0		/* Frame End */
#define FRAME_ESCAPE		0xdb		/* Frame Esc */
#define TRANS_FRAME_START	0xde		/* transposed frame start */
#define TRANS_FRAME_END		0xdc		/* transposed frame esc */
#define TRANS_FRAME_ESCAPE	0xdd		/* transposed frame esc */

/*
 * Message limits.  SL_MAXDATA is the maximum number of bytes that can
 * be read or written.  SL_RPCSIZE is the maximum message size for
 * the serial link.  The actual MTU is two times the max message (since
 * each byte might be escaped), plus the two framing bytes.  We add two 
 * to the message length to account for the type byte and checksum.
 */
#define SL_MAXDATA 62			/* max data that can be read */
#define SL_RPCSIZE (1 + SL_MAXDATA)	/* errno byte + data */
#define SL_MTU ((2 * (SL_RPCSIZE + 2) + 2))
