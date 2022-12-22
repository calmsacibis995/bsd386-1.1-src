/* Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 * 
 * Derived from software contributed to Berkeley by Peter McIlroy.
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

/* BSDI $Id: extern.h,v 1.2 1992/02/15 04:21:00 trent Exp $ */

#include <sys/cdefs.h>

FILE *ftmp __P((void));

length_t enterkey __P((struct recheader *, DBT *, int, struct field *));

void rd_append __P((int, union f_handle, int, FILE *, u_char *, u_char *));

void append  __P((u_char **, int, int, FILE *, void (*)(), struct field *));

void fsort __P((int, int, union f_handle, int, FILE *, struct field *));

void fmerge __P((int, union f_handle, int, int (*)(), FILE *,
						 void (*)(), struct field *));

void merge __P((int, int, int (*)(), FILE *, void (*)(), struct field *));

int getnext __P((int, union f_handle, int, struct recheader *,
						u_char *, struct field *));
int geteasy __P((int, union f_handle, int, struct recheader *,
						u_char *, struct field *));
int makeline __P((int, union f_handle, int, struct recheader *,
						u_char *, struct field *));

int makekey __P((int, union f_handle, int, struct recheader *,
						u_char *, struct field *));
void putrec __P((struct recheader *, FILE *));

void putline __P((struct recheader *, FILE *));

void onepass __P((u_char **, int, long, long *, u_char *, FILE *));

void *malloc __P((int));

