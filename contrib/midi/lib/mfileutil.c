
/*
 * $Id: mfileutil.c,v 1.1.1.1 1992/08/14 00:14:35 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <unistd.h>
#include        <string.h>
#include        <sys/fcntl.h>
#include        <sys/ioctl.h>
#include        <i386/isa/midiioctl.h>
#include        "mutil.h"

#define		MAX_TIME	0xef

/* hack so we know if a read failed because of an eof instead of bad data */
int MidiEof = 0;

int
read_header_chunk(mfile, hchunk)
	int mfile;
	HCHUNK *hchunk;
{

	if (mread(mfile, hchunk->str, sizeof(hchunk->str)) !=
	    sizeof(hchunk->str)) {
		if (!MidiEof)
			(void) fprintf(stderr, "Couldn't read header chunk "
			     "identifier\n");
		return (0);
	}

	if (mread(mfile, (char *)&hchunk->length, sizeof(hchunk->length)) !=
	    sizeof(hchunk->length)) {
		(void) fprintf(stderr, "Couldn't read header chunk length\n");
		return (0);
	}

	if (mread(mfile, (char *)&hchunk->format, sizeof(hchunk->format)) !=
	    sizeof(hchunk->format)) {
		(void) fprintf(stderr, "Couldn't read header chunk format\n");
		return (0);
	}

	if (mread(mfile, (char *)&hchunk->num_trks, sizeof(hchunk->num_trks))
	    != sizeof(hchunk->num_trks)) {
		(void) fprintf(stderr, "Couldn't read header chunk "
		    "num_trks\n");
		return (0);
	}

	if (mread(mfile, (char *)&hchunk->division, sizeof(hchunk->division))
	    != sizeof(hchunk->division)) {
		(void) fprintf(stderr, "Couldn't read header chunk "
		    "division\n");
		return (0);
	}

	/* fix byte ordering */
	hchunk->length = mtohl(hchunk->length);
	hchunk->format = mtohs(hchunk->format);
	hchunk->num_trks = mtohs(hchunk->num_trks);
	hchunk->division = mtohs(hchunk->division);

	if (strncmp(hchunk->str, "MThd", 4) != 0) {
		(void) fprintf(stderr, "Not a standard MIDI file\n");
		return (0);
	}

	if (hchunk->length != 6) {
		(void) fprintf(stderr, "Bad header chunk size. Is 0x%lx "
		    "should be 6\n", hchunk->length);
		return (0);
	}

	if (hchunk->format == 0 && hchunk->num_trks != 1) {
		(void) fprintf(stderr, "Header format says single track, "
		    "but number of tracks is set to %d\n", hchunk->num_trks);
		return (0);
	}

	return (1);
}

int
read_track_chunk(mfile, tracks)
	int mfile;
	TCHUNK *tracks;
{

	if (mread(mfile, tracks->str, 4) != 4) {
		(void) fprintf(stderr, "Couldn't read track chunk "
		     "identifier\n");
		return (0);
	}

	if (strncmp(tracks->str, "MTrk", 4) != 0) {
		(void) fprintf(stderr, "Bad track chunk identifier\n");
		(void) fprintf(stderr, "tracks->str = %s\n", tracks->str);
		return (0);
	}

	if (mread(mfile, (char *)&tracks->length, sizeof(tracks->length)) !=
	    sizeof(tracks->length)) {
		(void) fprintf(stderr, "Couldn't read track length\n");
		return (0);
	}

	tracks->msize = tracks->length = mtohl(tracks->length);
	tracks->pos = 0;

	/* allocate space for tracks events */
	if ((tracks->events = (unsigned char *) malloc(tracks->length)) ==
	    NULL) {
		(void) fprintf(stderr, "Not enough memory for track data\n");
		return (0);
	}
	tracks->event_start = tracks->events;

	if (mread(mfile, (char *)tracks->events, tracks->length) !=
	    tracks->length) {
			(void) fprintf(stderr, "Couldn't read track data\n");
			return (0);
	}
	tracks->events = tracks->event_start;

	return (1);
}

