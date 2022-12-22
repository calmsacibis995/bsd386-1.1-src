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

/* BSDI $Id: fsort.c,v 1.2 1992/02/15 04:21:09 trent Exp $ */

/* 
 * Read in the next bin.  If it fits in one segment sort it;
 * otherwise refine it by segment deeper by one character,
 * and try again on smaller bins.  Sort the final bin at this level
 * of recursion to keep the head of fstack at 0.
 */

#include "sort.h"
#include "fsort.h"

#define FSORTMAX 5

u_char **keylist = 0, *buffer = 0, *linebuf = 0;
struct tempfile fstack[MAXFCT];
extern char *toutpath;

int PANIC = FSORTMAX;

void 
fsort(binno, depth, infiles, nfiles, outfd, ftbl)
	register int binno, depth;
	register union f_handle infiles;
	register int nfiles;
	FILE *outfd;
	register struct field *ftbl;
{
	register u_char *bufend, **keypos;
	u_char *weights;

	int ntfiles, mfct = 0, total, i, maxb, lastb, panic = 0;
	register int c, nelem;
	long sizes [NBINS+1];
	union f_handle tfiles, mstart = {MAXFCT-16};
	register int (*get)(int, union f_handle, int, RECHEADER *,
		u_char *, struct field *);
	register struct recheader *crec;
	struct field tfield[2];
	FILE *prevfd, *tailfd[FSORTMAX+1];

	bzero(tailfd, sizeof(tailfd));
	prevfd = outfd;
	bzero(tfield, sizeof(tfield));
	if (ftbl[0].flags & R) tfield[0].weights = Rascii;
	else tfield[0].weights = ascii;
	tfield[0].icol.num = 1;
	weights = ftbl[0].weights;
	if (!buffer) {
		buffer = (u_char *) malloc(BUFSIZE);
		keylist = (u_char **) malloc(MAXNUM * sizeof(u_char *));
		if (!SINGL_FLD)
			linebuf = (u_char *) malloc(MAXLLEN);
	}
	bufend = buffer + BUFSIZE;
	if (binno >= 0) {
		tfiles.top = infiles.top + nfiles;
		get = getnext;
	}
	else {
		tfiles.top = 0;
		if (SINGL_FLD) get = makeline;
		else get = makekey;
	}				
	for (;;) {
		bzero(sizes, sizeof(sizes));
		c = ntfiles = 0;
		if (binno == weights[REC_D] &&
			!(SINGL_FLD && ftbl[0].flags & F)) {	/* pop */
			rd_append(weights[REC_D],
				infiles, nfiles, prevfd, buffer, bufend);
			break;
		}
		else if (binno == weights[REC_D]) {
			depth = 0;		/* start over on flat weights */
			ftbl = tfield;
			weights = ftbl[0].weights;
		}
		while (c != EOF) {
			keypos = keylist;
			nelem = 0;
			crec = (RECHEADER *) buffer;
			while ((c = get(binno, infiles, nfiles, crec, bufend,
					 ftbl)) == 0) {
				*keypos++ = crec->data + depth;
				if (++nelem == MAXNUM) {
					c = BUFFEND;
					break;
				}
				crec =(RECHEADER *)	((char *) crec +
				SALIGN(crec->length) + sizeof(TRECHEADER));
			}
			if (c == BUFFEND || ntfiles || mfct) {	/* push */
				if (panic >= PANIC) {
					fstack[MAXFCT-16+mfct].fd = ftmp();
					if (radixsort(keylist, nelem, weights,
						REC_D)) err(strerror(errno));
					append(keylist, nelem, depth, fstack[
					 MAXFCT-16+mfct].fd, putrec, ftbl);
					mfct++;
					/* reduce number of open files */
					if (mfct==16 || (c==EOF && ntfiles)) {
						fstack[tfiles.top+ntfiles].fd 
							= ftmp();
						fmerge(0, mstart, mfct, geteasy,
						 fstack[tfiles.top+ntfiles].fd, 
						 putrec, ftbl);
						++ntfiles;
						mfct = 0;
					}
				}
				else {
					fstack[tfiles.top+ntfiles].fd = ftmp();
					onepass(keylist, depth, nelem, sizes,
					weights, fstack[tfiles.top+ntfiles].fd);
					++ntfiles;
				}
			}
		}
		get = getnext;
		if (!ntfiles && !mfct) {	/* everything in memory--pop */
			if (nelem > 1)
			   if (radixsort(keylist, nelem, weights, REC_D))
				err(strerror(errno));
			append(keylist, nelem, depth, outfd, putline, ftbl);
			break;					/* pop */
		}
		if (panic >= PANIC) {
			if (!ntfiles) 
				fmerge(0, mstart, mfct, geteasy,
					outfd, putline, ftbl);
			else 
				fmerge(0, tfiles, ntfiles, geteasy, 
					outfd, putline, ftbl);
			break;
				
		}
		total = maxb = lastb = 0;	/* find if one bin dominates */
		for (i = 0; i < NBINS; i++)
		  if (sizes[i]) {
			if (sizes[i] > sizes[maxb])
				maxb = i;
			lastb = i;
			total += sizes[i];
		}
		if (sizes[maxb] < max((total / 2) , BUFSIZE))
			maxb = lastb;	/* otherwise pop after last bin */
		fstack[tfiles.top].lastb = lastb;
		fstack[tfiles.top].maxb = maxb;

		/* start refining next level. */
		get(-1, tfiles, ntfiles, crec, bufend, 0);	/* rewind */
		for (i = 0; i < maxb; i++) {
			if (!sizes[i])	/* bin empty; step ahead file offset */
				get(i, tfiles, ntfiles, crec, bufend, 0);
			else fsort(i, depth+1, tfiles, ntfiles, outfd, ftbl);
		}
		if (lastb != maxb) {
			if (prevfd != outfd) tailfd[panic] = prevfd;
			prevfd = ftmp();
			for (i = maxb+1; i <= lastb; i++)
				if (!sizes[i])
					get(i, tfiles, ntfiles, crec, bufend,0);
				else 
					fsort(i, depth+1, tfiles, ntfiles,
						prevfd, ftbl);
		}

		/* sort biggest (or last) bin at this level */
		depth++;
		panic++;
		binno = maxb;
		infiles.top = tfiles.top;	/* getnext will free tfiles, */
		nfiles = ntfiles;		/* so overwrite them */
	}
	if (prevfd != outfd) {
		concat(outfd, prevfd);
		fclose(prevfd);
	}
	for (i = panic; i >= 0; --i) if (tailfd[i]) {
			concat(outfd, tailfd[i]);
			fclose(tailfd[i]);
	}
}

