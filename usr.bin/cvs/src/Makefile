SRCS=	add.c admin.c checkin.c checkout.c classify.c commit.c \
	create_adm.c diff.c entries.c find_names.c hash.c history.c ignore.c \
	import.c lock.c log.c logmsg.c main.c myndbm.c rcs.c modules.c \
	no_diff.c parseinfo.c patch.c recurse.c release.c remove.c repos.c \
	rtag.c status.c tag.c update.c vers_ts.c subr.c version.c
PROG=	cvs
CFLAGS+= -DDIRENT -DSTDC_HEADERS -DPOSIX -DFTIME_MISSING -DHAVE_TIMEZONE
CFLAGS+= -DUTIME_NULL_MISSING -I. -I${.CURDIR} -I${.CURDIR}/../lib 
LDFLAGS+= -L${.CURDIR}/../lib/obj
LDADD=	-lcvs
MAN1= cvs.0
MAN5= cvsfiles.0

.include "../../Makefile.inc"
.include <bsd.prog.mk>