int
skip_track_chunk(mfile)
	int mfile;
{
	long length;
	char str[4];

	if (mread(mfile, str, sizeof(str)) != sizeof(str)) {
		(void) fprintf(stderr, "Couldn't read track chunk "
		    "identifier\n");
		return (0);
	}

	if (strncmp(str, "MTrk", 4) != 0) {
		(void) fprintf(stderr, "Bad track chunk identifier\n");
		(void) fprintf(stderr, "str = %s\n", str);
		return (0);
	}

	if (mread(mfile, (char *)&length, sizeof(length)) != sizeof(length)) {
		(void) fprintf(stderr, "Couldn't read track length\n");
		return (0);
	}

	length = mtohl(length);

	if (lseek(mfile, length, SEEK_CUR) == -1) {
		(void) fprintf(stderr, "Couldn't seek past track\n");
		return (0);
	}

	return (1);
}

int
get_smf_event(track, event, event_type)
	TCHUNK *track;
	unsigned char *event;
	EVENT_TYPE *event_type;
{
	long length = 0;
	int i;
	int size;
	static int extra_bytes[] = {2, 2, 2, 2, 1, 1, 2, 0};
	unsigned char etype;
	static unsigned char r_state;


			
	if (track->pos >= track->length)
		return (0);

	/*
	 * get timing bytes
	 */
	if (track->pos++ == track->length)
		return (-1);
	*event++ = *track->events++;
	size = 1;
	while (*(event - 1) & 0x80) {
		if (track->pos++ == track->length)
			return (-1);
		*event++ = *track->events++;
		size++;
	}

	if (track->pos++ == track->length)
		return (-1);
	*event = *track->events++;

	/* get event type */
	switch (*event) {
	case 0xff:
		/* meta event */
		if (track->pos++ == track->length)
			return (-1);
		size++;
		event++;
		*event_type = *event++ = *track->events++;
		size++;
		/* get length as a variable length */
		do {
			if (track->pos++ == track->length)
				return (-1);
			length = (length << 7) + (*track->events & 0x7f);
			*event = *track->events++;
			size++;
		} while (*event++ & 0x80);
		for (; length > 0; length--) {
			/* get event */
			if (track->pos++ == track->length)
				return (-1);
			*event++ = *track->events++;
			size++;
		}
		break;
	case 0xf0:
	case 0xf7:
		event++;
		size++;
		*event_type = SYSEX;
		do {
			if (track->pos++ == track->length)
				return (-1);
			*event = *track->events++;
			size++;
		} while (*event++ != 0xf7);
		break;
	default:
		*event_type = NORMAL;
		size++;
		if (*event & 0x80) {
			etype = *event;
			r_state = (*event >> 4) & 0x0f;
			for (i = 0; i < extra_bytes[(etype >> 4) & 0x07];
			    i++) {
				event++;
				if (track->pos++ == track->length)
					return(-1);
				*event = *track->events++;
				size++;
			}
		} else {
			/* assume it is a continuation of note on */
			/* get other data byte */
			switch (r_state) {
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0e:
				event++;
				if (track->pos++ == track->length)
					return (-1);
				*event = *track->events++;
				size++;
				break;
			default:
				break;
			}
		}
	}
	return (size);
}

int
put_smf_event(track, event, event_size)
	TCHUNK *track;
	unsigned char *event;
	int event_size;
{

	track->length += event_size;
	if (track->pos + event_size > track->msize) {
		track->msize += BUFSIZ;
		if ((track->event_start = realloc(track->event_start,
		    track->msize)) == NULL) {
			perror("");
			(void) fprintf(stderr, "Not enough memory for new "
			    "event\n");
			track->length -= event_size;
			track->msize -= BUFSIZ;
			return (0);
		}
		/* starting position might move */
		track->events = track->event_start + track->pos;
	}

	memcpy(track->events, event, event_size);
	track->events += event_size;
	track->pos += event_size;
	return (1);
}

void
rewind_track(track)
	TCHUNK *track;
{
	track->pos = 0;
	track->events = track->event_start;
}

void
reset_track(track)
	TCHUNK *track;
{
	track->pos = 0;
	track->length = 0;
	track->events = track->event_start;
}


