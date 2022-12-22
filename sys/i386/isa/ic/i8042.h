/*	BSDI $Id: i8042.h,v 1.2 1993/11/06 00:53:07 karels Exp $	*/
/* modified by BSDI as of the date in the line above */

/*
 * Copyright (c) Computer Newspaper Services Limited 1993
 * All rights reserved. 
 * 
 * License to use, copy, modify, and distribute this work and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that you also ensure modified files carry prominent notices
 * stating that you changed the files and the date of any change, ensure
 * that the above copyright notice appear in all copies, that both the
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Computer Newspaper Services not
 * be used in advertising or publicity pertaining to distribution or use
 * of the work without specific, written prior permission from Computer
 * Newspaper Services.
 * 
 * By copying, distributing or modifying this work (or any derived work)
 * you indicate your acceptance of this license and all its terms and
 * conditions.
 * 
 * COMPUTER NEWSPAPER SERVICES PROVIDE THIS SOFTWARE "AS IS", WITHOUT
 * ANY WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT
 * NOT LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 * A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.	THE
 * ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE,
 * INCLUDING ANY DUTY TO SUPPORT OR MAINTAIN, BELONGS TO THE LICENSEE.
 * SHOULD ANY PORTION OF THE SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT
 * COMPUTER NEWSPAPER SERVICES) ASSUMES THE ENTIRE COST OF ALL
 * SERVICING, REPAIR AND CORRECTION.	IN NO EVENT SHALL COMPUTER
 * NEWSPAPER SERVICES BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * $Id: i8042.h,v 1.2 1993/11/06 00:53:07 karels Exp $
 */

/*
 * Information Systems Engineering Group
 * Jan-Simon Pendry
 *
 * originally from Paul Borman <prb@cray.com>
 */

#define	KBDATA		0x60	/* kbd data port */
#define	KBSTAT		0x64	/* kbd controller status port (I) */
#define	  KBS_ORDY	0x01	/* Output Buffer ready */
#define	  KBS_IFULL	0x02	/* Input Buffer Full */
#define	  KBS_SYSTEM	0x04	/* System Flag */
#define	  KBS_OCMD	0x08	/* Output Buffer has Command */
#define	  KBS_NOINHIBIT	0x10	/* Password Inactive and KBD not inibited */
#define	  KBS_AUXDATA	0x20	/* Output Buffer is from Auxilliary Port */
#define	  KBS_TIMEOUT	0x40	/* General Time Out */
#define	  KBS_PARITY	0x80	/* Parity Error */

/*
 * Input Port Bit Definitions
 */
#define	KBI_KBD		0x01	/* Keyboard Data Input */
#define	KBI_AUX		0x02	/* Auxilliary Data Input */

/*
 * Output Port Bit Definitions
 */
#define	KBO_RESET	0x01	/* Reset Microprocessor */
#define	KBO_A20		0x02	/* Gate Address Line 20 */
#define	KBO_AUX		0x04	/* Auxilliary Data Out */
#define	KBO_AUXCLOCK	0x08	/* Auxilliary Clock Out */
#define	KBO_IRQ01	0x10	/* Keyboard Interrupt */
#define	KBO_IRQ12	0x20	/* Auxilliary Interrupt */
#define	KBO_KBDCLOCK	0x40	/* Keyboard Clock Out */
#define	KBO_KBD		0x80	/* Keyboard Data Out */

/*
 * Controller Command Byte
 */
#define	KBC_ENABLEKBDI	0x01	/* Enable Keyboard Interrupt */
#define	KBC_ENABLEAUXI	0x02	/* Enable Auxilliary Interrupt */
#define	KBC_SYSTEM	0x04	/* System Flag */
#define	KBC_INHIBDIS	0x08	/* keyboard inhib disabled; Reserved on PS/2 */
#define	KBC_DISABLEKBD	0x10	/* Disable Keyboard */
#define	KBC_DISABLEAUX	0x20	/* Disable Auxilliary */
#define	KBC_KBDXLATE	0x40	/* Keyboard Translate */
#define	KBC_RESERVED2	0x80	/* Reserved */

/*
 * The next two commands read or write locations in the 8042 memory,
 * with the location specified by the low 6 bits of the command code.
 * Location 0 is the "command byte".
 */
