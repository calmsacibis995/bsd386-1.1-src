/*	BSDI $Id: ps.c,v 1.4 1994/01/05 00:16:59 polk Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ps.c	5.46 (Berkeley) 4/3/92";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/kinfo.h>
#include <sys/tty.h>
#include <nlist.h>
#include <kvm.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include "ps.h"

#ifdef SPPWAIT
#define NEWVM
#endif

KINFO *kinfo;
struct varent *vhead, *vtail;

int	eval;			/* exit value */
int	rawcpu;			/* -C */
int	sumrusage;		/* -S */
int	termwidth;		/* width of screen (0 == infinity) */
int	totwidth;		/* calculated width of requested variables */

static int needuser, needcomm, needenv;

enum sort { DEFAULT, SORTMEM, SORTCPU } sortby = DEFAULT;

uid_t	getuid();
char	*ttyname();

char dfmt[] = "pid tt state time command";
char jfmt[] = "user pid ppid pgid sess jobc state tt time command";
char lfmt[] = "uid pid ppid cpu pri nice vsz rss wchan state tt time command";
char   o1[] = "pid";
char   o2[] = "tt state time command";
char ufmt[] = "user pid %cpu %mem vsz rss tt state start time command";
#ifdef NEWVM
char vfmt[] =
	"pid state time sl re pagein vsz rss lim tsiz %cpu %mem command";
#else
/*	if we ever use trs again we need to uncomment in ps.1 */
char vfmt[] =
	"pid state time sl re pagein vsz rss lim tsiz trs %cpu %mem command";
#endif

