/* BSDI $Id: init.c,v 1.4 1994/01/13 23:09:50 cp Exp $ */

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

#include "sort.h"

extern struct coldesc clist[(ND+1)*2];
extern int ncols;
u_char gweights[NBINS];

/* 
 * clist (list of columns which correspond to one or more icol or tcol)
 * is in increasing order of columns.
 * Fields are kept in increasing order of fields.
 */

/* 
 * keep clist in order--inserts a column in a sorted array 
 */
insertcol(field)
	struct field *field;
{
	int i;

	for (i = 0; i < ncols; i++)
		if (field->icol.num <= clist[i].num) 
			break;
	if (field->icol.num != clist[i].num) {
		memmove(clist+i+1, clist+i, sizeof(COLDESC)*(ncols-i));
		clist[i].num = field->icol.num;
		ncols++;
	}
	if (field->tcol.num && field->tcol.num != field->icol.num) {
		for (i = 0; i < ncols; i++)
			if (field->tcol.num <= clist[i].num) 
				break;
		if (field->tcol.num != clist[i].num) {
			memmove(clist+i+1, clist+i,sizeof(COLDESC)*(ncols-i));
			clist[i].num = field->tcol.num;
			ncols++;
		}
	}
}

/* 
 * matches fields with the appropriate columns--n^2 but who cares?
 */
fldreset(fldtab)
	struct field *fldtab;
{
	int i;

	fldtab[0].tcol.p = clist+ncols-1;
	for (++fldtab; fldtab->icol.num; ++fldtab) {
		for (i = 0; fldtab->icol.num != clist[i].num; i++);
		fldtab->icol.p = clist + i;
		if (!fldtab->tcol.num)
			continue;
		for (i = 0; fldtab->tcol.num != clist[i].num; i++);
		fldtab->tcol.p = clist + i;
	}
}

/* 
 * interprets a column in a -k field 
 */
char *
setcolumn(pos, cur_fld, gflag)
	char *pos;
	struct field *cur_fld;
	int gflag;
{
	struct column *col;
	int tmp;

	col = cur_fld->icol.num ? (&(*cur_fld).tcol) : (&(*cur_fld).icol);
	pos += sscanf(pos, "%d", &(col->num));
	while (isdigit(*pos)) pos++;
	if (col->num <= 0 && !(col->num == 0 && col == &(cur_fld->tcol)))
		err("field numbers must be positive");
	if (*pos == '.') {
		if (!col->num) 
			err("cannot indent end of line");
		pos += sscanf(++pos, "%d", &(col->indent));
		while (isdigit(*pos)) pos++;
		if (&cur_fld->icol == col)
			col->indent--;
		if (0 > col->indent)
			err("illegal offset");
	}
	if (optval(*pos, cur_fld->tcol.num))	
		while (tmp = optval(*pos, cur_fld->tcol.num)) {
			cur_fld->flags |= tmp;
			pos++;
	}
	if (cur_fld->icol.num == 0) 
		cur_fld->icol.num = 1;

	return(pos);
}

setfield(pos, cur_fld, gflag)
	char *pos;
	struct field *cur_fld;
	int gflag;
{
	static int nfields = 0;
	int tmp;
	char *setcolumn();

	if (++nfields == ND)
		err("too many sort keys. (Limit is %d)", ND-1);
	cur_fld->weights = ascii;
	cur_fld->mask = alltable;
	pos = setcolumn(pos, cur_fld, gflag);
	if (*pos == '\0')			/* key extends to EOL. */
		cur_fld->tcol.num = 0;
	else {
		if (*pos != ',') err("illegal field descriptor");
			setcolumn((++pos), cur_fld, gflag);
	}
	cur_fld->flags |= gflag;
	tmp = cur_fld->flags;

	/* 
	 * Assign appropriate mask table and weight table.
 	 * If the global weights are reversed, the local field must be 
	 * "re-reversed". 
	 */
	if (((tmp & R) ^ (gflag & R)) && tmp & F)
		cur_fld->weights = RFtable;
	else if (tmp & F) 
		cur_fld->weights = Ftable;
	else if ((tmp & R) ^ (gflag & R)) 
		cur_fld->weights = Rascii;
	if (tmp & I) 
		cur_fld->mask = itable;
	else if (tmp & D) 
		cur_fld->mask = dtable;
	cur_fld->flags |= (gflag & (BI | BT));
	if (!cur_fld->tcol.indent) 
		cur_fld->flags &= (D|F|I|N|R|BI);
					/* BT has no meaning at end of field */
	if (cur_fld->tcol.num && !(!(cur_fld->flags & BI) 
	    && cur_fld->flags & BT) && (cur_fld->tcol.num 
	    <= cur_fld->icol.num && cur_fld->tcol.indent 
	    < cur_fld->icol.indent))
			err("fields out of order");
	insertcol(cur_fld);
	return(cur_fld->tcol.num);

}