#define	KCMD_RCMD	0x20	/* Read command byte -- Low bits get address */
#define	KCMD_WCMD	0x60	/* Write command byte -- Low bits get address */
#define	KCMD_TESTPASSWD	0xa4	/* Test Password Installed */
#define	KCMD_LOADPASSWD	0xa5	/* Load Password */
#define	KCMD_ENABLEPASSWD 0xa6	/* Enable Password */
#define	KCMD_DISABLEAUX	0xa7	/* Disable Auxilliary Device */
#define	KCMD_ENABLEAUX	0xa8	/* Enable Auxilliary Device */
#define	KCMD_DISABLEKBD	0xad	/* Disable Keyboard Interface */
#define	KCMD_ENABLEKBD	0xae	/* Enable Keyboard Interface */
#define	KCMD_READINPUT	0xc0	/* Read Input Port */
#define	KCMD_POLLINLOW	0xc1	/* Poll Input Port Low */
#define	KCMD_POLLINHIGH	0xc2	/* Poll Input Port High */
#define	KCMD_READOUTPUT	0xd0	/* Read Output Port */
#define	KCMD_WRITEOUTPUT 0xd1	/* Write Output Port */
#define	KCMD_WRITEKBDBUF 0xd2	/* Write Keyboard Output Buffer */
#define	KCMD_WRITEAUXBUF 0xd3	/* Write Auxilliary Buffer */
#define	KCMD_WRITEAUX	0xd4	/* Write Auxilliary Device */
#define	  KAUX_ENABLE	0xf4	/*	Enable auxilliary device */
#define	  KAUX_DISABLE	0xf5	/*	Disable auxilliary device */
#define	  KAUX_RESET	0xff	/*	Reset auxilliary device */
#define	KCMD_READTEST	0xe0	/* Read Test Inputs */
#define	KCMD_PULSE	0xf0	/* Pulse Output Port -- Low Bits select bit */

#define	KCMD_TESTAUX	0xa9	/* Text aux device */
#define	KCMD_SETLED	0xed	/* Update leds (to data port) */
#define	KCMD_TYPEMATIC	0xf3	/* Set typematic */
#define	  KTM_1_4_SEC	0x00	/* 1/4 second delay */
#define	  KTM_1_2_SEC	0x20	/* 1/2 second delay */
#define	  KTM_1_SEC	0x60	/* 1 second delay */
#define	  KTM_2_0_RPS	0x1f	/* 2.0 Repetitions per second */
#define	  KTM_2_1_RPS	0x1e	/* 2.1 Repetitions per second */
#define	  KTM_2_3_RPS	0x1d	/* 2.3 Repetitions per second */
#define	  KTM_2_5_RPS	0x1c	/* 2.5 Repetitions per second */
#define	  KTM_2_7_RPS	0x1b	/* 2.7 Repetitions per second */
#define	  KTM_3_0_RPS	0x1a	/* 3.0 Repetitions per second */
#define	  KTM_3_3_RPS	0x19	/* 3.3 Repetitions per second */
#define	  KTM_3_7_RPS	0x18	/* 3.7 Repetitions per second */
#define	  KTM_4_0_RPS	0x17	/* 4.0 Repetitions per second */
#define	  KTM_4_3_RPS	0x16	/* 4.3 Repetitions per second */
#define	  KTM_4_6_RPS	0x15	/* 4.6 Repetitions per second */
#define	  KTM_5_0_RPS	0x14	/* 5.0 Repetitions per second */
#define	  KTM_5_5_RPS	0x13	/* 5.5 Repetitions per second */
#define	  KTM_6_0_RPS	0x12	/* 6.0 Repetitions per second */
#define	  KTM_6_7_RPS	0x11	/* 6.7 Repetitions per second */
#define	  KTM_7_5_RPS	0x10	/* 7.5 Repetitions per second */
#define	  KTM_8_0_RPS	0x0f	/* 8.0 Repetitions per second */
#define	  KTM_8_6_RPS	0x0e	/* 8.6 Repetitions per second */
#define	  KTM_9_2_RPS	0x0d	/* 9.2 Repetitions per second */
#define	  KTM_10_0_RPS	0x0c	/* 10.0 Repetitions per second */
#define	  KTM_10_9_RPS	0x0b	/* 10.9 Repetitions per second */
#define	  KTM_12_0_RPS	0x0a	/* 12.0 Repetitions per second */
#define	  KTM_13_3_RPS	0x09	/* 13.3 Repetitions per second */
#define	  KTM_15_0_RPS	0x08	/* 15.0 Repetitions per second */
#define	  KTM_16_0_RPS	0x07	/* 16.0 Repetitions per second */
#define	  KTM_17_1_RPS	0x06	/* 17.1 Repetitions per second */
#define	  KTM_18_5_RPS	0x05	/* 18.5 Repetitions per second */
#define	  KTM_20_0_RPS	0x04	/* 20.0 Repetitions per second */
#define	  KTM_21_8_RPS	0x03	/* 21.8 Repetitions per second */
#define	  KTM_24_0_RPS	0x02	/* 24.0 Repetitions per second */
#define	  KTM_26_7_RPS	0x01	/* 26.7 Repetitions per second */
#define	  KTM_30_0_RPS	0x00	/* 30.0 Repetitions per second */
#define	KCMD_RESEND	0xfe	/* Resend pervious output (to status port) */
#define	KCMD_RESET	0xff	/* Reset keyboard controller (to data port) */

#define	KBD_RESETOK	0xaa	/* Keyboard reset-OK code */
#define	KBD_ACK		0xfa	/* Keyboard acknowledge code */