kvm_t *kd;

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	register struct proc *p;
	register struct kinfo_proc *kp;
	register struct varent *vent;
	int nentries;
	register int i;
	struct winsize ws;
	dev_t ttydev;
	int all, ch, flag, fmt, lineno, pid, prtheader, uid, what, xflg;
	int pscomp();
	char *nlistf, *memf, *swapf;
	char *kludge_oldps_options();
	char errbuf[80];
	int wflag = 0;

	if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDERR_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDIN_FILENO,  TIOCGWINSZ, (char *)&ws) == -1) ||
	     ws.ws_col == 0)
		termwidth = 79;
	else
		termwidth = ws.ws_col - 1;

	if (argc > 1)
		argv[1] = kludge_oldps_options(argv[1]);

	fmt = 0;
	all = xflg = 0;
	pid = uid = -1;
	ttydev = NODEV;
	memf = nlistf = swapf = NULL;
	while ((ch = getopt(argc, argv,
	    "aCeghjLlM:mN:O:o:p:rSTt:uvW:wx")) != EOF)
		switch((char)ch) {
		case 'a':
			all = 1;
			break;
		case 'e':
			/* XXX set ufmt */
			needenv = 1;
			break;
		case 'C':
			rawcpu = 1;
			break;
		case 'g':
			break;	/* no-op */
		case 'h':
			prtheader = ws.ws_row > 5 ? ws.ws_row : 22;
			break;
		case 'j':
			parsefmt(jfmt);
			fmt = 1;
			jfmt[0] = '\0';
			break;
		case 'L':
			showkey();
			exit(0);
		case 'l':
			parsefmt(lfmt);
			fmt = 1;
			lfmt[0] = '\0';
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			sortby = SORTMEM;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'O':
			parsefmt(o1);
			parsefmt(optarg);
			parsefmt(o2);
			o1[0] = o2[0] = '\0';
			fmt = 1;
			break;
		case 'o':
			parsefmt(optarg);
			fmt = 1;
			break;
		case 'p':
			pid = atoi(optarg);
			xflg = 1;
			break;
		case 'r':
			sortby = SORTCPU;
			break;
		case 'S':
			sumrusage = 1;
			break;
		case 'T':
			if ((optarg = ttyname(STDIN_FILENO)) == NULL)
				err("stdin: not a terminal");
			/* FALLTHROUGH */
		case 't': {
			char *ttypath;
			struct stat stbuf;
			char pathbuf[MAXPATHLEN];

			if (strcmp(optarg, "co") == 0)
				ttypath = _PATH_CONSOLE;
			else if (*optarg != '/')
				(void) sprintf(ttypath = pathbuf, "%s%s",
				    _PATH_TTY, optarg);
			else
				ttypath = optarg;
			if (stat(ttypath, &stbuf) == -1)
				err("%s: %s", ttypath, strerror(errno));
			if (!S_ISCHR(stbuf.st_mode))
				err("%s: not a terminal", ttypath);
			ttydev = stbuf.st_rdev;
			break;
		}
		case 'u':
			parsefmt(ufmt);
			sortby = SORTCPU;
			fmt = 1;
			ufmt[0] = '\0';
			break;
		case 'v':
			parsefmt(vfmt);
			sortby = SORTMEM;
			fmt = 1;
			vfmt[0] = '\0';
			break;
		case 'W':
			swapf = optarg;
			break;
		case 'w':
			if (wflag)
				termwidth = UNLIMITED;
			else if (termwidth < 131)
				termwidth = 131;
			wflag++;
			break;
		case 'x':
			xflg = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {

		nlistf = *argv;
		if (*++argv) {
			memf = *argv;
			if (*++argv)
				swapf = *argv;
		}
	}
#endif
	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL || swapf != NULL)
		setgid(getgid());

	kd = kvm_openfiles(nlistf, memf, swapf, O_RDONLY, errbuf);
	if (kd == 0)
		err("%s", errbuf);

	if (!fmt)
		parsefmt(dfmt);

	if (!all && ttydev == NODEV && pid == -1)  /* XXX - should be cleaner */
		uid = getuid();

	/*
	 * scan requested variables, noting what structures are needed,
	 * and adjusting header widths as appropiate.
	 */
	scanvars();
	/*
	 * get proc list
	 */
	if (uid != -1) {
		what = KINFO_PROC_UID;
		flag = uid;
	} else if (ttydev != NODEV) {
#if defined(hp300) || defined(i386)
		dev_t c2rdev(dev_t);

		flag = c2rdev(ttydev);
#else
		flag = ttydev;
#endif
		what = KINFO_PROC_TTY;
	} else if (pid != -1) {
		what = KINFO_PROC_PID;
		flag = pid;
	} else
		what = KINFO_PROC_ALL;
	/*
	 * select procs
	 */
	if ((kp = kvm_getprocs(kd, what, flag, &nentries)) == 0)
		err("%s", kvm_geterr(kd));
	kinfo = malloc(nentries * sizeof(*kinfo));
	if (kinfo == NULL)
		err("%s", strerror(errno));
	for (i = nentries; --i >= 0; ++kp) {
		kinfo[i].ki_p = kp;
		if (needuser)
			saveuser(&kinfo[i]);
	}
	/*
	 * print header
	 */
	printheader();
	if (nentries == 0)
		exit(0);
	/*
	 * sort proc list
	 */
	qsort((void *)kinfo, nentries, sizeof(KINFO), pscomp);
	/*
	 * for each proc, call each variable output function.
	 */
	for (i = lineno = 0; i < nentries; i++) {
		if (xflg == 0 && (KI_EPROC(&kinfo[i])->e_tdev == NODEV ||
		    (KI_PROC(&kinfo[i])->p_flag & SCTTY ) == 0))
			continue;
		for (vent = vhead; vent; vent = vent->next) {
			(*vent->var->oproc)(&kinfo[i], vent->var, vent->next);
			if (vent->next != NULL)
				(void) putchar(' ');
		}
		(void) putchar('\n');
		if (prtheader && lineno++ == prtheader-4) {
			(void) putchar('\n');
			printheader();
			lineno = 0;
		}
	}
	exit(eval);
}

#if defined(hp300) || defined(i386)
/*
 * XXX  If this is the console device, attempt to ascertain
 * the true console device dev_t.
 */

struct nlist c2rnl[] = {
        { "_cn_tty" },
#define X_CNTTY         0
        { "" },
};

dev_t
c2rdev(dev_t tdev)
{
	static dev_t cn_dev;
	struct tty cn_tty, *cn_ttyp;

	if (tdev != 0)
		return (tdev);

	if (cn_dev)
		return (cn_dev);
	if (kvm_nlist(kd, c2rnl))
		return (0);
	if (kvm_read(kd, (u_long)c2rnl[X_CNTTY].n_value,
	    (char *)&cn_ttyp, sizeof(cn_ttyp)) > 0) {
		(void)kvm_read(kd, (u_long)cn_ttyp,
		    (char *)&cn_tty, sizeof (cn_tty));
		cn_dev = cn_tty.t_dev;
	}
	return (cn_dev);
}
#endif

scanvars()
{
	register struct varent *vent;
	register VAR *v;
	register int i;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		i = strlen(v->header);
		if (v->width < i)
			v->width = i;
		totwidth += v->width + 1;	/* +1 for space */
		if (v->flag & USER)
			needuser = 1;
		if (v->flag & COMM)
			needcomm = 1;
	}
	totwidth--;
}

extern char *fmt_argv __P((char **, char *, int));

