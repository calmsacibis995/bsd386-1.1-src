
DEFINES = -DTFTP_DIR=\"/tftpboot\"
PROG = rarpd

CFLAGS = -O $(DEFINES)
CC = cc -g

${PROG}: ${PROG}.c
	$(CC) $(CFLAGS) -o $@ $(PROG).c

install: ${PROG}
	install ${PROG} /usr/etc

install-man:
	install rarpd.8 /usr/man/man8

lint:
	lint ${PROG}.c

tags:
	ctags -wtd *.c

clean:
	rm -f *.o core ${PROG}
