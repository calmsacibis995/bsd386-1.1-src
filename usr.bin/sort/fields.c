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

/* BSDI $Id: fields.c,v 1.2 1992/02/15 04:21:03 trent Exp $ */

#include "sort.h"

#define blancmange(ptr) {					\
	if (BLANK & d_mask[*(ptr)])				\
		while (BLANK & d_mask[*(++(ptr))]);		\
}

#define NEXTCOL(pos) {						\
	if (!SEP_FLAG)						\
		while (BLANK & l_d_mask[*(++pos)]);		\
	while (!((FLD_D | REC_D_F) & l_d_mask[*++pos]));	\
}
		
extern u_char *enterfield __P((u_char *, u_char *, struct field *, int));

extern u_char *number __P((u_char *, u_char *, u_char *, u_char *, int));

extern struct coldesc clist[(ND+1)*2];
extern int ncols;

#define DECIMAL '.'
#define OFFSET 128

u_char TENS[10];	/* TENS[0] = REC_D <= 128 ? 130 - '0' : 2 -'0'... */
u_char NEGTENS[10];	/* NEGTENS[0] = REC_D <= 128 ? 126 + '0' : 252 +'0' */
u_char *OFF_TENS, *OFF_NTENS;	/* TENS - '0', NEGTENS - '0' */
u_char fnum[NBINS], rnum[NBINS];

/* 
 * constructs sort key with leading recheader, followed by the key,
 * followed by the original line.
 */
length_t 
enterkey(keybuf, line, size, fieldtable)
	struct recheader *keybuf;	/* pointer to start of key */
	DBT *line;
	int size;
	struct field fieldtable[];
{
	int i;
	register u_char *l_d_mask;
	register u_char *lineend, *pos;
	u_char *endkey, *keypos;
	register struct coldesc *clpos;
	register int col = 1;
	struct field *ftpos;

	l_d_mask = d_mask;
	pos = (u_char *) line->data - 1;
	lineend = (u_char *) line->data + line->size-1;
				/* don't include rec_delimiter */
	keypos = keybuf->data;

	for (i = 0; i < ncols; i++) {
		clpos = clist+i;
		for (; (col < clpos->num) && (pos < lineend); col++)
			{ NEXTCOL(pos); }
		if (pos >= lineend) 
			break;
		clpos->start = SEP_FLAG ? pos+1 : pos;
		NEXTCOL(pos);
		clpos->end = pos;
		col++;
		if (pos >= lineend) {
			clpos->end = lineend;
			++i;
			break;
		}
	}
	for (; i <= ncols; i++)
		clist[i].start = clist[i].end = lineend;
	if (clist[0].start < (u_char *) line->data)
		++clist[0].start;
	endkey = (u_char *) keybuf + size - line->size;
	for (ftpos = fieldtable + 1; ftpos->icol.num; ftpos++)
		if (!(keypos = enterfield(keypos, endkey, ftpos,
		    fieldtable->flags))) 
			return(1);

	if (UNIQUE) 
		*(keypos-1) = REC_D;
	keybuf->offset = keypos - keybuf->data;
	keybuf->length = keybuf->offset + line->size;
	if (keybuf->length + sizeof(TRECHEADER) > size)
		return(1);		/* line too long for buffer */
	memcpy(keybuf->data + keybuf->offset, line->data, line->size);
	return(0);
}

u_char *
enterfield(tablepos, endkey, cur_fld, gflags)
	struct field *cur_fld;
	register u_char *tablepos, *endkey;
{
	register u_char *start, *end, *lineend, *mask, *lweight;
	struct column icol, tcol;
	register u_int flags;
	u_int Rflag;

	icol = cur_fld->icol;
	tcol = cur_fld->tcol;
	flags = cur_fld->flags;
	start = icol.p->start;
	lineend = clist[ncols].end;
	if (flags & BI) 
		blancmange(start);
	start += icol.indent;
	start = min(start, lineend);
	if (!tcol.num) 
		end = lineend;
	else {
		if (tcol.indent) {
			end = tcol.p->start;
			if (flags & BT) blancmange(end);
			end += tcol.indent;
			end = min(end, lineend);
		}
		else 
			end = tcol.p->end;
	}
	if (flags & N) {
		Rflag = (gflags & R ) ^ (flags & R) ? 1 : 0;
		tablepos = number(tablepos, endkey, start, end, Rflag);
		return(tablepos);
	}
	mask = alltable;
	mask = cur_fld->mask;
	lweight = cur_fld->weights;	
	for (; start < end; start++)
	if (mask[*start]) {
		if (*start <= 1) {
			if (tablepos+2 >= endkey) return(0);
			*tablepos++ = lweight[1];
			*tablepos++ = lweight[*start ? 2 : 1];
		}
		else {
			*tablepos++ = lweight[*start];
			if (tablepos == endkey)
				return(0);
		}
	}
	*tablepos++ = lweight[0];
	return(tablepos == endkey ? 0 : tablepos);
}

