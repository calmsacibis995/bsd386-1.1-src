/* $Header: /bsdi/MASTER/BSDI_OS/contrib/tcsh/config_f.h,v 1.2 1994/01/13 21:59:27 polk Exp $ */
/*
 * config_f.h -- configure various defines for tcsh
 *
 * This is included by config.h.
 *
 * Edit this to match your particular feelings; this is set up to the
 * way I like it.
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
 */
#ifndef _h_config_f
#define _h_config_f

/*
 * SHORT_STRINGS Use 16 bit characters instead of 8 bit chars
 * 	         This fixes up quoting problems and eases implementation
 *	         of nls...
 *
 */
#define SHORT_STRINGS

/*
 * NLS:		Use Native Language System
 *		Routines like setlocale() are needed
 *		if you don't have <locale.h>, you don't want
 *		to define this.
 */
#define NLS

/*
 * LOGINFIRST   Source ~/.login before ~/.cshrc
 */
#undef LOGINFIRST

/*
 * VIDEFAULT    Make the VI mode editor the default
 */
#undef VIDEFAULT

/*
 * KAI          use "bye" command and rename "log" to "watchlog"
 */
#undef KAI

/*
 * TESLA	drops DTR on logout. Historical note:
 *		tesla.ee.cornell.edu was a vax11/780 with a develcon
 *		switch that sometimes would not hang up.
 */
#undef TESLA

/*
 * DOTLAST      put "." last in the default path, for security reasons
 */
#define DOTLAST

/*
 * AUTOLOGOUT	tries to determine if it should set autologout depending
 *		on the name of the tty, and environment.
 *		Does not make sense in the modern window systems!
 */
#define AUTOLOGOUT

/*
 * SUSPENDED	Newer shells say 'Suspended' instead of 'Stopped'.
 *		Define to get the same type of messages.
 */
#define SUSPENDED

/*
 * KANJI	Ignore meta-next, and the ISO character set. Should
 *		be used with SHORT_STRINGS
 *
 */
#undef KANJI

/*
 * SYSMALLOC	Use the system provided version of malloc and friends.
 *		This can be much slower and no memory statistics will be
 *		provided.
 */
#if defined(PURIFY) || defined(MALLOC_TRACE)
# define SYSMALLOC
#else
# undef SYSMALLOC
#endif

/*
 * RCSID	This defines if we want rcs strings in the binary or not
 *
 */
#if !defined(lint) && !defined(SABER)
# ifndef __GNUC__
#  define RCSID(id) static char *rcsid = (id);
# else
#  define RCSID(id) static char *rcsid() { return rcsid(id); }
# endif /* !__GNUC__ */
#else
# define RCSID(id)	/* Nothing */
#endif /* !lint && !SABER */

#endif /* _h_config_f */
