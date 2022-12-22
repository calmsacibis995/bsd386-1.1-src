/*
 * $Id: minfo.c,v 1.1.1.1 1992/08/14 00:14:34 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <fcntl.h>
#include        <string.h>
#include        <unistd.h>
#include        <sys/types.h>
#include        <sys/time.h>
#include        <sys/ioctl.h>
#include        <i386/isa/midiioctl.h>
#include        "mutil.h"

long elapsed;
char Key[15][10] = {"C flat", "G flat", "D flat", "A flat", "E flat",
    "B flat", "F", "C", "G", "D", "A", "E", "B", "F sharp", "C sharp"};

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
	char file_name[256];
	char opt;
	void usage(char *);
	int process_file(int, TRKS *);

	got_file_name = 0;
        spec_tracks.num_tracks = 0;

	while ((opt = getopt(argc, argv, "T:")) != EOF) {
		switch (opt) {
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

	if (argc - optind == 0) {
		strcpy(file_name, "stdin");
		mfile = 0;
		i = 1;
		do {
			memcpy(&spec_tracks, &orig_tracks,
			    sizeof(orig_tracks));
			(void)printf("Processing file %d from stdin\n", i++);
			if (!process_file(mfile, &spec_tracks) && !MidiEof) {
				(void)fprintf(stderr, "Couldn't process "
				    "file from stdin\n");
			}
		} while (!MidiEof);
		(void)printf("No file %d\n", i - 1);
	} else {
		do {
			(void)memcpy(&spec_tracks, &orig_tracks,
			    sizeof(orig_tracks));
			strcpy(file_name, argv[optind++]);
			if ((mfile = open(file_name, O_RDONLY, 0)) == -1) {
				perror("");
				(void) fprintf(stderr, "Couldn't open MIDI "
				    "file %s\n", file_name);
			}
			(void)printf("Processing file %s\n", file_name);
			if (!process_file(mfile, &spec_tracks)) {
				(void)fprintf(stderr, "Couldn't process "
				    "file %s\n", file_name);
				close(mfile);
			}
			close(mfile);
		} while (optind < argc);
	}

        return (0);
}

int
process_file(mfile, spec_tracks)
	int mfile;
	TRKS *spec_tracks;
{
	HCHUNK hchunk;
	TCHUNK *tracks;
	int event_size;
	int i;
	int num_open_trks;
	EVENT_TYPE event_type;
	unsigned char event[256];
	void print_event_info(unsigned char *, int, EVENT_TYPE);

	/* read header chunk */
	if (!read_header_chunk(mfile, &hchunk)) {
		return (0);
	}

	(void) printf("format = %d\n", hchunk.format);
	(void) printf("number of tracks = %d\n", hchunk.num_trks);
	(void) printf("division = %d\n", hchunk.division);

	if (!(num_open_trks = load_tracks(mfile, &hchunk, &tracks,
	    spec_tracks))) {
		(void) fprintf(stderr, "Couldn't load tracks\n");
		return (0);
	}

	for (i = 0; i < num_open_trks; i++) {
		elapsed = 0;
		printf("\nTrack %d\n", spec_tracks->tracks[i]);
		for (;;) {
			event_size = get_smf_event(&tracks[i], event,
			    &event_type);
			if (event_size == -1) {
				(void) fprintf(stderr, "Error reading event\n");
				return (0);
			} else if (event_size > 0) {
				printf("Track %d: ", spec_tracks->tracks[i]);
				print_event_info(event, event_size,
				    event_type);
				printf("\n");
			} else
				break;
		}
	}


	for (i = 0; i < num_open_trks; i++) {
		free_track(&tracks[i]);
	}
	free(tracks);
	return (1);
}


