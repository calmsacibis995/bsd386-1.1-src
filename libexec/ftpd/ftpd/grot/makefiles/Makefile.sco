CC       = cc
IFLAGS   = -I.. -I../support
LFLAGS   = -L../support
CFLAGS   = -O ${IFLAGS} ${LFLAGS}
# 1) If you have -lprot_s, consider to use it instead of -lprot.
# 2) -lcrypt can be used in place of -lcrypt_i. If you do not have any crypt
#    library, get and install ftp.sco.com:/SLS/lng225* (International Crypt
#    Supplement), then use -lcrypt_i.
LIBES    = -lsupport -lsocket -lprot -lcrypt_i -lc_s -lc -lx
LIBC     = /lib/libc.a
LINTFLAGS=	
#LKERB    =
MKDEP    = ../util/mkdep

SRCS   = ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c access.c extensions.c \
		 realpath.c acl.c private.c authenticate.c conversions.c hostacc.c
OBJS   = ftpd.o ftpcmd.o glob.o logwtmp.o popen.o vers.o access.o extensions.o \
		 realpath.o acl.o private.o authenticate.o conversions.o hostacc.o
 
all: ftpd ftpcount ftpshut ckconfig
 
ftpcount:	ftpcount.c pathnames.h
	${CC} ${CFLAGS} -o $@ ftpcount.c vers.o ${LIBES}

ftpshut:    ftpshut.c pathnames.h
	${CC} ${CFLAGS} -o $@ ftpshut.c vers.o ${LIBES}

ftpd: ${OBJS} ${LIBC}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBES}

ckconfig:   ckconfig.c
	${CC} ${CFLAGS} -o $@ ckconfig.c
 
index:	index.o ${LIBC}
	${CC} -Bstatic -o $@ index.o
 
vers.o: ftpd.c ftpcmd.y
	sh newvers.sh
	${CC} ${CFLAGS} -c vers.c
 
clean:
	rm -f ${OBJS} ftpd ftpcmd.c ftpshut ftpshut.o ftpcount ftpcount.o
	rm -f core index index.o
 
cleandir: clean
	rm -f tags .depend
 
depend: ${SRCS}
	${MKDEP} ${CFLAGS} ${SRCS}
 
lint: ${SRCS}
	lint ${CFLAGS} ${LINTFLAGS} ${SRCS}
 
tags: ${SRCS}
	ctags ${SRCS}
