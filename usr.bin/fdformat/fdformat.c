/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * floppy disk formatting - works with BSDI floppy driver
 *
 * format [-v] device disktab_type
 *
 *  	-v	verbose
 *
 * BSDI $Id: fdformat.c,v 1.3 1993/12/21 04:56:04 polk Exp $ 
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <paths.h>

#define	MAX_SECPERTRACK		18
#define FDBLK		512


struct fmt {
	char cyl;
	char h;
	char sec;
	char type;
};

int lflag;		/* put disklabel to disk after formatting */
int vflag;		/* verbose */
char *progname;
extern errno;

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	int ch, f;
	char *dkname, *specname, *type;
	char namebuf[80];
	struct disklabel *dp;

	progname = argv[0];
	while ((ch = getopt(argc, argv, "vl")) != EOF)
	switch (ch) {
	case 'l':
		lflag = 1;
		break;
	case 'v':
		vflag = 1;
		break;
	case '?':
	default:
		usage();
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();
		
	dkname = argv[0];
	type = argv[1];
	
	if (dkname[0] != '/') {
		struct stat st;

		(void)sprintf(namebuf, "%s%s", _PATH_DEV, dkname);
		if (stat(namebuf, &st) == -1)
			(void)sprintf(namebuf, "%s%sa", _PATH_DEV, dkname);
		specname = namebuf;
	} else
		specname = dkname;
	f = open(specname, O_WRONLY);
	if (f < 0)
		sorry("%s: %s", specname, strerror(errno));

	dp = getdiskbyname(type);
	if (dp == NULL) 
		sorry("unknown disk type: %s\n", type);

	/*
	 * Set kernel idea of disk geometry for specified format.
	 */
	dp->d_checksum = 0;
	dp->d_checksum = dkcksum(dp);
	if (ioctl(f, DIOCSDINFO, (char *)dp))
		sorry("%s: can't set disk label: %s",
		    specname, strerror(errno));

	if (vflag) {
		printf("Formatting %s as %s: ", specname, type);
		fflush(stdout);
	}
	format(f, dp);
	close(f);
	
	if (vflag)
		printf("\tFormatting done.\n");
	if (lflag) {
		char cmd[80];
		sprintf(cmd, "disklabel -w -r %s %s floppy", specname, type);
		if (vflag)
			printf("%s\n", cmd);
		if (system(cmd))
			sorry("disklabel failed");
	}
	exit(0);
}


format(f, dp)
	register struct disklabel *dp;
{
	int cyl, h;
	struct format_op df;
	struct fmt fid[MAX_SECPERTRACK];
	
	df.df_buf = (char *)fid;
	df.df_startblk = 0;
	df.df_count = sizeof(struct fmt) * dp->d_nsectors;
	
	if (vflag) {
		printf("track          ");	/* 1 + 9 spaces - see below*/
		fflush(stdout);
	}
	for (cyl = 0; cyl < dp->d_ncylinders; cyl ++) {
		for (h = 0; h < dp->d_ntracks; h ++) {
			genfid(fid, dp, cyl, h);
			if (ioctl(f, DIOCWFORMAT, &df) < 0) {
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
				sorry(strerror(errno));
			}
			if (vflag) {
				/* 9 \b - see above :-) */
				printf("\b\b\b\b\b\b\b\b\b%2d head %1d", cyl, h);
				fflush(stdout);
			}
			df.df_startblk += dp->d_nsectors 
				* dp->d_secsize / FDBLK;
		}
	}
}			
			
/* generate format id array for given track and head
 */
genfid(fid, dp, cyl, h)
	register struct fmt *fid;
	register struct disklabel *dp;
	int cyl;
	int h;
{
	int sec;
	register i;
	char	type;

	if (dp->d_interleave >= dp->d_nsectors)
		sorry("interleave (%d) > sectors-per-track (%d)", 
			dp->d_interleave, dp->d_nsectors);
			
	for (i = 0 ; i < dp->d_nsectors ; i++ )
		fid[i].sec = 0;
		
	type = dp->d_secsize >> 8;
	if (type == 4)
		type = 3;
		
	for (i = 0, sec = 1; sec <= dp->d_nsectors ; sec++) {
		fid[i].cyl = (char) cyl;
		fid[i].h = (char) h;
		fid[i].sec = (char) sec;
		fid[i].type = type;
		
		i += dp->d_interleave;
		if (i >= dp->d_nsectors) {
			i -= dp->d_nsectors;
			while (i < dp->d_nsectors && fid[i].sec == 0)
				++ i;
		}
	}
}

usage()
{
	printf("%s [-l] [-v] device disktab_type\n", progname);
	exit(1);
}

sorry(char *fmt, ...) 
{
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}