int
write_header_chunk(mfile, hchunk)
	int mfile;
	HCHUNK *hchunk;
{

	(void)strncpy(hchunk->str, "MThd", 4);
	if (mwrite(mfile, hchunk->str, 4) != 4) {
		(void) fprintf(stderr, "Couldn't write header chunk "
		    "identifier\n");
		return (0);
	}

	hchunk->length = 6;
	hchunk->length = htoml(hchunk->length);
	if (mwrite(mfile, (char *)&hchunk->length, sizeof(hchunk->length)) !=
		sizeof(hchunk->length)) {
		(void) fprintf(stderr, "Couldn't write header chunk length\n");
		return (0);
	}
	hchunk->length = mtohl(hchunk->length);

	hchunk->format = htoms(hchunk->format);
	if (mwrite(mfile, (char *)&hchunk->format, sizeof(hchunk->format)) !=
		sizeof(hchunk->format)) {
		(void) fprintf(stderr, "Couldn't write header chunk format\n");
		return (0);
	}
	hchunk->format = mtohs(hchunk->format);

	hchunk->num_trks = htoms(hchunk->num_trks);
	if (mwrite(mfile, (char *)&hchunk->num_trks, sizeof(hchunk->num_trks))
	    != sizeof(hchunk->num_trks)) {
		(void) fprintf(stderr, "Couldn't write number of tracks\n");
		return (0);
	}
	hchunk->num_trks = mtohs(hchunk->num_trks);

	hchunk->division = htoms(hchunk->division);
	if (mwrite(mfile, (char *)&hchunk->division, sizeof(hchunk->division))
	    != sizeof(hchunk->division)) {
		(void) fprintf(stderr, "Couldn't write header chunk "
		    "division\n");
		return (0);
	}
	hchunk->division = mtohs(hchunk->division);

	return (1);
}

int
write_track_chunk(mfile, tchunk)
	int mfile;
	TCHUNK *tchunk;
{
	long midi_length;

	(void)strncpy(tchunk->str, "MTrk", 4);
	if (mwrite(mfile, tchunk->str, 4) != 4) {
		(void) fprintf(stderr, "Couldn't write track chunk "
		    "identifier\n");
		return (0);
	}

	midi_length = htoml(tchunk->length);
	if (mwrite(mfile, (char *)&midi_length, sizeof(midi_length)) !=
	    sizeof(midi_length)) {
		(void) fprintf(stderr, "Couldn't write track length\n");
		return (0);
	}

	if (mwrite(mfile, (char *)tchunk->event_start, tchunk->length)
	    != tchunk->length) {
		(void) fprintf(stderr, "Couldn't write track events\n");
		return (0);
	}

	return (1);
}

int
load_tracks(mfile, hchunk, tracks_ptr, spec_tracks)
	int mfile;
	HCHUNK *hchunk;
	TCHUNK **tracks_ptr;
	TRKS *spec_tracks;
{
	TCHUNK *tracks;
	int i;
	int cur_track;

	if (hchunk->format == 0) {
		if (spec_tracks->num_tracks) {
			(void) fprintf(stderr, "Specified tracks will be "
			    "ignored on MIDI file type 0\n");
		}

		if ((tracks = (TCHUNK *) malloc(sizeof(TCHUNK) * 2)) == NULL) {
			(void) fprintf(stderr, "No memory available for "
			    "tracks\n");
			return (0);
		}

		if (!read_track_chunk(mfile, &tracks[0])) {
			(void) fprintf(stderr, "Couldn't read track 0\n");
			return (0);
		}

		if (!split_type_zero(tracks)) {
			(void) fprintf(stderr, "Couldn't split MIDI file "
			    "type 0\n");
			return (0);
		}

		*tracks_ptr = tracks;

		return (2);
	}
		
	if (spec_tracks->num_tracks == 0) {
		for (i = 0; i < hchunk->num_trks; i++)
			spec_tracks->tracks[i] = i;
		spec_tracks->num_tracks = hchunk->num_trks;
	}

/* This check should be somewhere else (open_devs?)
	if (spec_tracks->num_tracks > 8) {
		(void) fprintf(stderr, "Too many tracks (%d).  Only using "
		    "first 8\n", spec_tracks->num_tracks);
		spec_tracks->num_tracks = 8;
	}
*/

	if ((tracks = (TCHUNK *) malloc(sizeof(TCHUNK) *
		    spec_tracks->num_tracks)) == NULL) {
		(void) fprintf(stderr, "No memory available for tracks\n");
		return (0);
	}

	cur_track = 0;
	/* read in tracks */
	for (i = 0; i <= spec_tracks->tracks[spec_tracks->num_tracks - 1];
		    i++) {
		if (i == spec_tracks->tracks[cur_track]) {
			/* read track chunk */
			if (!read_track_chunk(mfile, &tracks[cur_track++])) {
				(void) fprintf(stderr, "Couldn't read track "
				    "%d\n", i);
				return (0);
			}
		} else {
			if (!skip_track_chunk(mfile)) {
				(void) fprintf(stderr, "Couldn't skip track "
				"chunk %d\n", i);
				return (0);
			}
		}
	}

	*tracks_ptr = tracks;
	return (spec_tracks->num_tracks);
}