void
print_event_info(event, event_size, event_type)
	unsigned char *event;
	int event_size;
	EVENT_TYPE event_type;
{
	int adj;
	int denom;
	int i;
	int us;
	void print_normal_event(unsigned char *);
	void print_text(unsigned char *);

	elapsed += var2fix(event, &adj);
	event += adj;

	printf("(%ld) ", elapsed);


	switch (event_type) {
	case NORMAL:
		print_normal_event(event);
		break;
	case SYSEX:
		printf("SYSEX");
		for (i = 0; i < event_size; i++)
			printf(" 0x%02x", *event++);
		break;
	case METASEQNUM:
		printf("METASEQNUM");
		event += 3;
		printf(" %d", event[0] * 0x0100 + event[1]);
		break;
	case METATEXT:
		printf("METATEXT");
		print_text(event);
		break;
	case METACPY:
		printf("METATEXT");
		print_text(event);
		break;
	case METASEQNAME:
		printf("METASEQNAME");
		print_text(event);
		break;
	case METAINSTNAME:
		printf("METAINSTNAME");
		print_text(event);
		break;
	case METALYRIC:
		printf("METALYRIC");
		print_text(event);
		break;
	case METAMARKER:
		printf("METAMARKER");
		print_text(event);
		break;
	case METACUE:
		printf("METACUE");
		print_text(event);
		break;
	case METACHANPREFIX:
		printf("METACHANPREFIX");
		print_text(event);
		break;
	case METAEOT:
		printf("METAEOT");
		break;
	case METATEMPO:
		printf("METATEMPO");
		us = event[3] * 0x010000 + event[4] * 0x0100 + event[5];
		printf(" %d us per beat %d BPM", us, 60 * 1000000 / us);
		break;
	case METASMPTE:
		printf("METASMPTE");
		printf(" Hours %d, Minutes %d, Seconds %d, Frames %d, "
		    "1/100s Frames %d", event[3], event[4], event[5],
		    event[6], event[7]);
		break;
	case METATIME:
		printf("METATIME");
		for (denom = 1, i = 0; i < event[4]; i++, denom *= 2);
		printf(" %d / %d, %d clock per met. click, %d 32nd notes "
		    "per 1/4 note", event[3], denom, event[5],
		    event[6]);
		break;
	case METAKEY:
		printf("METAKEY");
		printf(" %s %s", Key[(signed char)event[3] + 7],
		    event[4] ? "minor" : "major");
		break;
	case METASEQSPEC:
		printf("METASEQSPEC");
		for (i = 3; i < event_size; i++)
			printf(" 0x%02x", event++);
		break;
	}
}

void
print_normal_event(event)
	unsigned char *event;
{
	static int channel;
	static int r_state;

	if (event[0] > 0xf0) {
		switch (event[0]) {
		case 0xf2:
			printf("Song Position %d", event[1] + (event[2] & 0x01)
			    * 0x80 + (event[2] >> 1) * 0x0100);
			break;
		case 0xf3:
			printf("Song Select %d", event[1]);
			break;
		case 0xf6:
			printf("Tune Request");
			break;
		case 0xf7:
			printf("EOX (Terminator)");
			break;
		case 0xf8:
			printf("NO-OP");
			break;
		default:
			printf("Undefined");
		}
		return;
	} else if (event[0] > 0x7f) {
		channel = event[0] & 0x0f;
		printf("Channel %d - ", channel);
		r_state = (event[0] >> 4) & 0x0f;
		event++;
	} else {
		printf("Channel %d - ", channel);
	}

	switch (r_state) {
	case 0x08:
		printf("Note Off, Pitch %d, Velocity %d", event[0], event[1]);
		break;
	case 0x09:
		printf("Note On, Pitch %d, Velocity %d", event[0], event[1]);
		break;
	case 0x0a:
		printf("Key Pressure, Pitch %d, Pressure %d", event[0],
		    event[1]);
		break;
	case 0x0b:
		printf("Parameter, Parameter %d, Value %d", event[0],
		    event[1]);
		break;
	case 0x0c:
		printf("Program, Value %d", event[0]);
		break;
	case 0x0d:
		printf("Channel Pressure, Pressure %d", event[0]);
		break;
	case 0x0e:
		printf("Pitch Wheel, Value %d", event[0] + (event[1] & 0x01)
		    *0x80 + (event[1] >> 1) * 0x0100);
	}
}
	

void
print_text(event)
	unsigned char *event;
{
	int i;
	int len;

	len = event[2];
	event += 3;
	printf(" ");
	for (i = 0; i < len; i++)
		printf("%c", *event++);
}

void
usage(program)
	char *program;
{
        (void) fprintf(stderr, "%s [ -T track-list ] [ midi_file ]\n",
	    program);
}
