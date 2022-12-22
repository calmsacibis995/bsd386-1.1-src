
/*
 * $Id: mplayutil.c,v 1.1.1.1 1992/08/14 00:14:35 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/file.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/ioctl.h>
#include	<i386/isa/midiioctl.h>
#include	"mutil.h"

#define MAX_TRACKS 8

/* global */
double TempoRatio = 1.0;	/* this changes when clocks_per_met change */
int StopRecording;
int Verbose;
int Tempo = 120;
char Key[15][10] = {"C flat", "G flat", "D flat", "A flat", "E flat",
    "B flat", "F", "C", "G", "D", "A", "E", "B", "F sharp", "C sharp"};

int
play_tracks(tracks, devs, num, t_scalar, repeat)
	TCHUNK *tracks;
	int *devs;
	int num;
	int t_scalar;
	int repeat;
{

	return (record_tracks(tracks, devs, num, NULL, -1, t_scalar, repeat));
}

int
record_tracks(p_tracks, p_devs, p_num, r_track, r_dev, t_scalar, repeat)
	TCHUNK *p_tracks;
	int *p_devs;
	int p_num;
	TCHUNK *r_track;
	int r_dev;
	int t_scalar;
	int repeat;
{
	struct timeval mwait;
	fd_set w_select;
	fd_set r_select;
	int click_fraction[16];
	int i;
	int num_active;
	int select_return;
	int something_open;
	int *track_active;

	num_active = p_num;
	if (p_num) {
		if ((track_active = (int *) malloc(sizeof(int) * p_num))
		    == NULL)
			return (0);
		for (i = 0; i < p_num; i++)
			track_active[i] = 1;
	}

	for (i = 0; i < 16; i++)
		click_fraction[i] = 0;
	something_open = 1;
	StopRecording = 0;
	do {
		while (something_open && !StopRecording) {
			/* select on open play tracks */
			FD_ZERO(&w_select);
			for (i = 0; i < p_num; i++)
				if (track_active[i])
					FD_SET(p_devs[i], &w_select);
			FD_ZERO(&r_select);
			if (r_dev != -1)
				FD_SET(r_dev, &r_select);
			mwait.tv_sec = 0;
			mwait.tv_usec = 100000;
			if ((select_return = select(getdtablesize(), &r_select,
			    &w_select, NULL, &mwait)) == -1) {
				if (errno == EINTR)
					break;
				else {
					perror("");
					(void) fprintf(stderr,
					    "Error in select\n");
					return (0);
				}
			}
			if (select_return == 0)
				continue;
	
			/* write event to non-blocked one */
			for (i = 0; i < p_num && !StopRecording; i++) {
				if (!track_active[i] || !FD_ISSET(p_devs[i],
				    &w_select))
					continue;

				something_open = process_play_event(i,
				    &p_tracks[i], p_devs[i], &track_active[i],
				    &num_active, t_scalar);
				if (!track_active[i] && repeat) {
					rewind_track(&p_tracks[i]);
					track_active[i] = 1;
					num_active++;
					something_open = 1;
				}
			}
			if (r_dev != -1 && FD_ISSET(r_dev, &r_select)) {

				if (!process_record_event(r_dev, r_track))
					return (0);
			}
		}
	} while (repeat && !StopRecording);

	if (!StopRecording) {
		for (i = 0; i < p_num; i++)
			if (ioctl(p_devs[i], MFLUSHQ, NULL) == -1) {
				(void) fprintf(stderr,
				    "Error flushing queue for track %d\n", i);
				perror("");
			}
	} else {
		/* flush any delays stuck in board2smf */
/*
		flush_event(r_track);
*/
	}
	if (p_num)
		free(track_active);
	return (1);
}