int
split_type_zero(tracks)
	TCHUNK *tracks;
{
	EVENT_TYPE event_type;
	int event_size;
	unsigned char event[256];
	unsigned char *data_events;
	unsigned char *data_ptr;
	unsigned char *tempo_events;
	unsigned char *tempo_ptr;
	int data_size = 0;
	int i;
	int tempo_size = 0;

	if ((data_events = (unsigned char *) malloc(tracks[0].length))
		    == NULL) {
		(void) fprintf(stderr, "Not enought memory to split track\n");
		return (0);
	}
	if ((tempo_events = (unsigned char *) malloc(tracks[0].length))
		    == NULL) {
		(void) fprintf(stderr, "Not enought memory to split track\n");
		return (0);
	}

	data_ptr = data_events;
	tempo_ptr = tempo_events;
	while (event_size = get_smf_event(&tracks[0], event, &event_type)) {
		if (event_size == -1) {
			(void) fprintf(stderr, "Problem getting event "
			    "while splitting\n");
			return (0);
		}
		if (event_type ==  NORMAL || event_type == SYSEX) {
			/* copy data event */
			for (i = 0; i < event_size; i++)
				*data_ptr++ = event[i];
			data_size += event_size;
			/* copy timing, but use noop */
			for (i = 0; i < event_size; i++) {
				if (event[i] & 0x80) {
					*tempo_ptr++ = event[i];
					tempo_size++;
				} else {
					*tempo_ptr++ = event[i];
					*tempo_ptr++ = MIDI_NOOP;
					tempo_size += 2;
					break;
				}
			}
		} else {
			/* copy tempo event (or general META event) */
			for (i = 0; i < event_size; i++)
				*tempo_ptr++ = event[i];
			tempo_size += event_size;
			/* copy timing, but use noop */
			for (i = 0; i < event_size; i++) {
				if (event[i] & 0x80) {
					*data_ptr++ = event[i];
					data_size++;
				} else {
					*data_ptr++ = event[i];
					*data_ptr++ = MIDI_NOOP;
					data_size += 2;
					break;
				};
			}
		}
	}
			
	free(tracks[0].events);
	tracks[0].events = tracks[0].event_start = tempo_events;
	tracks[1].events = tracks[1].event_start = data_events;
	tracks[0].msize = tracks[1].msize = tracks[0].length;
	tracks[0].length = tempo_size;
	tracks[1].length = data_size;
	tracks[0].pos = 0;
	tracks[1].pos = 0;
	(void) strncpy(tracks[0].str, "MTrk", 4);
	(void) strncpy(tracks[1].str, "MTrk", 4);
	return (1);
}

