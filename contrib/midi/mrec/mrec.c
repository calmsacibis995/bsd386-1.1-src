/*
 * $Id: mrec.c,v 1.1.1.1 1992/08/14 00:14:33 trent Exp $
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
#include	<termios.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/ioctl.h>
#include	<i386/isa/midiioctl.h>
#include	<mutil.h>

enum {NOT_STARTED, PLAYING, DONE} Playing = NOT_STARTED;
int Control;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct termios stdin_ios;
	HCHUNK rhchunk;
	HCHUNK phchunk;
	TCHUNK *ptracks;
	TCHUNK rtrack;
	TRKS spec_tracks;
	long div_ioctls[] = {MRES192, MRES168, MRES144, MRES120, MRES96,
	    MRES72, MRES48};
	tcflag_t orig_local;
	int divs[] = {192, 168, 144, 120, 96, 72, 48};
	int cond_on;
	int got_pfile_name;
	int got_rfile_name;
	int i;
	int num_open_trks;
	int pdevs[8];
	int pfile;
	int rfile;
	int tempo_scalar;
	int verbose;
	short division = -1;
	unsigned char tempo = 120;
	unsigned char atrks;
	char *chk_ptr;
	char pfile_name[256];
	char rfile_name[256];
	void usage(char *);
	void key_press();

	got_pfile_name = 0;
	got_rfile_name = 0;
	verbose = 0;

        for (i = 1; i < argc; i++) {
                if (argv[i][0] == '-') {
                        switch (argv[i][1]) {
                        case 'v':
                                verbose = 1;
                                break;
                        case 't':
				tempo = (unsigned char) strtol(argv[i + 1],
				    &chk_ptr, 0);
				if (chk_ptr == argv[i + 1]) {
					(void) fprintf(stderr,
					    "Invalid tempo %s\n", argv[i + 1]);
					exit(1);
				}
				i++;
				break;
			case 'T':
				if (argv[i][2] == '\0') {
					if ((spec_tracks.num_tracks =
					    get_range(argv[i + 1],
					    spec_tracks.tracks)) == -1) {
						(void) fprintf(stderr,
						    "Couldn't parse track"
						    " list\n");
						exit(1);
					}
					i++;
				} else {
					if ((spec_tracks.num_tracks =
					    get_range(&argv[i][2],
					    spec_tracks.tracks)) == -1) {
						(void) fprintf(stderr,
						    "Couldn't parse track"
						    " list\n");
						exit(1);
					}
				}
				break;
			case 'd':
				division = (short) strtol(argv[i + 1],
				    &chk_ptr, 0);
				if (chk_ptr == argv[i + 1]) {
					(void) fprintf(stderr,
					    "Invalid division %s\n",
					    argv[i + 1]);
					exit(1);
				}
				i++;
				break;
			case 'p':
				if (got_pfile_name) {
					usage(argv[0]);
					exit(1);
				}
				strcpy(pfile_name, argv[++i]);
				got_pfile_name = 1;
				break;
			case '\0':
				if (got_rfile_name) {
					usage(argv[0]);
					exit(1);
				}
				got_rfile_name = 1;
				strcpy(rfile_name, "stdin");
				break;
			}
		} else {
			if (got_rfile_name) {
				usage(argv[0]);
				exit(1);
			}
			got_rfile_name = 1;
			strcpy(rfile_name, argv[i]);
		}
	}

        if (!strcmp(rfile_name, "stdin"))
                rfile = 0;
        else
                if ((rfile = open(rfile_name, O_WRONLY | O_TRUNC | O_CREAT,
		    0644)) == -1) {
                        perror("");
                        (void) fprintf(stderr, "Couldn't open record"
			    " MIDI file %s\n", rfile_name);
                        exit(1);
                }

	if (got_pfile_name) {
		if ((pfile = open(pfile_name, O_RDONLY, 0)) == -1) {
			perror("");
			(void)fprintf(stderr,
			    "Couldn't open play MIDI file %s\n", pfile_name);
				exit(1);
		}

	        /* read header chunk */
		if (!read_header_chunk(pfile, &phchunk)) {
               		 exit(1);
        	}
	}

        if (verbose) {
                (void) printf("format = %d\n", phchunk.format);
                (void) printf("number of tracks = %d\n", phchunk.num_trks);
                (void) printf("division = %d\n", phchunk.division);
        }

        /* open control */
        if ((Control = open("/dev/midicntl", O_RDONLY | O_NONBLOCK, 0)) == -1) {
                perror("");
                (void) fprintf(stderr, "Couldn't open /dev/midicntl\n");
                exit(1);
        }

        if (division == -1)
		if (got_pfile_name)
	                division = phchunk.division;
		else
			division = 120;

        if (division < 0) {
		(void) fprintf(stderr, "%s can't currently handle SMPTE"
		    " time codes\n", argv[0]);
                exit(1);
        }

        for (i = 0; i < sizeof(divs) / sizeof(divs[0]); i++) {
                if (division % divs[i] == 0) {
                        if (ioctl(Control, div_ioctls[i], NULL) == -1) {
                                perror("");
                                (void) fprintf(stderr,
				    "Couldn't set ticks / clock\n");
                                exit(1);
                        }
                        tempo_scalar = division / divs[i];
                        break;
                }
        }

        if (i == sizeof(divs) / sizeof(divs[0])) {
               (void) fprintf(stderr, "Bad ticks / beat value %d\n", division);
                (void) fprintf(stderr,
                    "Must be one of 48, 72, 96, 120, 144, 168, 192\n");
                (void) fprintf(stderr, "Try setting division manually\n");
                exit(1);
        }

	if (!got_pfile_name) {
		atrks = 0;
		if (ioctl(Control, MSELTRKS, &atrks) == -1) {
			(void) fprintf(stderr,
			    "Couldn't select active play tracks\n");
			exit(1);
		}
		num_open_trks = 0;
		ptracks = NULL;
	} else {
		if (!(num_open_trks = load_tracks(pfile, &phchunk, &ptracks,
		    &spec_tracks))) {
			(void) fprintf(stderr, "Couldn't load tracks\n");
			exit(1);
       		}

		if ((cond_on = open_midi_devices(&phchunk, &spec_tracks,
		    num_open_trks, pdevs, &atrks)) == -1) {
			(void) fprintf(stderr, "Coudln't open midi devices\n");
			exit(1);
		}

		if (ioctl(Control, MSELTRKS, &atrks) == -1) {
			(void) fprintf(stderr,
			    "Couldn't select active play tracks\n");
			exit(1);
		}

		if (cond_on)
			if (ioctl(Control, MCONDON, NULL) == -1) {
				(void) fprintf(stderr,
				    "Couldn't select conductor track\n");
				exit(1);
			}

		if (ioctl(Control, MCLRPC, NULL) == -1) {
			(void) fprintf(stderr,
			    "Couldn't clear play counters\n");
			exit(1);
		}
	}

        rtrack.length = 0;
		rtrack.events = NULL;

        if(ioctl(0, TIOCGETA, &stdin_ios) == -1) {
                perror("");
                exit(1);
        }
        orig_local = stdin_ios.c_lflag;
        stdin_ios.c_lflag &= ~ICANON;
        stdin_ios.c_lflag &= ~ECHO;
        if(ioctl(0, TIOCSETA, &stdin_ios) == -1) {
                perror("");
                exit(1);
        }

        signal(SIGIO, key_press);

        if (fcntl(0, F_SETOWN, getpid()) == -1) {
                perror("");
                (void) fprintf(stderr, "Couldn't setown for stdin\n");
                exit(1);
        }

        if (fcntl(0, F_SETFL, O_ASYNC) == -1) {
                perror("");
                (void) fprintf(stderr, "Couldn't set stdin to async mode\n");
                exit(1);
        }

        (void) printf("Press a key to begin recording\n");

        pause();

        /* won't exit util stop_recording is called */
        if (!record_tracks(ptracks, pdevs, num_open_trks, &rtrack,
	    Control, tempo_scalar, 1)) {
                (void) fprintf(stderr, "Couldn't record track\n");
                exit(1);
        }

        rhchunk.format = 0;
        rhchunk.num_trks = 1;
        rhchunk.division = division / tempo_scalar;

        if (!write_header_chunk(rfile, &rhchunk))
                exit(1);

        if (!write_track_chunk(rfile, &rtrack))
                exit(1);

        stdin_ios.c_lflag = orig_local;
        if(ioctl(0, TIOCSETA, &stdin_ios) == -1) {
                perror("");
                exit(1);
        }

	if (ioctl(Control, MSTOPANDREC, NULL) == -1) {
		perror("MSTOPANDREC");
		(void) fprintf(stderr, "Couldn't stop recording\n");
		exit(1);
	}

        return (0);
}


void
usage(program)
	char *program;
{
        (void) fprintf(stderr, "%s midi_file\n", program);
}

void
key_press()
{
	int trash;

        signal(SIGIO, SIG_IGN);

        if (read(0, &trash, 1) != 1) {
                perror("");
                (void) fprintf(stderr, "Couldn't read one char\n");
                exit(1);
        }


        switch (Playing) {
        case NOT_STARTED:
                Playing = PLAYING;

                if (ioctl(Control, MSTARTANDREC, NULL) == -1) {
			perror("MSTARTANDREC");
                        (void) fprintf(stderr, "Couldn't start recording\n");
                        exit(1);
                }
                (void) printf("Press a key to stop recording\n");
                signal(SIGIO, key_press);
                break;
        case PLAYING:
                Playing = DONE;
                stop_recording();
                if (fcntl(0, F_SETFL, 0) == -1) {
                        perror("");
                        (void) fprintf(stderr,
			    "Couldn't set stdin to async mode\n");
                        exit(1);
                }
        case DONE:
                /* shut the fuck up lint */
                ;
        }

        return;
}
