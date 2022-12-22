#
# Makefile for Sound Blaster application programs.
# Steve Haehnichen <shaehnic@ucsd.edu>
#

CFLAGS = -O2
CC = gcc
GETOPT = getopt.o getopt1.o

PROGS = mixer cdsp notes chew float midithru filt midi2fm revox playmidi record

# Non-distributed experimental toys
EXP_PROGS = robo along comp clip resynth ptrack chan_voc

all: $(PROGS)
#all: $(PROGS) $(EXP_PROGS)

revox: revox.o $(GETOPT)
	$(CC) $(CFLAGS) -o revox revox.o $(GETOPT)

chan_voc: chan_voc.o fft_10.o fftsubs.o $(GETOPT)
	$(CC) $(CFLAGS) -o chan_voc chan_voc.o fft_10.o fftsubs.o $(GETOPT)

resynth: resynth.o fft_8.o fftsubs.o $(GETOPT)
	$(CC) $(CFLAGS) -o resynth resynth.o fft_8.o fftsubs.o $(GETOPT)

robo: robo.o $(GETOPT)
	$(CC) $(CFLAGS) -o robo robo.o $(GETOPT)

comp: comp.o $(GETOPT)
	$(CC) $(CFLAGS) -o comp comp.o $(GETOPT)

clip: clip.o $(GETOPT)
	$(CC) $(CFLAGS) -o clip clip.o $(GETOPT)

along: along.o $(GETOPT)
	$(CC) $(CFLAGS) -o along along.o $(GETOPT)

ptrack: ptrack.o $(GETOPT)
	$(CC) $(CFLAGS) -o ptrack ptrack.o $(GETOPT)

record: record.o $(GETOPT)
	$(CC) $(CFLAGS) -o record record.o $(GETOPT)

midifile.o: midifile.c
	$(CC) $(CFLAGS) -c midifile.c

playmidi: playmidi.o midifile.o $(GETOPT)
	$(CC) $(CFLAGS) -o playmidi playmidi.o midifile.o $(GETOPT)

midi2fm: midi2fm.o 
	$(CC) $(CFLAGS) -o midi2fm midi2fm.o

filt: filt.o 
	$(CC) $(CFLAGS) -o filt filt.o

midithru: midithru.o 
	$(CC) $(CFLAGS) -o midithru midithru.o

chew: $(GETOPT) chew.o
	$(CC) $(CFLAGS) -o chew chew.o $(GETOPT)

float: $(GETOPT) float.o
	$(CC) $(CFLAGS) -o float float.o $(GETOPT)

cdsp: $(GETOPT) cdsp.o
	$(CC) $(CFLAGS) -o cdsp cdsp.o $(GETOPT)

mixer: $(GETOPT) mixer.o
	$(CC) $(CFLAGS) -o mixer mixer.o $(GETOPT)

notes: notes.o
	$(CC) $(CFLAGS) -o notes notes.o -lm

clean:
	rm -f *.o core $(PROGS)
