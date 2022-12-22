#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	from @(#)Makefile	5.16 (Berkeley) 3/14/88
#
####
#### Updated 3/27/89 for 'dig' version 1.0 at University of Southern
#### California Information Sciences Institute (USC-ISI).
####
#### Modified & distributed with 'dig' version 2.0 from USC-ISI (9/1/90).
####
####

SRCS=	herror.c res_comp.c res_debug.c res_init.c res_mkquery.c res_query.c \
	res_send.c strcasecmp.c gethostnamadr.c sethostent.c qtime.c
OBJS=	herror.o res_comp.o res_debug.o res_init.o res_mkquery.o res_query.o \
	res_send.o strcasecmp.o gethostnamadr.o sethostent.o qtime.o
DEFS=	-DDEBUG
CFLAGS=	-O ${DEFS}

dig: libresolv.a dig.c list.o Makefile options.c
	${CC} ${CFLAGS} dig.c -L`pwd` -lresolv list.o -o dig

dig.g: libresolv.a dig.c list.o Makefile options.c
	${CC} -g ${DEFS} dig.c -L`pwd` -lresolv list.o -o dig.g

libresolv.a: ${OBJS}
	ar cru libresolv.a ${OBJS}
	ranlib libresolv.a

.c.o:
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

dig.local: dig.c list.o options.c
	${CC} ${CFLAGS} -DRESLOCAL dig.c -lresolv list.o -o dig.local

dig.dbx: libresolv.a dig.c list.o Makefile
	${CC} -g ${DEFS} dig.c -L`pwd` -lresolv list.o -o dig

clean: FRC
	rm -f *.o errs a.out core tags

depend: FRC
	mkdep -f Makefile.resolv ${CFLAGS} ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

herror.c:
herror.o: herror.c /usr/include/sys/types.h 
herror.o: /usr/include/sys/uio.h
res_comp.c:
res_comp.o: res_comp.c hfiles.h /usr/include/sys/types.h
res_comp.o:  /usr/include/stdio.h nameser.h
res_debug.c:
res_debug.o: res_debug.c hfiles.h /usr/include/sys/types.h
res_debug.o:  /usr/include/netinet/in.h
res_debug.o: /usr/include/stdio.h nameser.h resolv.h pflag.h
res_init.c:
res_init.o: res_init.c hfiles.h /usr/include/sys/types.h
res_init.o:  /usr/include/sys/socket.h
res_init.o: /usr/include/netinet/in.h /usr/include/stdio.h nameser.h resolv.h
res_mkquery.c:
res_mkquery.o: res_mkquery.c hfiles.h /usr/include/stdio.h
res_mkquery.o: /usr/include/sys/types.h 
res_mkquery.o: /usr/include/netinet/in.h nameser.h resolv.h
res_query.c:
res_query.o: res_query.c hfiles.h param.h
res_query.o: /usr/include/signal.h /usr/include/sys/types.h
res_query.o:  /usr/include/sys/socket.h
res_query.o: /usr/include/netinet/in.h /usr/include/ctype.h netdb.h
res_query.o: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
res_query.o: /usr/include/strings.h /usr/include/arpa/inet.h nameser.h resolv.h
res_send.c:
res_send.o: res_send.c hfiles.h param.h
res_send.o: /usr/include/signal.h /usr/include/sys/types.h
res_send.o:  /usr/include/sys/socket.h
res_send.o: /usr/include/sys/uio.h /usr/include/netinet/in.h
res_send.o: /usr/include/stdio.h /usr/include/errno.h /usr/include/sys/errno.h
res_send.o: nameser.h resolv.h qtime.h /usr/include/stdio.h
res_send.o: /usr/include/sys/time.h /usr/include/time.h
res_send.o: /usr/include/sys/types.h pflag.h
strcasecmp.c:
strcasecmp.o: strcasecmp.c
gethostnamadr.c:
gethostnamadr.o: gethostnamadr.c hfiles.h param.h
gethostnamadr.o: /usr/include/signal.h /usr/include/sys/types.h
gethostnamadr.o:  /usr/include/sys/socket.h
gethostnamadr.o: /usr/include/netinet/in.h /usr/include/ctype.h netdb.h
gethostnamadr.o: /usr/include/stdio.h /usr/include/errno.h
gethostnamadr.o: /usr/include/sys/errno.h /usr/include/arpa/inet.h nameser.h
gethostnamadr.o: resolv.h
sethostent.c:
sethostent.o: sethostent.c hfiles.h /usr/include/sys/types.h
sethostent.o:  nameser.h /usr/include/netinet/in.h
sethostent.o: resolv.h
qtime.c:
qtime.o: qtime.c qtime.h /usr/include/stdio.h /usr/include/sys/time.h
qtime.o: /usr/include/time.h /usr/include/sys/types.h
qtime.o: 
dig.c:
dig.o: dig.c hfiles.h /usr/include/sys/types.h
dig.o: /usr/include/netinet/in.h /usr/include/stdio.h resolv.h
dig.o: /usr/include/sys/file.h /usr/include/sys/fcntl.h /usr/include/sys/stat.h
dig.o: /usr/include/ctype.h subr.c hfiles.h /usr/include/stdio.h
dig.o: /usr/include/strings.h /usr/include/sys/types.h netdb.h
dig.o: /usr/include/sys/socket.h nameser.h /usr/include/signal.h
dig.o: /usr/include/setjmp.h res.h strcasecmp.c pflag.h qtime.h
dig.o: /usr/include/stdio.h /usr/include/sys/time.h /usr/include/time.h
dig.o: /usr/include/sys/types.h
dig.o:

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
