/*
 * Copyright (c) 1987 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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
"@(#) Copyright (c) 1987 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)disklabel.c	5.20 (Berkeley) 2/9/91";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#endif /* not lint */

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <ufs/fs.h>
#include <string.h>
#define DKTYPENAMES
#include <sys/disklabel.h>
#include <stdio.h>
#include <ctype.h>
#include "pathnames.h"

/*
 * Disklabel: read and write disklabels.
 * The label is usually placed on one of the first sectors of the disk.
 * Many machines (VAX 11/750) also place a bootstrap in the same area,
 * in which case the label is embedded in the bootstrap.
 * The bootstrap source must leave space at the proper offset
 * for the label on such machines.
 */

#if defined(vax) || defined(i386)
#define RAWPARTITION	'c'
#else
#define RAWPARTITION	'a'
#endif

#ifndef BBSIZE
#define	BBSIZE	8192			/* size of boot area, with label */
#endif

#if defined(vax) || defined(i386)
#define	BOOT				/* also have bootstrap in "boot area" */
#define	BOOTDIR	_PATH_BOOTSTRAPS	/* source of boot binaries */
#else
#ifdef lint
#define	BOOT
#endif
#endif

#define	DEFEDITOR	_PATH_VI

#ifdef BOOT
char	*xxboot;
char	*bootxx;
#endif

char	*dkname;
char	*specname;
char	tmpfil[] = _PATH_TMP;

extern	int errno;
char	namebuf[BBSIZE], *np = namebuf;
struct	disklabel lab;
struct	disklabel *readlabel(), *makebootarea();
char	bootarea[BBSIZE];
char	boot0[MAXPATHLEN];
char	boot1[MAXPATHLEN];

enum	{
	UNSPEC, EDIT, NOWRITE, READ, RESTORE, WRITE, WRITEABLE, SET
} op = UNSPEC;

int	rflag, sflag;

