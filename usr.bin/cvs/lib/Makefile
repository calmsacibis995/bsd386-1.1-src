LIB=cvs
SRCS=	argmatch.c error.c getopt.c getopt1.c sighandle.c strippath.c \
	stripslash.c yesno.c getdate.y regex.c fnmatch.c
	 
CFLAGS+= -DDIRENT -DSTDC_HEADERS -DPOSIX -DFTIME_MISSING -DHAVE_TIMEZONE
CFLAGS+= -DUTIME_NULL_MISSING -I. -I${.CURDIR} -I${.CURDIR}/../lib 

install:

.include <bsd.lib.mk>
