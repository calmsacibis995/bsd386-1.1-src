#
#

#CFLAGS= -g -DSYSV -I../../hdrs
CFLAGS= -g -I../../hdrs

all:	gencat dumpmsg

gencat:	gencat.o genlib.o
	$(CC) $(CFLAGS) -o $@ gencat.o genlib.o

dumpmsg: dumpmsg.o genlib.o
	$(CC) $(CFLAGS) -o $@ dumpmsg.o genlib.o

clean:
	rm -f gencat.o genlib.o dumpmsg.o *~ *.bak gencat dumpmsg

gencat.o:	gencat.h
genlib.o:	gencat.h ../../hdrs/msgcat.h