#ifdef DEBUG
int	debug;
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	register struct disklabel *lp;
	FILE *t;
	int ch, f, error = 0, openflag;
	char *name = 0, *type;

	while ((ch = getopt(argc, argv, "NRWersw")) != EOF)
		switch (ch) {
			case 'N':
				if (op != UNSPEC)
					usage();
				op = NOWRITE;
				break;
			case 'R':
				if (op != UNSPEC)
					usage();
				op = RESTORE;
				break;
			case 'W':
				if (op != UNSPEC)
					usage();
				op = WRITEABLE;
				break;
			case 'e':
				if (op != UNSPEC)
					usage();
				op = EDIT;
				break;
			case 'r':
				++rflag;
				break;
			case 's':
				++sflag;
				break;
			case 'w':
				if (op != UNSPEC)
					usage();
				op = WRITE;
				break;
#ifdef DEBUG
			case 'd':
				debug++;
				break;
#endif
			case '?':
			default:
				usage();
		}
	argc -= optind;
	argv += optind;
	if (op == UNSPEC)
		op = READ;
	if (argc < 1)
		usage();
	if (sflag && (rflag || op == READ))
		usage();

	dkname = argv[0];
	if (dkname[0] != '/') {
		(void)sprintf(np, "%sr%s%c", _PATH_DEV, dkname, RAWPARTITION);
		specname = np;
		np += strlen(specname) + 1;
	} else
		specname = dkname;
	openflag = (op == READ ? O_RDONLY : O_RDWR);
	/*
	 * Use O_NONBLOCK if doing write-only operation on raw disk,
	 * or if not reading or writing.  O_NONBLOCK inhibits the
	 * driver from reading the on-disk label, which may be incorrect
	 * or may not exist yet.
	 */
	if ((rflag && (op == WRITE || op == RESTORE)) ||
	    op == NOWRITE || op == WRITEABLE || op == SET)
		openflag |= O_NONBLOCK;
	f = open(specname, openflag);
	if (f < 0 && errno == ENOENT && dkname[0] != '/') {
		(void)sprintf(specname, "%s/r%s", _PATH_DEV, dkname);
		np = namebuf + strlen(specname) + 1;
		f = open(specname, openflag);
	}
	if (f < 0)
		Perror(specname);

	switch (op) {
	case EDIT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		error = edit(lp, f);
		break;
	case NOWRITE: {
		int flag = 0;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;
	}
	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
#ifdef __i386__
		disklabel_display(specname, stdout, lp, NULL);
#else
		disklabel_display(specname, stdout, lp);
#endif
		error = disklabel_checklabel(lp);
		break;
	case RESTORE:
#ifdef BOOT
		if (rflag) {
			if (argc == 4) {	/* [ priboot secboot ] */
				xxboot = argv[2];
				bootxx = argv[3];
				lab.d_secsize = DEV_BSIZE;	/* XXX */
				lab.d_bbsize = BBSIZE;		/* XXX */
			}
			else if (argc == 3) 	/* [ disktype ] */
				makelabel(argv[2], (char *)NULL, &lab);
			else {
				fprintf(stderr,
"Must specify either disktype or bootfiles with -r flag of RESTORE option\n");
				exit(1);
			}
		}
		else if (argc == 3 || argc == 4)
			fprintf(stderr,
"Must use -r flag to specify bootfiles or use disktype bootfile defaults\n");
		else
#endif
		if (argc != 2)
			usage();
		lp = makebootarea(bootarea, &lab);
		if (!(t = fopen(argv[1],"r")))
			Perror(argv[1]);
#ifdef __i386__
		if (disklabel_getasciilabel(t, lp, NULL))
#else
		if (disklabel_getasciilabel(t, lp))
#endif
			error = writelabel(f, bootarea, lp);
		break;
	case WRITE:
		type = argv[1];
#ifdef BOOT
		if (argc > (rflag ? 5 : 2) || argc < 2)
			usage();
		if (argc > 3) {
			bootxx = argv[--argc];
			xxboot = argv[--argc];
		}
#else
		if (argc > 3 || argc < 2)
			usage();
#endif
		if (argc > 2)
			name = argv[--argc];
		makelabel(type, name, &lab);
		lp = makebootarea(bootarea, &lab);
		*lp = lab;
		if (disklabel_checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;
	case WRITEABLE: {
		int flag = 1;
		if (ioctl(f, DIOCWLABEL, (char *)&flag) < 0)
			Perror("ioctl DIOCWLABEL");
		break;
	}
	}
	exit(error);
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
makelabel(type, name, lp)
	char *type, *name;
	register struct disklabel *lp;
{
	register struct disklabel *dp;
	char *strcpy();

	dp = getdiskbyname(type);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type\n", type);
		exit(1);
	}
	*lp = *dp;
#ifdef BOOT
	/*
	 * Check if disktab specifies the bootstraps (b0 or b1).
	 */
	if (!xxboot && lp->d_boot0) {
		if (*lp->d_boot0 != '/')
			(void)sprintf(boot0, "%s/%s", BOOTDIR, lp->d_boot0);
		else
			(void)strcpy(boot0, lp->d_boot0);
		xxboot = boot0;
	}
	if (!bootxx && lp->d_boot1) {
		if (*lp->d_boot1 != '/')
			(void)sprintf(boot1, "%s/%s", BOOTDIR, lp->d_boot1);
		else
			(void)strcpy(boot1, lp->d_boot1);
		bootxx = boot1;
	}
	/*
	 * If bootstraps not specified anywhere, makebootarea()
	 * will choose ones based on the name of the disk special
	 * file. E.g. /dev/ra0 -> raboot, bootra
	 */
#endif /*BOOT*/
	/* d_packname is union d_boot[01], so zero */
	bzero(lp->d_packname, sizeof(lp->d_packname));
	if (name)
		(void)strncpy(lp->d_packname, name, sizeof(lp->d_packname));
}

