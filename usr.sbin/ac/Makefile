#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:26:27 polk Exp $

PROG=	ac
SRCS=	findmidnight.c main.c make_name.c make_time.c \
	makelist.c mstrdup.c namelist2.c print2.c time_list.c update2.c

COPTS= -DWTMP_FILE='"/var/log/wtmp"'
CFLAGS+=-traditional 

.include <bsd.prog.mk>
