/*	BSDI	$Id: mount_msdos.c,v 1.1.1.1 1993/12/21 04:19:40 polk Exp $	*/

/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * This program is called by mount to handle msdosfs file systems.
 */

#include <stdio.h>
#include <sys/types.h>
#define MSDOSFS
#include <sys/mount.h>

void
usage()
{
	fprintf(stderr, "usage: mount_msdos [ -F flags ] bdev dir\n");
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
	struct msdosfs_args args;
	int c;
	int flags=0;

	while ((c = getopt(argc, argv, "F:")) != EOF) {
		switch (c) {
		case 'F':
			flags |= atoi(optarg);
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
	if (flags & MNT_RDONLY)
		args.exflags = MNT_EXRDONLY;
	else
		args.exflags = 0;
	args.exroot = 0;

	if (mount(MOUNT_MSDOS, dir, flags, &args) < 0) {
		perror("mount");
		exit(1);
	}

	exit(0);
}
