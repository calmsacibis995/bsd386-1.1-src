/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include <sys/types.h>
#define ISO9660
#include <sys/mount.h>

char usage_str[] = "usage: mount_iso9660 bdev dir\n";
char error_str[] = "mount failed\n";

void
usage()
{
	write (2, usage_str, sizeof usage_str-1);
	exit(1);
}
		
struct iso9660_args args;

void
exit(val)
{
	_exit(val);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *dev;
	char *dir;

	if (argc != 3)
		usage();

	dev = argv[1];
	dir = argv[2];

	args.fspec = dev;
	args.exflags = MNT_EXRDONLY;

	if (mount(MOUNT_ISO9660, dir, MNT_RDONLY, &args) < 0) {
		write (2, error_str, sizeof error_str - 1);
		exit(1);
	}

	return (0);
}