static char *
fmt(fn, ki, comm, maxlen)
	char **(*fn) __P((kvm_t *, const struct kinfo_proc *, int));
	KINFO *ki;
	char *comm;
	int maxlen;
{
	register char *s;

	s = fmt_argv((*fn)(kd, ki->ki_p, termwidth), comm, maxlen);
	if (s == NULL)
		err("%s", strerror(errno));
	return (s);
}

saveuser(ki)
	KINFO *ki;
{
	register struct usave *usp = &ki->ki_u;
	struct pstats pstats;
	extern char *fmt_argv();

	if (kvm_read(kd, (u_long)&KI_PROC(ki)->p_addr->u_stats,
		     (char *)&pstats, sizeof(pstats)) == sizeof(pstats)) {
		/*
		 * The u-area might be swapped out, and we can't get
		 * at it because we have a crashdump and no swap.
		 * If it's here fill in these fields, otherwise, just
		 * leave them 0.
		 */
		usp->u_start = pstats.p_start;
		usp->u_ru = pstats.p_ru;
		usp->u_cru = pstats.p_cru;
		usp->u_valid = 1;
	} else
		usp->u_valid = 0;
	/*
	 * save arguments if needed
	 */
	if (needcomm)
		ki->ki_args = fmt(kvm_getargv, ki, KI_PROC(ki)->p_comm,
		    MAXCOMLEN);
	else
		ki->ki_args = NULL;
	if (needenv)
		ki->ki_env = fmt(kvm_getenvv, ki, (char *)NULL, 0);
	else
		ki->ki_env = NULL;
}

pscomp(k1, k2)
	KINFO *k1, *k2;
{
	int i;
#ifdef NEWVM
#define VSIZE(k) (KI_EPROC(k)->e_vm.vm_dsize + KI_EPROC(k)->e_vm.vm_ssize + \
		  KI_EPROC(k)->e_vm.vm_tsize)
#else
#define VSIZE(k) ((k)->ki_p->p_dsize + (k)->ki_p->p_ssize + (k)->ki_e->e_xsize)
#endif

	if (sortby == SORTCPU)
		return (getpcpu(k2) - getpcpu(k1));
	if (sortby == SORTMEM)
		return (VSIZE(k2) - VSIZE(k1));
	i =  KI_EPROC(k1)->e_tdev - KI_EPROC(k2)->e_tdev;
	if (i == 0)
		i = KI_PROC(k1)->p_pid - KI_PROC(k2)->p_pid;
	return (i);
}

/*
 * ICK (all for getopt), would rather hide the ugliness
 * here than taint the main code.
 *
 *  ps foo -> ps -foo
 *  ps 34 -> ps -p34
 *
 * The old convention that 't' with no trailing tty arg means the users
 * tty, is only supported if argv[1] doesn't begin with a '-'.  This same
 * feature is available with the option 'T', which takes no argument.
 */
char *
kludge_oldps_options(s)
	char *s;
{
	size_t len;
	char *newopts, *ns, *cp;

	len = strlen(s);
	if ((newopts = ns = malloc(len + 2)) == NULL)
		err("%s", strerror(errno));
	/*
	 * options begin with '-'
	 */
	if (*s != '-')
		*ns++ = '-';	/* add option flag */
	/*
	 * gaze to end of argv[1]
	 */
	cp = s + len - 1;
	/*
	 * if last letter is a 't' flag with no argument (in the context
	 * of the oldps options -- option string NOT starting with a '-' --
	 * then convert to 'T' (meaning *this* terminal, i.e. ttyname(0)).
	 */
	if (*cp == 't' && *s != '-')
		*cp = 'T';
	else {
		/*
		 * otherwise check for trailing number, which *may* be a
		 * pid.
		 */
		while (cp >= s && isdigit(*cp))
			--cp;
	}
	cp++;
	bcopy(s, ns, (size_t)(cp - s));	/* copy up to trailing number */
	ns += cp - s;
	/*
	 * if there's a trailing number, and not a preceding 'p' (pid) or
	 * 't' (tty) flag, then assume it's a pid and insert a 'p' flag.
	 */
	if (isdigit(*cp) && (cp == s || cp[-1] != 't' && cp[-1] != 'p' &&
	    (cp - 1 == s || cp[-2] != 't')))
		*ns++ = 'p';
	(void) strcpy(ns, cp);		/* and append the number */

	return (newopts);
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "ps: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(1);
	/* NOTREACHED */
}

usage()
{
	(void) fprintf(stderr,
"usage: ps [-aChjlmrSTuvwx] [-O|o fmt] [-p pid] [-t tty]\n\t  [-M core] [-N system] [-W swap]\n       ps [-L]\n");
	exit(1);
}