int
get_board_event(events_data, events_size, event_data, event_size)
	unsigned char *events_data;
	int events_size;
	unsigned char *event_data;
	int *event_size;
{
	MESS_STATE state;
	int delta;
	int parsed;
	static unsigned char r_state;

	*event_size = 0;
	parsed = 0;
	delta = 0;
	state = START;
	while (!parsed) {
		if (delta >= events_size)
			return (-1);
		switch (state) {
		case START:
			if (*events_data < 0xf0) {
				state = TIMEDEVENT;
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				break;
			}
			switch (*events_data) {
			case 0xf8:		/* timer overflow */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				parsed = 1;
				break;
			case 0xfc:		/* all play tracks ended */
			case 0xfd:		/* clock to PC message */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				parsed = 1;
				break;
			case 0xf9:
				/* conductor track request - should never see */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				parsed = 1;
				break;
			case 0xff:
				/* system message */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				state = SYSMSG;
				break;
			case 0xfe:
				/* command acknowledge - should never see */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				parsed = 1;
				break;
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
			case 0xf4:
			case 0xf5:
			case 0xf6:
			case 0xf7:
				/* track requests - should never see */
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
				parsed = 1;
				break;
			}
			break;
		case TIMEDEVENT:
			if (*events_data < 0xf0) {
				/* midi channel/voice message */
				if (*events_data & 0x80) {
					/* new running status */
					r_state = (*events_data >> 4) & 0x0f;
					state = NEWSTATUS;
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					break;
				} else {
					/* midi data byte */
					/*
					 * if timed messaged are off then
					 * this 1st data byte is actually the
					 * the second
					 * Only when midi not playing, data
					 * transfer is stopped state is active
					 * and timing byte on switch is off
					 */
					/*
					 * Since we can't know these things
					 * here, we'll assume how this routine
					 * will be used (oh Ghads!), and
					 * pretrend that the the timing byte
					 * is on when we the event was
					 * generated.  Why would they want
					 * to put events in a Standard Midi
					 * file if this wasn't the case
					 */
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					state = MIDIDATA;
					break;
				}
			} else {
				/* mcc timed event */
				if (*events_data == 0xf9) {
					/* measure end message */
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					parsed = 1;
					break;
				} else if (*events_data == 0xfc) {
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					parsed = 1;
					break;
				} else {
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					parsed = 1;
					break;
				}
			}
		case NEWSTATUS:
			/* first nonstatus byte of a midi message */
			/*
			 * if timed messaged are off then this 1st data byte is
			 * actually the the second.
			 * Only when midi not playing, data transfer is stopped
			 * state is active and timing byte on switch is off.
			 * We're not doing this.  See explaination (cop-out)
			 * above
			 */
			*event_data++ = *events_data++;
			(*event_size)++;
			delta++;
			switch (r_state) {
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0e:
				state = MIDIDATA;
				break;
			default:
				parsed = 1;
				break;
			}
			break;
		case MIDIDATA:
			/* second data byte of a midi message */
			*event_data++ = *events_data++;
			(*event_size)++;
			delta++;
			parsed = 1;
			break;
		case SYSMSG:
			/* first byte of a system message. */
			if (*events_data == 0xf0)
				/* system exclusive */
				state = SYSX;
			else {
				/* real time message */
				parsed = 1;
			}
			*event_data++ = *events_data++;
			(*event_size)++;
			delta++;
			break;
		case SYSX:
			/* system exclusive message */
			/*
			 * Supposedly the 0xf7 is optional as a terminator.
			 * Since all bytes in the SYSX message are data bytes,
			 * we should be able to tell when it is over by looking
			 * for any byte with high bit set.  Of course this is
			 * the first byte of the next message.
			 */
			if (*events_data & 0x80) {
				switch (*events_data) {
				case 0xf7:
					*event_data++ = *events_data++;
					(*event_size)++;
					delta++;
					parsed = 1;
					break;
				case 0xf8:
				case 0xfc:
				case 0xfd:
				case 0xf9:
				case 0xff:
				case 0xfe:
				case 0xf0:
				case 0xf1:
				case 0xf2:
				case 0xf3:
				case 0xf4:
				case 0xf5:
				case 0xf6:
					/*
					 * abnormal terminators
					 * We'll save these for next go
					 * around and add a terminator
					 */
					*event_data++ = 0xf7;
					(*event_size)++;
					parsed = 1;
					break;
				default:
					/*
					 * we already grabbed the timing
					 * byte.  This the the event byte.
					 * since we went on too far we'll
					 * have to decrement the delta.
					 * also cover up that extra byte
					 * with a terminator.
					 */
					*--event_data = 0x7f;
					delta--;
					parsed = 1;
					break;
				}
			} else {
				*event_data++ = *events_data++;
				(*event_size)++;
				delta++;
			}
			break;
		}
	}
	return (delta);
}

void
board2smf(board_event, board_length, smf_event, smf_length)
	unsigned char *board_event;
	int board_length;
	unsigned char *smf_event;
	int *smf_length;
{
	static long elapsed = 0;
	int delta;
	int i;


	*smf_length = 0;

	if (*board_event == 0xf8)
		elapsed += 240;
	else {
		delta = fix2var((long)(*board_event + elapsed), smf_event);
		elapsed = 0;
		*smf_length += delta;
		smf_event += delta;
		board_event++;
		for (i = 1; i < board_length; i++) {
			*smf_event++ = *board_event++;
			(*smf_length)++;
		}
	}

	return;
}


