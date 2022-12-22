/*
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: ioport.c,v 1.2 1994/01/06 16:34:55 torek Exp $
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/proc.h>

#include <machine/tss.h>

#include <i386/isa/pcconsioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAPDEV	"/dev/console"

char	*progname;

void	display __P((int, struct iorange *));
void	map __P((int, int, int, struct iorange *));
__dead void usage __P((void));
int	val __P((char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, lim, mode, verbose, noexec;
	enum { UNSET, DISPLAY, MAP } op;
	struct iorange ior;
#define	SET(x) (op == UNSET ? (op = (x), mode = (ch)) : usage())

	op = UNSET;
	mode = 0;
	verbose = 0;
	noexec = 0;
	progname = argv[0];

	while ((ch = getopt(argc, argv, "ademnuv")) != EOF) {
		switch (ch) {

		case 'a':
			SET(DISPLAY);
			break;

		case 'd':
			SET(DISPLAY);
			break;

		case 'e':
			SET(DISPLAY);
			break;

		case 'm':
			SET(MAP);
			break;

		case 'n':
			noexec = 1;
			break;

		case 'u':
			SET(MAP);
			break;

		case 'v':
			verbose = 1;
			break;

		case '?':
		default:
			usage();
		}
	}

	if (op == UNSET)
		usage();

	argc -= optind;
	argv += optind;
	if (argc < 2 && op == MAP) {
		(void)fprintf(stderr, 
		    "%s: missing iobase or count specification\n",
		    progname);
		usage();
	}
	switch (argc) {

	case 0:
		ior.iobase = 0;
		ior.cnt = USER_IOPORTS;
		break;

	case 1:
		ior.iobase = val(argv[0]);
		if ((u_int)ior.iobase >= USER_IOPORTS)
			errx(1, "iobase must be in [0..%#x]", USER_IOPORTS - 1);
		ior.cnt = USER_IOPORTS - ior.iobase;
		break;

	case 2:
		ior.iobase = val(argv[0]);
		if ((u_int)ior.iobase >= USER_IOPORTS)
			errx(1, "iobase must be in [0..%#x]", USER_IOPORTS - 1);
		lim = USER_IOPORTS - ior.iobase;
		ior.cnt = val(argv[1]);
		if (ior.cnt == 0 || (u_int)ior.cnt > lim)
			errx(1, "with iobase of %#x, count must be in [1..%d]",
			    ior.iobase, lim);
		break;

	default:
		usage();
	}

	if (op == MAP)
		map(verbose, noexec, mode, &ior);
	else
		display(mode, &ior);
	exit(0);
}

/*
 * Peculiarly, a 1 bit means "disabled" rather than "enabled".
 * This macro inverts that.
 */
#define	abled(map, bit)	(((map)[(bit) >> 3] & (1 << ((bit) & 7))) == 0)

/*
 * We want to print something if:
 *	we are to print all (mode=='a'), or
 *	we are to print enables (mode=='e') and enabled (v==1), or
 *	we are to print disables (mode=='d') and not enabled (v==0).
 */
#define	print_it(mode, v) \
	((mode) == 'a' || ((mode) == 'e') == (v))

void
display(mode, rp)
	int mode;
	struct iorange *rp;
{
	register int i, stop, ena, oena;
	kvm_t *kd;
	struct i386tss tss;
	char errbuf[_POSIX2_LINE_MAX];
	static struct nlist nl[] = {
		{ "_tss" },
#define	X_TSS	0
		{ NULL }
	};
	static char *msg[2] = { "Disabled" , "Enabled" };

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
	if (kd == NULL)
		errx(1, "kvm_openfiles: %s", errbuf);
	if (kvm_nlist(kd, nl))
		errx(1, "kvm_nlist: %s", kvm_geterr(kd));
	if (kvm_read(kd, (u_long)nl[X_TSS].n_value, (char *)&tss,
	    sizeof tss) != sizeof(tss))
		errx(1, "kvm_read: %s", kvm_geterr(kd));
	if (kvm_close(kd))
		errx(1, "kvm_close: %s", kvm_geterr(kd));
	i = rp->iobase;
	stop = i + rp->cnt;
	oena = abled(tss.tss_iomap, i);
	if (print_it(mode, oena))
		printf("0x%04x", i);
	while (++i < stop) {
		ena = abled(tss.tss_iomap, i);
		if (ena != oena) {
			if (print_it(mode, oena))
				printf(" - 0x%04x : %s\n", i - 1, msg[oena]);
			if (print_it(mode, ena))
				printf("0x%04x", i);
			oena = ena;
		}
	}
	if (print_it(mode, oena))
		printf(" - 0x%04x : %s\n", i - 1, msg[oena]);
}

void
map(verbose, noexec, mode, rp)
	int verbose, noexec, mode;
	struct iorange *rp;
{
	int consfd, op;
	char *m1, *m2;

	if (mode == 'm') {
		m1 = "En";
		m2 = "PCCONIOCMAPPORT",
		op = PCCONIOCMAPPORT;
	} else {
		m1 = "Dis";
		m2 = "PCCONIOCUNMAPPORT";
		op = PCCONIOCUNMAPPORT;
	}
	if (verbose)
		(void)fprintf(stderr,
		    "%s: %sabling port access at %#x count=%d\n",
		    progname, m1, rp->iobase, rp->cnt);
	if (!noexec) {
		if ((consfd = open(MAPDEV, 0)) < 0) {
			err(1, "cannot open %s", MAPDEV);
			exit(1);
		}
		if (ioctl(consfd, op, rp) < 0)
			err(1, "ioctl(%s)", m2);
	}
}

__dead void
usage()
{

	(void)fprintf(stderr, "Usage: %s {-a | -d | -e} [iobase [count]]\n\
or:    %s [-v] {-m | -u} iobase count\n",
	    progname, progname);
	exit(1);
}

int
val(s)
	char *s;
{
	char *ep;
	long l;

	l = strtol(s, &ep, 0);
	if (l <= INT_MIN || l >= INT_MAX || ep == s || *ep != 0)
		usage();
	return (l);
}