writelabel(f, boot, lp, op)
	int f;
	char *boot;
	register struct disklabel *lp;
{
	register int i;
	int flag;
	off_t lseek();

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (sflag) {
		/* set label in kernel only */
		if (ioctl(f, DIOCSDINFO, lp) < 0) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		return (0);
	}
	if (rflag) {
		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.
		 */
		if (ioctl(f, DIOCSDINFO, lp) < 0 &&
		    errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		(void)lseek(f, (off_t)0, L_SET);
		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		flag = 1;
		if (ioctl(f, DIOCWLABEL, &flag) < 0)
			perror("ioctl DIOCWLABEL");
		if ((i = write(f, boot, lp->d_bbsize)) != lp->d_bbsize) {
			if (i == -1)
				perror("write");
			else
				fprintf(stderr,
				    "short write: wrote %d of %d\n",
				    i, lp->d_bbsize);
			return (1);
		}
		flag = 0;
		(void) ioctl(f, DIOCWLABEL, &flag);
	} else if (ioctl(f, DIOCWDINFO, lp) < 0) {
		l_perror("ioctl DIOCWDINFO");
		return (1);
	}
#ifdef vax
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		daddr_t alt;

		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			(void)lseek(f, (off_t)((alt + i) * lp->d_secsize), L_SET);
			if (write(f, boot, lp->d_secsize) < lp->d_secsize) {
				int oerrno = errno;
				fprintf(stderr, "alternate label %d ", i/2);
				errno = oerrno;
				perror("write");
			}
		}
	}
#endif
	return (0);
}

l_perror(s)
	char *s;
{
	int saverrno = errno;

	fprintf(stderr, "disklabel: %s: ", s);

	switch (saverrno) {

	case ESRCH:
		fprintf(stderr, "No disk label on disk;\n");
		fprintf(stderr,
		    "use \"disklabel -r\" to install initial label\n");
		break;

	case EINVAL:
		fprintf(stderr, "Label magic number or checksum is wrong!\n");
		fprintf(stderr, "(disklabel or kernel is out of date?)\n");
		break;

	case EBUSY:
		fprintf(stderr, "Open partition would move or shrink\n");
		break;

	case EXDEV:
		fprintf(stderr,
	"Labeled partition or 'a' partition must start at beginning of disk\n");
		break;

	default:
		errno = saverrno;
		perror((char *)NULL);
		break;
	}
}

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
struct disklabel *
readlabel(f)
	int f;
{
	register struct disklabel *lp;

	if (rflag) {
		if (read(f, bootarea, BBSIZE) < BBSIZE)
			Perror(specname);
		for (lp = (struct disklabel *)bootarea;
		    lp <= (struct disklabel *)(bootarea + BBSIZE - sizeof(*lp));
		    lp = (struct disklabel *)((char *)lp + 16))
			if (lp->d_magic == DISKMAGIC &&
			    lp->d_magic2 == DISKMAGIC)
				break;
		if (lp > (struct disklabel *)(bootarea+BBSIZE-sizeof(*lp)) ||
		    lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
		    dkcksum(lp) != 0) {
			fprintf(stderr,
	"Bad pack magic number (label is damaged, or pack is unlabeled)\n");
			/* lp = (struct disklabel *)(bootarea + LABELOFFSET); */
			exit (1);
		}
	} else {
		lp = &lab;
		if (ioctl(f, DIOCGDINFO, lp) < 0)
			Perror("ioctl DIOCGDINFO");
	}
	return (lp);
}