int
smf2board(track_num, smf_event, smf_size, board_event, t_scalar,
    need_new_event, event_type)
	int track_num;
	unsigned char *smf_event;
	int smf_size;
	unsigned char *board_event;
	int t_scalar;
	int *need_new_event;
	EVENT_TYPE *event_type;
{
	static long time_left[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static int fraction[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static EVENT_TYPE e_type[16];
	static int s_size[16];
	static int s_pos[16];
	static unsigned char s_data[16][256];
	static unsigned char *s_event[16];
	int delta;
	int size;

	*need_new_event = 0;
	size = 0;
	if (smf_event != NULL) {
		memcpy(s_data[track_num], smf_event, smf_size);
		s_event[track_num] = s_data[track_num];
		s_pos[track_num] = 0;
		s_size[track_num] = smf_size;
		e_type[track_num] = *event_type;

		time_left[track_num] = var2fix(s_event[track_num], &delta);
		fraction[track_num] +=  time_left[track_num] % t_scalar;
		time_left[track_num] /= t_scalar;
		if (fraction[track_num] >= t_scalar) {
			time_left[track_num] += fraction[track_num] / t_scalar;
			fraction[track_num] %= t_scalar;
		}
		s_pos[track_num] += delta;
		s_event[track_num] += delta;
	}
	if (time_left[track_num] >= MAX_TIME) {
		*board_event++ = 0xf8;
		size += 1;
		time_left[track_num] -= MAX_TIME + 1;
		*event_type = NORMAL;
		return (size);
	} else {
		*board_event++ = (unsigned char) time_left[track_num];
		size++;
		time_left[track_num] = 0;
		*event_type = e_type[track_num];
	}
		
	while (s_pos[track_num]++ < s_size[track_num]) {
		*board_event++ = *s_event[track_num]++;
		size++;
	}

	*need_new_event = 1;

	return (size);
}

int
fix2var(val, ptr)
	long val;
	unsigned char *ptr;
{
	int i;
	unsigned char buf[4] = {0, 0, 0, 0};
	unsigned char *bptr;

	bptr = buf;
	*bptr++ = val & 0x7f;
	while ((val >>= 7) > 0) {
		*bptr |= 0x80;
		*bptr++ += (val & 0x7f);
	}

	i = 0;
	do {
		*ptr++ = *--bptr;
		i++;
	} while (bptr != buf);

	return (i);
}

long
var2fix(var, delta)
	unsigned char *var;
	int *delta;
{
	long fix;

	fix = 0;
	*delta = 0;
	if (*var & 0x80)
		do {
			fix = (fix << 7) + (*var & 0x7f);
			(*delta)++;
		} while (*var++ & 0x80);
	else {
		fix = *var++;
		(*delta)++;
	}

	return (fix);
}

unsigned char
double2tempo(d)
	double d;
{
	double bit_vals[] = {2.0, 1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625};
	int i;
	unsigned char tempo_scalar;

	for (i = 0; i < 8; i++) {
		if (d < bit_vals[i])
			tempo_scalar = (tempo_scalar << 1) & ~1;
		else {
			tempo_scalar = (tempo_scalar << 1) | 1;
			d -= bit_vals[i];
		}
	}
	return(tempo_scalar);
}

void
free_track(track)
	TCHUNK *track;
{
	free(track->event_start);
	track->event_start = NULL;
	track->events = NULL;
	track->length = 0;
	track->pos = 0;
	track->msize = 0;
}

int
mread(dev, buf, size)
	int dev;
	char *buf;
	int size;
{
	int num_read;
	int total_read;

	total_read = 0;
	do {
		if ((num_read = read(dev, buf, size - total_read)) == -1) {
			perror("");
			return (-1);
		}
		if (num_read == 0)
			break;
		total_read += num_read;
		buf += num_read;
	} while (size > total_read);
	if (total_read == 0)
		MidiEof = 1;
	return (total_read);
}

int
mwrite(dev, buf, size)
	int dev;
	char *buf;
	int size;
{
	int num_written;
	int total_written;

	total_written = 0;
	do {
		if ((num_written = write(dev, buf, size - total_written))
		    == -1) {
			perror("");
			return (-1);
		}
		if (num_written == 0)
			break;
		total_written += num_written;
		buf += num_written;
	} while (size > total_written);
	return (total_written);
}