int
process_play_event(track_num, p_track, p_dev, track_active, num_active,
    t_scalar)
	int track_num;
	TCHUNK *p_track;
	int p_dev;
	int *track_active;
	int *num_active;
	int t_scalar;
{
	EVENT_TYPE event_type;
	static int need_event[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; 
	int event_size;
	int smf_size;
	int something_open;
	int valid_event;
	unsigned char smf_event[2048];
	unsigned char event[2048];

	something_open = 1;
	valid_event = 0;
	do {
		if (!need_event[track_num])
			event_size = smf2board(track_num, NULL, smf_size,
			    event, t_scalar, &need_event[track_num],
			    &event_type);
		else {
			smf_size = get_smf_event(p_track, smf_event,
			    &event_type);
			if (smf_size == -1) {
				(void) fprintf(stderr,
				    "Error reading event\n");
				return (0);
			} else if (smf_size == 0) {
				*track_active = 0;
				valid_event = 1;
				if (--(*num_active) == 0)
					something_open = 0;
				break;
			}
			event_size = smf2board(track_num, smf_event, smf_size,
			    event, t_scalar, &need_event[track_num],
			    &event_type);
		}

/*
printf("track %d: ", track_num);
for (j = 0; j < event_size; j++)
printf("0x%02x ", event[j]);
printf("\n");
fflush(stdout);
*/
		switch(send_event(p_dev, event, event_size,
		    event_type, track_num == 0)) {
		case SEND_ERROR:
			return (0);
		case SEND_VALID:
			valid_event = 1;
			break;
		case SEND_INVALID:
			valid_event = 0;
			break;
		case SEND_DONE:
			*track_active = 0;
			valid_event = 1;
			if (--(*num_active) == 0)
				something_open = 0;
			break;
		}
	} while (!valid_event && !StopRecording);
	return(something_open);
}

int
process_record_event(dev, r_track)
	int dev;
	TCHUNK *r_track;
{
	int event_size;
	int events_delta;
	int events_pos;
	int events_size;
	/*
	int i;
	*/
	int smf_size;
	unsigned char events[256];
	unsigned char event_data[256];
	unsigned char smf_event[256];
	unsigned char *events_ptr;

	if ((events_size = read(dev, events, 256)) == -1) {
		perror("");
		(void) fprintf(stderr, "Couldn't read from MIDI board\n");
		return (0);
	}
/*
printf("\n");
printf("board:");
for (i = 0; i < events_size; i++)
printf(" 0x%02x", events[i]);
printf("\n");
*/
	events_ptr = events;
	events_pos = 0;
	do {
		if((events_delta = get_board_event(events_ptr, events_size,
		    event_data, &event_size)) == -1) {
			(void) fprintf(stderr, "Couldn't get board event "
			    "from stream\n");
			return (0);
		}
		events_ptr += events_delta;
		events_pos += events_delta;
/*
printf("single event:");
for (i = 0; i < event_size; i++)
printf(" 0x%02x", event_data[i]);
printf("\n");
*/

		board2smf(event_data, event_size, smf_event, &smf_size);
/*
printf("smf:");
for (i = 0; i < smf_size; i++)
printf(" 0x%02x", smf_event[i]);
printf("\n");
*/
		if (smf_size > 0)
			if (!put_smf_event(r_track, smf_event, smf_size)) {
				(void) fprintf(stderr, "Couldn't put event "
				    "in record track\n");
				return (0);
			}
	} while (events_pos < events_size);
	return(1);
}

void
flush_event(r_track)
	TCHUNK *r_track;
{
	int smf_size;
	unsigned char flush_event[] = {0x00, 0xf8};
	unsigned char smf_event[256];

	board2smf(flush_event, sizeof(flush_event), smf_event, &smf_size);

	if (!put_smf_event(r_track, smf_event, smf_size)) {
		(void)fprintf(stderr, "Couldn't put flush event\n");
	}
}

void
stop_recording()
{
	StopRecording = 1;
}


SEND_VAL send_event(dev, event, event_size, event_type, conductor)
	int dev;
	unsigned char *event;
	int event_size;
	EVENT_TYPE event_type;
	int conductor;
{
	SEND_VAL return_val;
	static int clocks_per_beat = 24;
	int num_written;
	short seq_num;
	unsigned char time_event[10];

	switch (event_type) {
	case NORMAL:
		if ((num_written = write(dev, event, event_size)) == -1) {
			perror("NORMAL");
			(void) fprintf(stderr, "Couldn't write to MIDI "
			    "board\n");
			return_val = SEND_ERROR;
			break;
		}
		return_val = SEND_VALID;
		break;
	case METASEQNUM:
		seq_num = event[4] << 8 + event[5];
		if (Verbose)
			printf("Sequence number %d\n", seq_num);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METASEQNUM");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METATEXT:
		event[event_size] = '\0';
		if (Verbose)
			(void) printf("METATEXT: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METATEXT");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METACPY:
		event[event_size] = '\0';
		if (Verbose)
			printf("METACPY: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METACPY");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METASEQNAME:
		event[event_size] = '\0';
		if (Verbose)
			printf("METASEQNAME: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METASEQNAME");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METAINSTNAME:
		event[event_size] = '\0';
		if (Verbose)
			printf("METAINSTNAME: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METAINSTNAME");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METALYRIC:
		event[event_size] = '\0';
		if (Verbose)
			(void) printf("METALYRIC: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METALYRIC");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METAMARKER:
		event[event_size] = '\0';
		if (Verbose)
			printf("METAMARKER: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METAMARKER");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METACUE:
		event[event_size] = '\0';
		if (Verbose)
			printf("METACUE: %s\n", &event[4]);
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METACUE");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METACHANPREFIX:
		/* I have no data on how this is formatted */
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METACHANPREFIX");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METAEOT:
		return_val = SEND_DONE;
		break;
	case METATEMPO:
		/* only do this for the conductor track - pos 0 */
		if (conductor) {
			/* just the byte meaningful to the board */
			event[1] = (unsigned char) (MSETBASETMP & 0xff);
			/* convert usec / beat to beats / min */
			event[2] = TempoRatio * (60 * 1000000) /
			    (event[4] * 0x010000 + event[5] * 0x0100 +
			    event[6]);
			Tempo = event[2];
			if (Verbose)
				printf("METATEMPO: Beats Per Minute: %d\n",
				    event[2]);
			if (write(dev, event, 3) == -1) {
				perror("METATEMPO");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		} else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METATEMPO");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METASMPTE:
		/* Not sure on this one */
		if (Verbose) {
			printf("METASMPTE: Hour %d Minute %d Second %d "
			    "Frame %d Fractional Frame %d\n",
			    event[4], event[5], event[6], event[7],
			    event[8]);
		}
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METASMPTE");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METATIME:
		if (conductor) {
			if (Verbose)
				printf("METATIME: %d / %d "
				    "Clocks per met beat = %d\n",
				    event[4], event[5], event[6]);
			time_event[0] = event[0];
			time_event[1] = (unsigned char) (MSETBPM & 0xff);
			/* bpm = numerator of time sig */
			time_event[2] = event[4];
			/* no time delay */
			time_event[3] = 0;
			time_event[4] = (unsigned char) (MSETMETFRQ & 0xff);
			/* met freq = byte seven in time sig */
			time_event[5] = event[6];
			/*
			 * if the clocks per met changes we should adjust
			 * the tempo accordingly - at least until I figure
			 * out a better way of doing this.
			 */
			/* new clock_per_bear over old */
			TempoRatio = (double)event[6] / clocks_per_beat;
			Tempo = Tempo * TempoRatio;
			time_event[6] = 0;
			time_event[7] = (unsigned char) (MSETBASETMP & 0xff);
			time_event[8] = Tempo;
			if (write(dev, time_event, 9) == -1) {
				perror("METATIME");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			clocks_per_beat = event[6];
			return_val = SEND_VALID;
		} else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METATIME");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	case METAKEY:
		if (Verbose)
			printf("METAKEY: %s %s\n", Key[(signed char)event[4]
			    + 7], event[5] ? "minor" : "major");
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("METAKEY");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	default:
		if (event[0] == 0)
			return_val = SEND_INVALID;
		else {
			event[1] = MIDI_NOOP;
			if (write(dev, event, 2) == -1) {
				perror("default");
				(void) fprintf(stderr, "Couldn't write to "
				    "MIDI board\n");
				return_val = SEND_ERROR;
				break;
			}
			return_val = SEND_VALID;
		}
		break;
	}

	return (return_val);
}

int
open_midi_devices(hchunk, spec_tracks, num, devs, active_mask)
	HCHUNK *hchunk;
	TRKS *spec_tracks;
	int *num;
	int *devs;
	unsigned char *active_mask;
{
	int cond_on;
	int dev_counter;
	int i;
	int j;
	char dev_name[100];

	cond_on = 0;
	j = 0;
	if (hchunk->format == 0) {
		if ((devs[j++] = open("/dev/midicond", O_WRONLY | O_NONBLOCK,
		    0)) == -1) {
			(void) fprintf(stderr,
			    "Coudln't open conductor track\n");
			return (-1);
		}
		cond_on = 1;
	}

	for (i = 0; i < spec_tracks->num_tracks && !cond_on; i++)
		if (spec_tracks->tracks[i] == 0) {
			if ((devs[j++] = open("/dev/midicond", O_WRONLY |
				O_NONBLOCK, 0)) == -1) {
				(void) fprintf(stderr,
				    "Coudln't open conductor track\n");
				return (-1);
			}
			cond_on = 1;
		}

	dev_counter = 0;
	*active_mask = 0;
	for (i = j; i < *num; i++) {
		if (dev_counter == MAX_TRACKS) {
			(void) fprintf(stderr, "Too many play tracks (%d).  "
			   "Only using first %d\n", *num - cond_on, MAX_TRACKS);
			*num = MAX_TRACKS + cond_on;
			break;
		}

		(void) sprintf(dev_name, "/dev/midi%d", dev_counter);
		if ((devs[j++] = open(dev_name, O_WRONLY | O_NONBLOCK, 0))
		    == -1) {
			(void) fprintf(stderr, "Coudln't open midi track %d\n",
				dev_counter);
			return (-1);
		}
		*active_mask |= (unsigned char)(1 << dev_counter++);
	}

	return (cond_on);
}

void
set_midi_verbose(v)
	int v;
{
	Verbose = v;
}