struct disklabel *
makebootarea(boot, dp)
	char *boot;
	register struct disklabel *dp;
{
	struct disklabel *lp;
	register char *p;
	int b;
#ifdef BOOT
	char	*dkbasename;
#endif /*BOOT*/

	lp = (struct disklabel *)(boot + (LABELSECTOR * dp->d_secsize) +
	    LABELOFFSET);
#ifdef BOOT
	if (!rflag)
		return (lp);

	if (xxboot == NULL || bootxx == NULL) {
		dkbasename = np;
		if ((p = rindex(dkname, '/')) == NULL)
			p = dkname;
		else
			p++;
		while (*p && !isdigit(*p))
			*np++ = *p++;
		*np++ = '\0';

		if (xxboot == NULL) {
			(void)sprintf(np, "%s/%sboot", BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			xxboot = np;
			(void)sprintf(xxboot, "%s/%sboot", BOOTDIR, dkbasename);
			np += strlen(xxboot) + 1;
		}
		if (bootxx == NULL) {
			(void)sprintf(np, "%s/boot%s", BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			bootxx = np;
			(void)sprintf(bootxx, "%s/boot%s", BOOTDIR, dkbasename);
			np += strlen(bootxx) + 1;
		}
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr, "bootstraps: xxboot = %s, bootxx = %s\n",
			xxboot, bootxx);
#endif

	b = open(xxboot, O_RDONLY);
	if (b < 0)
		Perror(xxboot);
	if (read(b, boot, (int)dp->d_secsize) < 0)
		Perror(xxboot);
	close(b);
	b = open(bootxx, O_RDONLY);
	if (b < 0)
		Perror(bootxx);
	if (read(b, &boot[dp->d_secsize], (int)(dp->d_bbsize-dp->d_secsize)) < 0)
		Perror(bootxx);
	(void)close(b);
#endif /*BOOT*/

	for (p = (char *)lp; p < (char *)lp + sizeof(struct disklabel); p++)
		if (*p) {
			fprintf(stderr,
			    "Bootstrap doesn't leave room for disk label\n");
			exit(2);
		}
	return (lp);
}


edit(lp, f)
	struct disklabel *lp;
	int f;
{
	register int c;
	struct disklabel label;
	FILE *fd;
	char *mktemp();

	(void) mktemp(tmpfil);
	fd = fopen(tmpfil, "w");
	if (fd == NULL) {
		fprintf(stderr, "%s: Can't create\n", tmpfil);
		return (1);
	}
	(void)fchmod(fd, 0600);
#ifdef __i386__
	disklabel_display(specname, fd, lp, NULL);
#else
	disklabel_display(specname, fd, lp);
#endif
	fclose(fd);
	for (;;) {
		if (!editit())
			break;
		fd = fopen(tmpfil, "r");
		if (fd == NULL) {
			fprintf(stderr, "%s: Can't reopen for reading\n",
				tmpfil);
			break;
		}
		bzero((char *)&label, sizeof(label));
#ifdef __i386__
		if (disklabel_getasciilabel(fd, &label, NULL)) {
#else
		if (disklabel_getasciilabel(fd, &label)) {
#endif
			*lp = label;
			if (writelabel(f, bootarea, lp, WRITE) == 0) {
				(void) unlink(tmpfil);
				return (0);
			}
		}
		printf("re-edit the label? [y]: "); fflush(stdout);
		c = getchar();
		if (c != EOF && c != (int)'\n')
			while (getchar() != (int)'\n')
				;
		if  (c == (int)'n')
			break;
	}
	(void) unlink(tmpfil);
	return (1);
}

editit()
{
	register int pid, xpid;
	int stat, omask;
	extern char *getenv();

	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
	while ((pid = fork()) < 0) {
		extern int errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno != EAGAIN) {
			perror("fork");
			return(0);
		}
		sleep(1);
	}
	if (pid == 0) {
		register char *ed;

		sigsetmask(omask);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigsetmask(omask);
	return(!stat);
}

Perror(str)
	char *str;
{
	fputs("disklabel: ", stderr); perror(str);
	exit(4);
}

usage()
{
#ifdef BOOT
	fprintf(stderr, "%-62s%s\n%-62s%s\n%-62s%s\n%-62s%s\n%-62s%s\n",
"usage: disklabel [-r] disk", "(to read label)",
"or disklabel -w [-rs] disk type [ packid ] [ xxboot bootxx ]", "(to write label)",
"or disklabel -e [-rs] disk", "(to edit label)",
"or disklabel -R [-rs] disk protofile [ type | xxboot bootxx ]", "(to restore label)",
"or disklabel [-NW] disk", "(to write disable/enable label)");
#else
	fprintf(stderr, "%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n",
"usage: disklabel [-r] disk", "(to read label)",
"or disklabel -w [-rs] disk type [ packid ]", "(to write label)",
"or disklabel -e [-rs] disk", "(to edit label)",
"or disklabel -R [-rs] disk protofile", "(to restore label)",
"or disklabel [-NW] disk", "(to write disable/enable label)");
#endif
	exit(1);
}
