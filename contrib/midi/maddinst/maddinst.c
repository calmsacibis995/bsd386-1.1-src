
/*
 * $Id: maddinst.c,v 1.1.1.1 1992/08/14 00:14:32 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <i386/isa/midiioctl.h>
#include <mutil.h>

typedef struct {
	int	track;
	unsigned	char channel;
	unsigned	char inst;
} INST;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	HCHUNK hchunk;
	INST insts[16];
	TCHUNK *tracks;
	TRKS spec_tracks;
	FILE *tfile;
	int cond_on;
	int got_file_name;
	int mfile;
	int num_insts;
	int num_trks;
	int verbose;
	char file_name[256];
	char opt;

	void usage(char *);
	int read_inst_data(FILE *, INST *);
	int change_insts(TCHUNK *, TRKS *, INST *, int);
	int write_new_tracks(HCHUNK *, TCHUNK *);

	verbose = 0;
	got_file_name = 0;
	cond_on = 0;
	tfile = NULL;

	spec_tracks.num_tracks = 0;

	while ((opt = getopt(argc, argv, "i:")) != EOF) {
		switch (opt) {
		case 'i':
			if ((tfile = fopen(optarg, "r")) == NULL) {
				perror("");
				fprintf(stderr, "Couldn't open %s\n",
				    optarg);
				exit(1);
			}
			break;
		case '?':
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	if (argc == 2 || argc == 3) {
		if ((tfile = fopen(argv[1], "r")) == NULL) {
			perror("");
			fprintf(stderr, "Couldn't open %s\n", argv[1]);
			exit(1);
		}
	} else {
		usage(argv[0]);
		exit(1);
	}


	if (argc == 2) {
		strcpy(file_name, "stdin");
		mfile = 0;
	} else {
		strcpy(file_name, argv[optind]);
		if ((mfile = open(file_name, O_RDONLY, 0)) == -1) {
			perror("");
			(void) fprintf(stderr, "Couldn't open MIDI file %s\n",
			    file_name);
			exit(1);
		}
	}


	/* read header chunk */
	if (!read_header_chunk(mfile, &hchunk)) {
		exit(1);
	}

	if (verbose) {
		(void) printf("format = %d\n", hchunk.format);
		(void) printf("number of tracks = %d\n", hchunk.num_trks);
		(void) printf("division = %d\n", hchunk.division);
	}

	if ((num_insts = read_inst_data(tfile, insts)) == -1) {
		(void) fprintf(stderr, "Couldn't read instrument data\n");
		exit(1);
	}

	if (!(num_trks = load_tracks(mfile, &hchunk, &tracks,
	    &spec_tracks))) {
		(void) fprintf(stderr, "Couldn't load tracks\n");
		exit(1);
	}

	if (!change_insts(tracks, &spec_tracks, insts, num_insts)) {
		(void) fprintf(stderr, "Coudln't change instruments\n");
		exit(1);
	}

	if (!write_header_chunk(1, &hchunk)) {
		(void) fprintf(stderr, "Coudln't write header chunk\n");
		exit(1);
	}

	if (!write_new_tracks(&hchunk, tracks)) {
		(void) fprintf(stderr, "Couldn't write modified tracks\n");
		exit(1);
	}

	return (0);
}

int
read_inst_data(file, insts)
	FILE *file;
	INST *insts;
{
	int i;
	int line_num;
	int num_read;
	char line[256];

	i = 0;
	line_num = 0;
	for(;;) {
		if (fgets(line, sizeof(line), file) == NULL) {
			if (feof(file))
				break;
			else {
				perror("");
				fprintf(stderr, "Couldn't read isntrument "
				    "file, pass %d\n", line_num);
				return (-1);
			}
		}
		line_num++;
		if (line[0] == '#')
			continue;
		else {
			if ((num_read = sscanf(line, "%d %d %d",
			    &(insts[i].track), &(insts[i].channel),
			    &(insts[i].inst))) != 3) {
				perror("");
				fprintf(stderr, "Bad data format - line %d\n",
				    line_num);
				return (-1);
			}
			i++;
		}
	}
	return (i);
}

int
change_insts(tracks, spec_tracks, insts, num_insts)
	TCHUNK *tracks;
	TRKS *spec_tracks;
	INST *insts;
	int num_insts;
{
	TCHUNK new_track;
	int event_size;
	int i;
	int j;
	EVENT_TYPE event_type;
	unsigned char event[256];
	unsigned char new_event[8];

	strncpy(new_track.str, "MTrk", 4);
	j = 0;
	for (i = 0; i < spec_tracks->num_tracks; i++) {
		if (j == num_insts || insts[j].track != i)
			continue;

		new_track.pos = 0;
		new_track.length = 0;
		new_track.msize = 0;
		new_track.events = NULL;
		new_track.event_start = NULL;

		for(;;) {
			event_size = get_smf_event(&tracks[i], event,
			    &event_type);
			if (event_size == -1) {
				fprintf(stderr, "Error reading event\n");
				return (0);
			} else if (event_size == 0) {
				break;
			}
			if (event[1] != 0xff || event[0] != 0 ||
			    event_size == 0)
				break;
			if (event[1] != (0xc0 + insts[j].channel)) {
				if (!put_smf_event(&new_track, event,
				    event_size)) {
					(void) fprintf(stderr,
					    "Couldn't copy event\n");
					return (0);
				}
			}
		}

		if (event_size == 0)
			continue;

		/* create program selection event */
		/* timing */
		new_event[0] = 0;
		/* command */
		new_event[1] = 0xc0 + insts[j].channel;
		/* voice */
		new_event[2] = insts[j].inst;
		if (!put_smf_event(&new_track, new_event, 3)) {
			(void)fprintf(stderr, "Couldn't add new instrument\n");
			return (0);
		}
		if (!put_smf_event(&new_track, event, event_size)) {
			(void)fprintf(stderr, "Couldn't copy event\n");
			return (0);
		}

		/* copy rest of track */
		do {
			event_size = get_smf_event(&tracks[i], event,
			    &event_type);
			switch (event_size) {
			case -1:
				fprintf(stderr, "Error reading event\n");
				return (0);
			case 0:
				break;
			default:
				if (event[1] != (0xc0 +
				    insts[i].channel)) {
					if (!put_smf_event(&new_track,
					    event, event_size)) {
						(void) fprintf(stderr,
						    "Couldn't copy event\n");
						return (0);
					}
				}
			}
		} while (event_size != 0);

		free_track(&tracks[i]);
		memcpy((char *)&tracks[i], (char *)&new_track,
		    sizeof(TCHUNK));
		j++;
	}

	return (1);
}

int
write_new_tracks(hchunk, tracks)
	HCHUNK *hchunk;
	TCHUNK *tracks;
{
	int i;

	for (i = 0; i < hchunk->num_trks; i++) {
		if (!write_track_chunk(1, &tracks[i])) {
			(void) fprintf(stderr, "Couldn't write track %d\n", i);
			fprintf(stderr, "total tracks = %d\n", hchunk->num_trks);
			return (0);
		}
	}
	return (1);
}

void
usage(program)
	char *program;
{
	(void) fprintf(stderr,
	    "Usage: %s instrument_file [midi_file]\n", program);
}
