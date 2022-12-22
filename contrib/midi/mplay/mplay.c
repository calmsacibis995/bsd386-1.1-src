
/*
 * $Id: mplay.c,v 1.1.1.1 1992/08/14 00:14:33 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<string.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/ioctl.h>
#include	<i386/isa/midiioctl.h>
#include	<mutil.h>

/* global so we can use it in signal routines */
int Control;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	TRKS orig_tracks;
	TRKS spec_tracks;
	int got_file_name;
	int i;
	int mfile;
	int orig_arg;
	int repeat_first;
	int repeat_all;
	int verbose;
	unsigned char rel_tempo;
	unsigned char tempo;
	char file_name[256];
	char opt;
	void usage(char *);
	void abort_play();
	int process_file(int, TRKS *, int, int);

	verbose = 0;
	got_file_name = 0;
	repeat_all = 0;
	repeat_first = 0;
	tempo = 120;
	rel_tempo = 0x40;

	orig_tracks.num_tracks = 0;

	while ((opt = getopt(argc, argv, "vrRt:T:")) != EOF) {
		switch (opt) {
		case 'v':
			verbose = 1;
			set_midi_verbose(verbose);
			break;
		case 'r':
			repeat_first = 1;
			break;
		case 'R':
			repeat_all = 1;
			break;
		case 't':
			rel_tempo = double2tempo(atof(optarg));
			break;
		case 'T':
			if ((orig_tracks.num_tracks = get_range(optarg,
			    orig_tracks.tracks)) == -1) {
				(void) fprintf(stderr,
				    "Couldn't parse track list\n");
				exit(1);
			}
			break;
		case '?':
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	signal(SIGINT, abort_play);

	/* open control */
	if ((Control = open("/dev/midicntl", O_RDONLY, 0)) == -1) {
		perror("");
		(void) fprintf(stderr, "Couldn't open /dev/midicntl\n");
		exit(1);
	}

	if (ioctl(Control, MSETBASETMP, &tempo) == -1) {
		(void) fprintf(stderr, "Couldn't set tempo\n");
		return (0);
	}

	if (ioctl(Control, MSETRELTMP, &rel_tempo) == -1) {
		(void) fprintf(stderr, "Couldn't set relative tempo\n");
		exit(1);
	}

	if (ioctl(Control, MSTART, NULL) == -1) {
		(void) fprintf(stderr, "Couldn't start playing\n");
		exit(1);
	}

	if (argc - optind == 0) {
		if (repeat_all)
			(void)fprintf(stderr, "Cannot repeat all files "
			    "when playing from stdin\n");
		strcpy(file_name, "stdin");
		mfile = 0;
		i = 1;
		do {
			memcpy(&spec_tracks, &orig_tracks,
			    sizeof(orig_tracks));
			if (verbose)
				(void)printf("Processing file %d from stdin\n",
				    i++);
			if (!process_file(mfile, &spec_tracks, repeat_first,
			    verbose) && !MidiEof) {
				(void)fprintf(stderr, "Couldn't process "
				    "file from stdin\n");
				abort_play();
			}
		} while (!MidiEof);
		if (verbose)
			(void)printf("No file %d\n", i - 1);
	} else {
		orig_arg = optind;
		do {
			do {
				(void)memcpy(&spec_tracks, &orig_tracks,
				    sizeof(orig_tracks));
				strcpy(file_name, argv[optind++]);
				if ((mfile = open(file_name, O_RDONLY, 0))
				    == -1) {
					perror("");
					(void) fprintf(stderr,
					    "Couldn't open MIDI file %s\n",
					    file_name);
					abort_play();
				}
				if (verbose)
					(void)printf("Processing file %s\n",
					    file_name);
				if (!process_file(mfile, &spec_tracks,
				    repeat_first, verbose)) {
					(void)fprintf(stderr, "Couldn't "
					    "process file %s\n", file_name);
					close(mfile);
					abort_play();
				}
				close(mfile);
			} while (optind < argc);
			optind = orig_arg;
		} while (repeat_all);
	}

	if (ioctl(Control, MSTOP, NULL) == -1)
		(void) fprintf(stderr, "Couldn't stop play\n");

	close(Control);
	return (0);
}

int
process_file(mfile, spec_tracks, repeat, verbose)
	int mfile;
	TRKS *spec_tracks;
	int repeat;
	int verbose;
{
	HCHUNK hchunk;
	TCHUNK *tracks;
	long div_ioctls[] = {MRES192, MRES168, MRES144, MRES120, MRES96,
	    MRES72, MRES48};
	int divs[] = {192, 168, 144, 120, 96, 72, 48};
	int cond_on;
	int i;
	int num_open_trks;
	int track_devs[9];
	unsigned char atrks;
	int tempo_scalar;

	cond_on = 0;
	atrks = 0;

	/* read header chunk */
	if (!read_header_chunk(mfile, &hchunk)) {
		return (0);
	}

	if (verbose) {
		(void) printf("format = %d\n", hchunk.format);
		(void) printf("number of tracks = %d\n", hchunk.num_trks);
		(void) printf("division = %d\n", hchunk.division);
	}


	if (hchunk.division < 0) {
		(void) fprintf(stderr, "Can't currently handle SMPTE "
		    "time codes\n");
		return (0);
	}

	for (i = 0; i < sizeof(divs) / sizeof(divs[0]); i++) {
		if (hchunk.division % divs[i] == 0) {
			if (ioctl(Control, div_ioctls[i], NULL) == -1) {
				perror("");
				(void) fprintf(stderr,
				    "Couldn't set ticks / clock\n");
				return (0);
			}
			tempo_scalar = hchunk.division / divs[i];
			break;
		}
	}

	if (hchunk.division % divs[i] != 0) {
		(void) fprintf(stderr, "Bad ticks / beat value %d\n",
		    hchunk.division);
		(void) fprintf(stderr,
		    "Must be one of 48, 72, 96, 120, 144, 168, 192\n"
		    "Or an integer multiple thereof\n");
		return (0);
	}

	if (!(num_open_trks = load_tracks(mfile, &hchunk, &tracks,
	    spec_tracks))) {
		(void) fprintf(stderr, "Couldn't load tracks\n");
		return (0);
	}

	if ((cond_on = open_midi_devices(&hchunk, spec_tracks, &num_open_trks,
	    track_devs, &atrks)) == -1) {
		(void) fprintf(stderr, "Couldn't open MIDI devices\n");
		return (0);
	}

	if (ioctl(Control, MSELTRKS, &atrks) == -1) {
		(void) fprintf(stderr, "Couldn't select active play tracks\n");
		return (0);
	}

	if (cond_on) {
		if (ioctl(Control, MCONDON, NULL) == -1) {
			(void) fprintf(stderr,
			    "Couldn't select conductor track\n");
			return (0);
		}
	} else {
		if (ioctl(Control, MCONDOFF, NULL) == -1) {
			(void) fprintf(stderr,
			    "Couldn't unselect conductor track\n");
			return (0);
		}
	}


	if (ioctl(Control, MCLRPC, NULL) == -1) {
		(void) fprintf(stderr, "Couldn't clear play counters\n");
		return (0);
	}

	if (!play_tracks(tracks, track_devs, num_open_trks, tempo_scalar,
	    repeat)) {
		(void) fprintf(stderr, "Error playing tracks\n");
		return (0);
	}

	for (i = 0; i < num_open_trks; i++) {
		close(track_devs[i]);
		free_track(&tracks[i]);
	}
	free(tracks);
	return (1);
}

void
usage(program)
	char *program;
{
	(void) fprintf(stderr, "Usage: %s [-vrR] [-t rel_tempo] "
	    "[-T track_list] [midi_file ...]\n", program);
}

void
abort_play()
{
	
	if (ioctl(Control, MSTOP, NULL) == -1) {
		(void) fprintf(stderr, "Couldn't stop playing\n");
		exit(1);
	}
	sleep(1);
	close (Control);
	exit(0);
}
