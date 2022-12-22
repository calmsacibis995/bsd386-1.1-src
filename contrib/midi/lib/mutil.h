/*
 * $Id: mutil.h,v 1.1.1.1 1992/08/14 00:14:35 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#define mtohl(l) ((((l) & 0x000000ff) << 24) + (((l) & 0x0000ff00) << 8) + \
        (((l) & 0x00ff0000) >> 8) + (((l) & 0xff000000) >> 24))
#define mtohs(s) ((((s) & 0x00ff) << 8) + (((s) & 0xff00) >> 8))
#define htoml(l) ((((l) & 0x000000ff) << 24) + (((l) & 0x0000ff00) << 8) + \
        (((l) & 0x00ff0000) >> 8) + (((l) & 0xff000000) >> 24))
#define htoms(s) ((((s) & 0x00ff) << 8) + (((s) & 0xff00) >> 8))

typedef enum {NORMAL = 0xff, SYSEX = 0xf0, METASEQNUM = 0x00, METATEXT = 0x01,
    METACPY = 0x02, METASEQNAME = 0x03, METAINSTNAME = 0x04, METALYRIC = 0x05,
    METAMARKER = 0x06, METACUE = 0x07, METACHANPREFIX = 0x20, METAEOT = 0x2f,
    METATEMPO = 0x51, METASMPTE = 0x54, METATIME = 0x58, METAKEY = 0x59,
    METASEQSPEC = 0x7f} EVENT_TYPE;

typedef enum {SEND_ERROR, SEND_VALID, SEND_INVALID, SEND_DONE} SEND_VAL;

typedef enum {START, TIMEDEVENT, NEWSTATUS, MIDIDATA, SYSMSG, SYSX} MESS_STATE;

typedef struct {
        char str[4];
        long length;
        short format;
        short num_trks;
        short division;
} HCHUNK;

typedef struct {
        char str[4];
        long pos;
        long length;
	long msize;
        unsigned char *events;
        unsigned char *event_start;
} TCHUNK;

typedef struct {
	int num_tracks;
	int tracks[50];
} TRKS;

/* hack to tell us when reading from stdin if there is anymore data */
extern int MidiEof;

/* function prototypes */
extern int get_smf_event(TCHUNK *, unsigned char *, EVENT_TYPE *);
extern int put_smf_event(TCHUNK *, unsigned char *, int);
extern int read_header_chunk(int, HCHUNK *);
extern int read_track_chunk(int, TCHUNK *);
extern int skip_track_chunk(int);
extern void rewind_track(TCHUNK *);
extern void reset_track(TCHUNK *);
extern int write_header_chunk(int, HCHUNK *);
extern int write_track_chunk(int, TCHUNK *);
extern int get_range(char *, int *);
extern int play_tracks(TCHUNK *, int *, int, int, int);
extern int record_tracks(TCHUNK *, int *, int, TCHUNK *, int, int, int);
extern void stop_recording(void);
extern SEND_VAL send_event(int, unsigned char *, int, EVENT_TYPE, int);
extern int load_tracks(int, HCHUNK *, TCHUNK **, TRKS *);
extern int open_midi_devices(HCHUNK *, TRKS *, int *, int *, unsigned char *);
extern int split_type_zero(TCHUNK *);
extern int fix2var(long, unsigned char *);
extern long var2fix(unsigned char *, int *);
extern int get_board_event(unsigned char *, int, unsigned char *, int *);
extern void board2smf(unsigned char *, int, unsigned char *, int *);
extern int smf2board(int, unsigned char *, int, unsigned char *, int, int *,
    EVENT_TYPE *);
extern void flush_event(TCHUNK *);
extern int process_record_event(int, TCHUNK *);
extern int process_play_event(int,  TCHUNK *, int, int *, int *, int);
extern void set_midi_verbose(int);
extern unsigned char double2tempo(double);
extern void free_track(TCHUNK *);
extern int mread(int, char *, int);
extern int mwrite(int, char *, int);