/* 
 * Uses the first bin to assign sign, expsign, 0, and the first
 * 61 out of the exponent ( (254 - 3 origins - 4 over/underflows)/4 = 61 ).
 *   When sorting in forward order:
 * use (0-99) -> (130->240) for sorting the mantissa if REC_D <=128;
 * else use (0-99)->(2-102).
 * If the exponent is >=61, use another byte for each additional 253
 * in the exponent. Cutoff is at 567.
 * To avoid confusing the exponent and the mantissa, use a field delimiter
 * if the exponent is exactly 61, 61+252, etc--this is ok, since it's the
 * only time a field delimiter can come in that position.
 * Reverse order is done analagously.
*/

u_char *
number(pos, bufend, line, lineend, Rflag)
	register u_char *line, *pos, *bufend, *lineend;
	int Rflag;
{
	register int or_sign, parity = 0;
	register int exponent = -1, expincr = 1;
	int sign = 1, expsign = 1, bite;
	register u_char lastvalue, *nonzero, *tline, *C_TENS;
	u_char *nweights;

	if (Rflag) 
		nweights = rnum;
	else 
		nweights = fnum;
	if (pos > bufend - 8)
		return(0);

	/*   or_sign sets the sort direction:
	 *	(-r: +/-)(sign: +/-)(expsign: +/-) 
 	 */
	or_sign = sign ^ expsign ^ Rflag;
	blancmange(line);
	if (*line == '-') {	/* set the sign */
		or_sign ^= 1;
		sign = 0;
		line++;
	}
	/* eat initial zeroes */
	for (; *line == '0' && line < lineend; line++);
	/* calculate exponents < 0 */
	if (*line == DECIMAL) {
		exponent = 1;
		while (*++line == '0' && line < lineend)
			exponent++;
		expincr = 0;
		expsign = 0;
	}
	/* next character better be a digit */
	if (*line < '1' || *line > '9' || line >= lineend) {
		*pos++ = nweights[127];
		return(pos);
	}
	if (expincr) {
		for (tline = line-1; *++tline >= '0' && 
		    *tline <= '9' && tline < lineend;)
			exponent++;
	}
	if (exponent > 567) {
		*pos++ = nweights[sign ? (expsign ? 254 : 128)
					: (expsign ? 0 : 126)];
		fprintf(stderr, "warning: exponent out of bounds\n");
		return(pos);
	}
	bite = min(exponent, 61);
	*pos++ = nweights[(sign) ? (expsign ? 189+bite : 189-bite)
				: (expsign ? 64-bite : 64+bite)];
	if (bite >= 61) {
		do {
			exponent -= bite;
			bite = min(exponent, 254);
			*pos++ = nweights[or_sign ? 254-bite : bite];
		} while (bite == 254);
	}
	C_TENS = or_sign ? OFF_NTENS : OFF_TENS;
	for (; line < lineend; line++) {
		if (*line >= '0' && *line <= '9') {
			if (parity) {
				*pos++ = C_TENS[lastvalue] + (or_sign ? - *line
						: *line);
				if (pos == bufend)
					return(0);
				if (*line != '0' || lastvalue != '0')
					nonzero = pos;	
			}
			else lastvalue = *line;
			parity ^= 1;
		}
		else if (*line == DECIMAL) {
			if (!expincr)	/* a decimal already occurred once */
				break;
			expincr = 0;
		}
		else break;
	}
	if (parity && lastvalue != '0') {
		*pos++ = or_sign ? OFF_NTENS[lastvalue] - '0' :
					OFF_TENS[lastvalue] + '0';
	}
	else pos = nonzero;	
	if (pos > bufend-1) 
		return(0);
	*pos++ = or_sign ? nweights[254] : nweights[0];

	return(pos);
}

/* This forces a gap around the record delimiter
 * Thus fnum has vaues over (0,254) -> ((0,REC_D-1),(REC_D+1,255));
 * rnum over (0,254) -> (255,REC_D+1),(REC_D-1,0))
*/
num_init()
{
	int i;

	TENS[0] = REC_D <=128 ? 130 - '0' : 2 - '0';
	NEGTENS[0] = REC_D <=128 ? 126 + '0' : 254 + '0';
	OFF_TENS = TENS - '0';
	OFF_NTENS = NEGTENS - '0';
	for (i = 1; i < 10; i++) {
		TENS[i] = TENS[i-1] + 10;
		NEGTENS[i] = NEGTENS[i-1] - 10;
	}
	for (i = 0; i < REC_D; i++) {
		fnum[i] = i;
		rnum[255-i] = i;
	}
	for (i = REC_D; i <255; i++) {
		fnum[i] = i+1;
		rnum[255-i] = i-1;
	}
}
