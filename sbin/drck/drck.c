/*	BSDI $Id: drck.c,v 1.1.1.1 1992/02/19 17:34:27 trent Exp $	*/
/*-
 * drck.c
 *
 * Usage:	drck disk
 *	where disk is something like 'wd2' or 'sd0'.
 *	The raw 'c' partition for the given disk must exist.
 * Function:	Read a disk from one end to the other looking for
 *	bad blocks; the cylinder, head and sector for each bad
 *	block are printed.  The program reads the disk a cylinder
 *	at a time, so the system ought to have sufficient program
 *	memory to hold more than one disk cylinder.
 * Date:	Sun Dec  7 19:27:24 MST 1986
 * Author:	Donn Seeley
 * Remarks:
 *	Dd don't cut it.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bomb.h"

#ifdef i386
#define RAWPARTITION    'c'
#else
#define RAWPARTITION    'a'
#endif

int vflag;

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *buf, *diskname;
	register int buflen;
	register int nbytes;
	int diskf;
	struct disklabel dl;
	register off_t pos, npos;
	off_t disk_size;
	int ch;

	program_name = argv[0];

	while ((ch = getopt(argc, argv, "v")) != EOF)
		switch (ch) {
		case 'v':
			++vflag;
			break;
		default:
			sbomb("usage: %s [-v] disk", program_name);
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		sbomb("usage: %s [-v] disk", program_name);
	setlinebuf(stdout);

	/*
	 * Open the raw disk device.
	 */
	if ((diskname = malloc((unsigned) strlen(argv[0]) + sizeof "/dev/rc")) == NULL)
		bomb("%sr%s%c: can't allocate name", _PATH_DEV, argv[0],
			RAWPARTITION);
	(void) sprintf(diskname, "%sr%s%c", _PATH_DEV, argv[0], RAWPARTITION);
	if ((diskf = open(diskname, O_RDONLY)) == -1)
		bomb("can't open %s", diskname);

	/*
	 * Find the disk type, allocate the read buffer.
	 */
	if (ioctl(diskf, DIOCGDINFO, &dl) == -1)
		bomb("can't read disk label from %s", diskname);
	disk_size = dl.d_partitions[RAWPARTITION - 'a'].p_size * dl.d_secsize;
	buflen = dl.d_secpercyl * dl.d_secsize;
	if (buflen == 0)
		sbomb("bad disk label");
	if ((buf = malloc((unsigned) buflen)) == NULL)
		bomb("can't allocate %d-byte buffer", buflen);

	/*
	 * Scan the disk.
	 */
	for (pos = 0; pos < disk_size; errno = 0) {
		for (; pos + buflen <= disk_size &&
		       (nbytes = read(diskf, buf, buflen)) == buflen; pos += buflen)
			if (pos % (buflen * 100) == 0)
				printf("cylinder %ld\n", pos / buflen);
			else if (vflag) {
				putchar('.');
				fflush(stdout);
			}
		if (pos + buflen <= disk_size) {
			if (nbytes >= 0)
				exit(0);
			if (vflag)
				complain("%d", pos / dl.d_secsize);
			if (errno == ENXIO)
				sbomb("%s: partition shorter than expected", diskname);
			if (errno != EIO)
				bomb("%s: unknown read error", diskname);
			if (lseek(diskf, pos, L_SET) == (off_t) -1)
				bomb("%s: seek failed", diskname);
			npos = pos + buflen;
		} else
			npos = disk_size;
		for (errno = 0; pos < npos; pos += dl.d_secsize)
			if (read(diskf, buf, dl.d_secsize) != dl.d_secsize) {
				if (errno != EIO)
					bomb("%s: unknown read error", diskname);
				printf("error sn %ld cyl %ld head %ld sec %ld\n",
					pos / dl.d_secsize,
					pos / buflen,
					(pos % buflen) /
						(dl.d_nsectors * dl.d_secsize),
					(pos / dl.d_secsize) % dl.d_nsectors);
				if (lseek(diskf, pos + dl.d_secsize, L_SET) == (off_t) -1)
					bomb("%s: seek failed", diskname);
				errno = 0;
			} else if (vflag) {
				putchar('+');
				fflush(stdout);
			}
	}

	return (0);
}