/* 
 * Refines the current block and puts it into a segment of the current "memory".
 * Identical in spirit (and mostly in code) to the library radixsort routine.
 */
void
onepass(keylist, depth, nelem, sizes, wts, fd)
	u_char **keylist;
	long nelem;
	register u_char wts[];
	long sizes[];
	register int depth;
	FILE *fd;
{
	register u_char **p, **top, curb;
	register u_char **l2;
	register int i;
	register long tsizes[NBINS+1];
	long n;
	register int c[NBINS+1];

	if (!(l2 = (u_char **) malloc(nelem * sizeof(u_char **))))
		err(strerror(errno));
	bzero(c, sizeof(c));
	bzero(tsizes, sizeof(tsizes));
	p = keylist;
	for (i = nelem; i ; --i)
		++c[wts[*(*p++)]];
	for (i = 1; i <= NBINS; i++)
		c[i] += c[i-1];
	depth += sizeof(TRECHEADER);
	for (i = nelem; --i >= 0;) {
		--p;
		l2[--c[curb = wts[**p]]] = (*p -= depth);
		tsizes[curb] += ((RECHEADER *) *p)->length;
	}
	p = l2;
	for (i = 1; i <= NBINS; i++) {
		top = l2 + c[i];
		n = c[i] - c[i-1];
		tsizes[i-1] += n * sizeof(TRECHEADER);
		FWRITE(tsizes+i-1, sizeof(long), 1, fd);
		sizes[i-1] += tsizes[i-1];
		/* tell getnext how many elements in this bin, this segment. */
		for (; p < top; ++p) putrec((RECHEADER *) *p, fd);
	}
	free(l2);
}

