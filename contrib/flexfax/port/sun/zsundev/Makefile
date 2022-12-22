zsunbuf.o: zsunbuf.c
	cc -DKERNEL -DLOADABLE -DDEBUG -c zsunbuf.c

zsunbuftest: zsunbuftest.o
	cc -o zsunbuftest zsunbuftest.o

load: zsunbuf.o
	/usr/etc/modload zsunbuf.o -entry _zsunbufinit

clean:
	rm -f zsunbuf *.o zsunbuftest zsunbuf.shar

shar: README faxd.patch zsunbuf.c TESTING zsunbuftest.c Makefile
	shar README faxd.patch zsunbuf.c TESTING  \
		zsunbuftest.c  Makefile  >zsunbuf.shar
