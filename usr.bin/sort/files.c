/* BSDI $Id: files.c,v 1.3 1994/01/13 23:09:49 cp Exp $ */

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
#include "fsort.h"

/* 
 * this is the subroutine for file management.  It keeps
 * the buffers for all temporary files.
 */

getnext(binno, infl0, nfiles, pos, end, dummy)
	int binno;
	union f_handle infl0;
	register struct recheader *pos;
	register u_char *end;
	int nfiles;
	struct field *dummy;
{
	int i;
	static long nleft = 0;
	static int cnt = 0, flag = -1;
	static u_char maxb = 0;
	static FILE *fd;

	if (nleft == 0) {
		if (binno < 0)	/* reset files. */ {
			for (i = 0; i < nfiles; i++) {
				rewind(fstack[infl0.top + i].fd);
				fstack[infl0.top + i].max_o = 0;
			}
			flag = -1;
			nleft = cnt = 0;
			return(-1);
		}
		maxb = fstack[infl0.top].maxb;
		for (; nleft == 0; cnt++) {
			if (cnt >= nfiles) {
				cnt = 0;
				return(EOF);
			}
			fd = fstack[infl0.top + cnt].fd;
			fread(&nleft, sizeof(nleft), 1, fd);
			if (binno < maxb)  
				fstack[infl0.top+cnt].max_o
				    += sizeof(nleft) + nleft;
			else if (binno == maxb) {
				if (binno != fstack[infl0.top].lastb) {
					fseek(fd, fstack[infl0.top+
						cnt].max_o, SEEK_SET);
					fread(&nleft, sizeof(nleft), 1, fd);
				}
				if (nleft == 0)
					fclose(fd);
			}
			else if (binno == maxb + 1) {		/* skip a bin */
				fseek(fd, nleft, SEEK_CUR);
				fread(&nleft, sizeof(nleft), 1, fd);
				flag = cnt;
			}
		}
	}
	if ((u_char *) pos > end - sizeof(TRECHEADER))
		return(BUFFEND);
	fread(pos, 1, sizeof(TRECHEADER), fd);
	if (end - pos->data < pos->length) {
		for (i = sizeof(TRECHEADER) - 1; i >= 0;  i--)
			ungetc(*((char *) pos + i), fd);
		return(BUFFEND);
	}
	fread(pos->data, pos->length, 1, fd);
	nleft -= pos->length + sizeof(TRECHEADER);
	if (nleft == 0 && binno == fstack[infl0.top].maxb)
		fclose(fd);

	return(0);

}

/* 
 * this is called when the line is the key.  It's only called
 * in the first fsort pass.
 */
makeline(flno, filelist, nfiles, buffer, bufend, dummy2)
	int flno;
	union f_handle filelist;
	int nfiles;
	struct recheader *buffer;
	u_char *bufend;
	struct field *dummy2;
{
	static char *opos;
	register char *pos, *end, *effend;
	static int fileno = 0;
	static FILE *fd = 0;
	static int overflow = 0;
	register int c;

	pos = (char *) buffer->data;
	effend = (char *) bufend - sizeof(struct trecheader);
	end = min(effend, pos + MAXLLEN);
	if (overflow) {
		memmove(pos, opos, bufend - (u_char *) opos);
		pos += ((char *) bufend - opos);
		overflow = 0;
	}
	for (;;) {
		if (flno >= 0) {
			if (!(fd = fstack[flno].fd))
				return(EOF);
		}
		else if (!fd) {
			if (fileno  >= nfiles) return(EOF);
			if (!(fd = fopen(filelist.names[fileno], "r")))
			    err("cannot open %s: %s", filelist.names[fileno],
					strerror(errno));
			++fileno;
		}
		while ((pos < end) && ((c = getc(fd)) != EOF)) {
			if ((*pos++ = c) == REC_D) {
				buffer->offset = 0;
				buffer->length = pos - (char *) buffer->data;
				return(0);
			}
		}
		if (pos >= end && end == (char *) effend) {
			overflow = 1;
			opos = (char *) buffer->data;
			return(BUFFEND);
		}
		else if (c == EOF) {
			if (buffer->data != (u_char *) pos) {
				fprintf(stderr, "warning: last character not record delimiter\n");
				*pos++ = REC_D;
				buffer->offset = 0;
				buffer->length = pos - (char *) buffer->data;
				return(0);
			}
			FCLOSE(fd);
			fd = 0;
			if (flno >= 0) fstack[flno].fd = 0;
		}
		else {
			buffer->data[100] = '\000';
			fprintf(stderr, "warning:line too long:ignoring %s...\n",
			buffer->data);
		}
	}
}