optval(desc, tcolflag)
	char desc;
	int tcolflag;
{
	switch(desc) {
		case 'b': 
			if (!tcolflag) 
				return(BI);
			else 
				return(BT);
		case 'd': 
			return(D);
		case 'f': 
			return(F);
		case 'i': 
			return(I);
		case 'n': 
			return(N);
		case 'r': 
			return(R);
		default:  
			return(0);
	}
}


fixit(argc, argv)
	int *argc;
	char **argv;
{
	int i, j, v, w, x;
	static char vbuf[ND*20], *vpos, *tpos;

	vpos = vbuf;
	for (i = 1; i < *argc; i++) {
		if (argv[i][0] == '+') {
			tpos = argv[i]+1;
			argv[i] = vpos;
			vpos += sprintf(vpos, "-k");
			tpos += sscanf(tpos, "%d", &v);
			while (isdigit(*tpos)) tpos++;
			vpos += sprintf(vpos, "%d", v+1);
			if (*tpos == '.') {
				tpos += sscanf(++tpos, "%d", &x);
				vpos += sprintf(vpos, ".%d", x+1);
			}
			while (*tpos) *vpos++ = *tpos++;
			vpos += sprintf(vpos, ",");
			if (argv[i+1] &&
			    argv[i+1][0] == '-' && isdigit(argv[i+1][1])) {
				tpos = argv[i+1] + 1;
				tpos += sscanf(tpos, "%d", &w);
				while (isdigit(*tpos)) tpos++;
				x = 0;
				if (*tpos == '.') {
					tpos += sscanf(++tpos, "%d", &x);
					while (isdigit(*tpos)) *tpos++;
				}
				if (x) {
					vpos += sprintf(vpos, "%d", w+1);
					vpos += sprintf(vpos, ".%d", x);
				}
				else vpos += sprintf(vpos, "%d", w);
				while (*tpos) *vpos++ = *tpos++;
				for (j= i+1; j < *argc; j++)
					argv[j] = argv[j+1];
				*argc -= 1;
			}
		}
	}
}

/* ascii, Rascii, Ftable, and RFtable map
 * REC_D -> REC_D;  {not REC_D} -> {not REC_D}.
 * gweights maps REC_D -> (0 or 255); {not REC_D} -> {not gweights[REC_D]}.
 * Note: when sorting in forward order, to encode character zero in a key,
 * use \001\001; character 1 becomes \001\002.  In this case, character 0
 * is reserved for the field delimiter.  Analagously for -r (fld_d = 255).
 * Note: this is only good for ASCII sorting.  For different LC 's,
 * all bets are off.  See also num_init in number.c
 */
settables(gflags)
	int gflags;
{
	u_char *wts;
	int i, incr;

	for (i=0; i < 256; i++) {
		ascii[i] = i;
		if (i > REC_D && i < 255 - REC_D+1)
			Rascii[i] = 255 - i + 1;
		else Rascii[i] = 255 - i;
		if (i >= 'a' && i <= 'z') {
			Ftable[i] = Ftable[i- ('a' -'A')];
			RFtable[i] = RFtable[i - ('a' - 'A')];
		}
		else if (REC_D >= 'A' && REC_D < 'Z' && i < 'a' && i > REC_D) {
			Ftable[i] = i + 1;
			RFtable[i] = Rascii[i] - 1;
		}
		else {
			Ftable[i] = i;
			RFtable[i] = Rascii[i];
		}
		alltable[i] = 1;
		if (i == '\n' || (i >= 32 && i <= 126))
			itable[i] = 1;
		else 
			itable[i] = 0;
		if (i == '\n' || i == '\t' || i== ' ' || (i >='0' && i <= '9') ||
		   (i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z'))
			dtable[i] = 1;
		else 
			dtable[i] = 0;
	}

	Rascii[REC_D] = RFtable[REC_D] = REC_D;
	if (REC_D >= 'A' && REC_D < 'Z') 
		++Ftable[REC_D + ('a' - 'A')];
	if (gflags & R && (!(gflags & F) || !SINGL_FLD)) 
		wts = Rascii;
	else if (!(gflags & F) || !SINGL_FLD) 
		wts = ascii;
	else if (gflags & R) 
		wts = RFtable;
	else  
		wts = Ftable;
	bcopy(wts, gweights, sizeof(gweights));
	incr = (gflags & R) ? -1 : 1;
	for (i = 0; i < REC_D; i++)
		gweights[i] += incr;
	gweights[REC_D] = ((gflags & R) ? 255 : 0);
	if (SINGL_FLD && gflags & F) {
		for (i = 0; i < REC_D; i++) {
			ascii[i] += incr;
			Rascii[i] += incr;
		}
		ascii[REC_D] = Rascii[REC_D] = gweights[REC_D];
	}
}
