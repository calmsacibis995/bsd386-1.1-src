CC     = cc
AR     = ar cq
RANLIB = touch
LIBC   = /lib/libc.a
IFLAGS = 
LFLAGS = 
CFLAGS = -O -DDEBUG ${IFLAGS} ${LFLAGS}

SRCS   = fnmatch.c strcasestr.c strsep.c authuser.c 
OBJS   = fnmatch.o strcasestr.o strsep.o authuser.o 

all: $(OBJS)
	-rm -f libsupport.a
	${AR} libsupport.a $(OBJS)
	${RANLIB} libsupport.a

clean:
	-rm -f *.o libsupport.a

ftp.h:
	install -c -m 444 ftp.h /usr/include/arpa

paths.h:
	install -c -m 444 paths.h /usr/include

fnmatch.o: fnmatch.c
	${CC} ${CFLAGS} -c fnmatch.c

getusershell.o: getusershell.c
	${CC} ${CFLAGS} -c getusershell.c

strerror.o: strerror.c
	${CC} ${CFLAGS} -c strerror.c

strdup.o: strdup.c
	${CC} ${CFLAGS} -c strdup.c

strcasestr.o: strcasestr.c
	${CC} ${CFLAGS} -c strcasestr.c

strsep.o: strsep.c
	${CC} ${CFLAGS} -c strsep.c

authuser.o: authuser.c
	${CC} ${CFLAGS} -c authuser.c

ftw.o: ftw.c
	${CC} ${CFLAGS} -c ftw.c