/* 
 * This generates keys. It's only called in the first fsort pass
 */
int
makekey(flno, filelist, nfiles, buffer, bufend, ftbl)
	int flno;
	union f_handle filelist;
	int nfiles;
	struct recheader *buffer;
	u_char *bufend;
	struct field *ftbl;
{
	extern int seq();
	static int (*get)();
	static int fileno = 0;
	static FILE *dbdesc = 0;
	static DBT dbkey[1], line[1];
	static int overflow = 0;
	int c;

	if (overflow) {
		overflow = 0;
		enterkey(buffer, line, bufend - (u_char *) buffer, ftbl);
		return(0);
	}
	for (;;) {
		if (flno >= 0) {
			get = seq;
			if (!(dbdesc = fstack[flno].fd))
				return(EOF);
		}
		else if (!dbdesc) {
			if (fileno  >= nfiles)
				return(EOF);
			dbdesc = fopen(filelist.names[fileno], "r");
			if (!dbdesc)
				err("can't open file. %s", strerror(errno));
			++fileno;
			get = seq;
		}
		if (!(c = get(dbdesc, line, dbkey))) {
			if (line->size > bufend - buffer->data)
				overflow = 1;
			else overflow = enterkey(buffer, line,
					bufend - (u_char *) buffer, ftbl);
			if (overflow)
				return(BUFFEND);
			else return(0);
		}
		if (c == EOF) {
			FCLOSE(dbdesc);
			dbdesc = 0;
			if (flno >= 0) fstack[flno].fd = 0;
		}
		else {
			((char *) line->data)[60] = '\000';
			fprintf(stderr, "warning: line too long: ignoring %s...\n",
			(char *) line->data);
		}
			
	}
}


int 
seq(fd, line, key)
	FILE *fd;
	DBT *line;
	DBT *key;
{
	static char *buf, flag = 1;
	register char *pos, *end;
	register int c;

	if (flag) {
		flag = 0;
		buf = (char *) linebuf;
		end = buf + MAXLLEN;
		line->data = buf;
	}
	pos = buf;
	while ((c = getc(fd)) != EOF) {
		if ((*pos++ = c) == REC_D) {
			line->size = pos - buf;
			return(0);
		}
		if (pos == end) {
			line->size = MAXLLEN;
			*--pos = REC_D;
			while ((c = getc(fd)) != EOF) {
				if (c == REC_D)
					return(BUFFEND);
			}
		}
	}
	if (pos != buf) {
		fprintf(stderr, "warning: last character not record delimiter\n");
		*pos++ = REC_D;
		line->size = pos - buf;
		return(0);
	}
	else 
		return(EOF);

}


void 
putrec(rec, fd)
	register struct recheader *rec;
	register FILE *fd;
{

	FWRITE(rec, 1, rec->length + sizeof(TRECHEADER), fd);
}


void 
putline(rec, fd)
	register struct recheader *rec;
	register FILE *fd;
{

	FWRITE(rec->data+rec->offset, 1, rec->length - rec->offset, fd);
}


int 
geteasy(flno, filelist, nfiles, rec, end, dummy2)
	int flno;
	union f_handle filelist;
	int nfiles;
	struct recheader *rec;
	u_char *end;
	struct field *dummy2;
{
	int i;
	FILE *fd;

	fd = fstack[flno].fd;
	if ((u_char *) rec > end - sizeof(TRECHEADER))
		return(BUFFEND);
	if (!fread(rec, 1, sizeof(TRECHEADER), fd)) {
		fclose(fd);
		fstack[flno].fd = 0;
		return(EOF);
	}
	if (end - rec->data < rec->length) {
		for (i = sizeof(TRECHEADER) - 1; i >= 0;  i--)
			ungetc(*((char *) rec + i), fd);
		return(BUFFEND);
	}
	fread(rec->data, rec->length, 1, fd);

	return(0);
}
