/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: cdctl.c,v 1.1.1.1 1992/09/28 14:13:09 trent Exp $
 */

/*
 * This program provides a simple way to play audio from the cdrom,
 * as well as display the table of contents.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/cdefs.h>

#include <sys/cdrom.h>

static void print_toc __P((struct cdinfo *));
static void print_toc_hash __P((struct cdinfo *));

void
usage()
{
	fprintf(stderr, "usage: cdctl [options]\n");
	fprintf(stderr, "  -s num    start track num\n");
	fprintf(stderr, "  -e num    end track num\n");
	fprintf(stderr, "  -t        print table of contents\n");
	fprintf(stderr, "  -v        verbose table of contents\n");
	fprintf(stderr, "  -h        print hash code for disk\n");
	fprintf(stderr, "  -S        stop playing\n");
	fprintf(stderr, "  -w        hang until play done\n");
	fprintf(stderr, "  -p        print status every second\n");
	fprintf(stderr, "  -f dev    special device\n");
	exit(1);
}

int tflag;
int hflag;
int Sflag;
int wflag;
int pflag;
int vflag;

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct cdinfo *cdinfo;
	struct cdstatus status;
	struct track_info *tp;
	int i;
	extern char *optarg;
	int c;
	int start_track;
	int end_track;
	char *fname;
	struct track_info *stp, *etp;
	int minute, second, frame;

	fname = NULL;
	start_track = 0;
	end_track = 0;

	while ((c = getopt(argc, argv, "s:e:thSwpf:v")) != EOF) {
		switch (c) {
		case 's':
			start_track = atoi(optarg);
			break;
		case 'e':
			end_track = atoi(optarg);
			break;
		case 't':
			tflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'f':
			fname = optarg;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
		}
	}

	if ((cdinfo = cdopen(fname)) == NULL) {
		fprintf(stderr, "can't open cdrom\n");
		exit(1);
	}

	if (tflag) {
		print_toc(cdinfo);
		exit(0);
	}

	if (hflag) {
		print_toc_hash(cdinfo);
		exit(0);
	}

	if (Sflag) {
		cdstop(cdinfo);
		exit(0);
	}

	stp = NULL;
	etp = NULL;

	if (start_track == 0)
		stp = cdinfo->tracks;
	if (end_track == 0)
		etp = &cdinfo->tracks[cdinfo->ntracks - 1];

	for (i = 0, tp = cdinfo->tracks; i < cdinfo->ntracks; i++, tp++) {
		if (tp->track_num == start_track)
			stp = tp;
		if (tp->track_num == end_track)
			etp = tp;
	}

	if (stp == NULL) {
		fprintf(stderr, "can't find start track %d\n", start_track);
		exit(1);
	}

	if (etp == NULL) {
		fprintf(stderr, "can't find end track %d\n", end_track);
		exit(1);
	}

	cdplay(cdinfo, stp->start_frame, etp->start_frame + etp->nframes);

	if (pflag == 0 && wflag == 0)
		exit(0);

	while (1) {
		if (cdstatus(cdinfo, &status) < 0) {
			fprintf(stderr, "error in cdstatus\n");
			exit(1);
		}
		
		if (status.state != cdstate_playing)
			break;

		if (pflag) {
			printf("track %d ", status.track_num);
			printf("index %d ", status.index_num);

			frame_to_msf(status.rel_frame, &minute, &second,
			    &frame);
			printf("rel %02d:%02d:%02d ", minute, second, frame);

			frame_to_msf(status.abs_frame, &minute, &second,
			    &frame);
			printf("abs %02d:%02d:%02d ", minute, second, frame);
			printf("\n");

		}
		sleep(1);
	}

	return(0);
}

void
print_toc(cdinfo)
struct cdinfo *cdinfo;
{
	int i;
	struct track_info *tp;
	int minute, second, frame;

	for (i = 0, tp = cdinfo->tracks; i < cdinfo->ntracks; i++, tp++) {
		frame_to_msf(tp->nframes, &minute, &second, &frame);
		printf("%2d %02d:%02d:%02d", tp->track_num, minute, second,
		    frame);
		if (vflag)
			printf(" 0x%02x", tp->control);

		printf("\n");
	}
}

void
print_toc_hash(cdinfo)
struct cdinfo *cdinfo;
{
	int i;
	struct track_info *tp;
	struct track_hash_info {
		int	track_num;
		int	nframes;
		int	start_frame;
	} *track_hash_info, *thp;
	int track_hash_size;
	u_int crc;

	track_hash_size = cdinfo->ntracks * sizeof (struct track_hash_info);
	if ((track_hash_info = calloc(1, track_hash_size)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	for (i = 0, tp = cdinfo->tracks, thp = track_hash_info;
	     i < cdinfo->ntracks;
	     i++, tp++, thp++) {
		thp->track_num = tp->track_num;
		thp->nframes = tp->nframes;
		thp->start_frame = tp->start_frame;
	}

	crc = crcbuf(track_hash_info, track_hash_size);

	free(track_hash_info);

	printf("%d\n", crc);
}

