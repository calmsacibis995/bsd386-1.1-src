/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * This program is called by mount to handle iso9660 file systems.
 * You can pass in the -n flag by giving mount the option "-o -n" to mount.
 */

#include <stdio.h>
#include <sys/types.h>
#define ISO9660
#include <sys/mount.h>

void
usage()
{
	fprintf(stderr, "usage: mount_iso9660 [-n] bdev dir\n");
	exit(1);
}
		
int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	char *dev;
	char *dir;
	struct iso9660_args args;
	int c;
	int flags;
	int raw=0;	/* When set, don't do any file name translations. */

	flags = MNT_RDONLY;

	while ((c = getopt(argc, argv, "F:n")) != EOF) {
		switch (c) {
		case 'F':
			flags |= atoi(optarg);
			break;
		case 'n':
			raw = 1;
			break;
		default:
			usage();
		}
	}

	if (optind + 2 != argc)
		usage();

	dev = argv[optind];
	dir = argv[optind + 1];

	bzero(&args, sizeof args);
	args.fspec = dev;
	args.exflags = MNT_EXRDONLY;
	args.exroot = 0;
	args.raw = raw;

	if (mount(MOUNT_ISO9660, dir, flags, &args) < 0) {
		perror("mount");
		exit(1);
	}

	exit(0);
}

