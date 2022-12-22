/* BSDI $Id: sort.c,v 1.4 1994/01/29 21:16:54 donn Exp $ */

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
#include <unistd.h>
#include "pathnames.h"

u_char REC_D = '\n';
u_char d_mask[NBINS];		/* flags for rec_d, field_d, <blank> */

/*
 * weight tables.  Gweights is one of ascii, Rascii..
 * modified to weight rec_d = 0 (or 255) 
 */
extern u_char gweights[NBINS];
u_char ascii[NBINS], Rascii[NBINS], RFtable[NBINS], Ftable[NBINS];

/*
 * masks of ignored characters.  Alltable is 256 ones 
 */
u_char dtable[NBINS], itable[NBINS], alltable[NBINS];
int SINGL_FLD = 0, SEP_FLAG = 0, UNIQUE = 0;
struct coldesc clist[(ND+1)*2];
int ncols = 0;
extern struct coldesc clist[(ND+1)*2];
extern int ncols;

char devstdin[] = _PATH_STDIN;
char toutpath[MAXPATHLEN] = "";
void cleanup __P((int));

main(argc, argv)
	int argc;
	char *argv[];
{
	struct field fldtab[ND+2], *ftpos;
	extern int optind;
	union f_handle filelist;
	extern char *optarg;
	int ch, i, tmp = 0, stdinflag = 0;
	char cflag = 0, mflag = 0;
	char *outfile, *outpath = 0;
	int (*get)();
	FILE *outfd;

	bzero(fldtab, (ND+2) * sizeof(struct field));
	bzero(d_mask, NBINS);
	d_mask[REC_D = '\n'] = REC_D_F;
	SINGL_FLD = SEP_FLAG = 0;
	d_mask['\t'] = d_mask[' '] = BLANK | FLD_D;
	ftpos = fldtab;
	fixit(&argc, argv);
	while ((ch = getopt(argc, argv, "bcdfik:mHno:rt:T:ux")) != EOF) {

	switch (ch) {
		case 'b': 
			fldtab->flags |= BI | BT;
			break;
		case 'n':
		case 'd':
		case 'i':
		case 'f':
		case 'r': 
			fldtab->flags |= optval(ch);
			break;
		case 'o':
			outpath = optarg;
			break;
		case 'k':	/* initialize next field and set max column */
			 setfield(optarg, ++ftpos, fldtab->flags);
			break;
		case 't':
			if (SEP_FLAG) usage("multiple field delimiters");
			SEP_FLAG = 1;
			d_mask[' '] &= ~FLD_D;
			d_mask['\t'] &= ~FLD_D;
			d_mask[*optarg] |= FLD_D;
			if (d_mask[*optarg] & REC_D_F)
				err("record/field delimiter clash.");
			break;
		case 'T':
			if (REC_D != '\n') usage("multiple record delimiters");
			if ('\n' == (REC_D = *optarg))
				break;
			d_mask['\n'] = d_mask[' '];
			d_mask[REC_D] = REC_D_F;
			break;
		case 'u':
			UNIQUE = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'H':
			PANIC = 0;
			break;
		case '?':
		default: usage("");
		}
	}
	if (ftpos == fldtab)
		setfield("1",  ++ftpos, fldtab->flags & (R|N));
	if (cflag && argc > optind+1)
		err("too many input files for -c option");
	if (argc - 2 > optind && !strcmp(argv[argc-2], "-o")) {
		outpath = argv[argc-1];
		argc -= 2;
	}
	if (mflag && argc - optind > MAXFCT - (16+1))
		err("too many input files for -m option");

	for (i = optind; i < argc; i++) {
		/* allow one occurrence of /dev/stdin */
		if (!strcmp(argv[i], "-") || !strcmp(argv[i], devstdin)) {
			if (stdinflag)
				fprintf(stderr, "sort: ignoring extra \"%s\" in file list\n", argv[i]);
			else {
				stdinflag = 1;
				argv[i] = devstdin;
			}
		}
		else if (ch = access(argv[i], R_OK))
			err("%s: %s", argv[i], strerror(errno));
	}

	if (!(fldtab->flags & (I|D) || fldtab[1].icol.num)) {
		SINGL_FLD = 1;
		fldtab[0].icol.num = 1;
	}
	else {
		if (!fldtab[1].icol.num) {
			fldtab[0].flags &= ~(BI|BT);
			setfield("1", ++ftpos, fldtab->flags);
		}
		fldreset(fldtab);
		fldtab[0].flags &= ~F;
	}
	settables(fldtab[0].flags);
	num_init();
	fldtab->weights = gweights;

	if (optind == argc) 
		argv[--optind] = devstdin;
	filelist.names = argv+optind;

	if (SINGL_FLD) 
		get = makeline;
	else 
		get = makekey;

	if (cflag) {
		order(filelist, get, fldtab);
		/* NOT REACHED */
	}

	if (!outpath) 
		outfile = outpath = "/dev/stdout";
	else if (!(ch = access(outpath, 0))) {
		struct sigaction act = {0, SIG_BLOCK, 6};
		int sigtable[] = {SIGHUP, SIGINT, SIGPIPE, SIGXCPU, SIGXFSZ,
			SIGVTALRM, SIGPROF, 0};

		errno = 0;
		if (access(outpath, W_OK))
			err("%s: %s", outpath, strerror(errno));
		act.sa_handler = cleanup;
		outfile = toutpath;
		sprintf(outfile, "%sXXXX", outpath);
		outfile = mktemp(outfile);
		if (!outfile) 
			err("mktmp failed: %s", strerror(errno));
		for (i = 0; sigtable[i]; ++i)	/* always unlink toutpath */
			sigaction(sigtable[i], &act, 0);
	}
	else 
		outfile = outpath;

	if (!(outfd = fopen(outfile, "w"))) 
		if (errno == EACCES) 
			err("%s: protected directory", strerror(errno));
		else 
			err("fopen failed: %s %s", outfile, strerror(errno));

	if (mflag) 
		fmerge(-1, filelist, argc-optind, get, outfd, putline, fldtab);
	else 
		fsort(-1, 0, filelist, argc-optind, outfd, fldtab);

	if (outfile != outpath) {
		if (access(outfile, 0))
			err("Someone clobbered temporary file %s", outfile);
		unlink(outpath);
		if (link(outfile, outpath)) {
			fprintf(stderr, "cannot link %s; output left in %s\n",
				outpath, outfile);
			exit(2);
		}
		unlink(outfile);
	}

	exit(0);
}

void 
cleanup(s)
	int s;
{
	if (toutpath[0])
		unlink(toutpath);
	exit(2);			/* return 2 on error/interrupt */
}
