#	BSDI $Id: Makefile,v 1.7 1993/10/22 17:29:27 donn Exp $
#	@(#)Makefile	5.10 (Berkeley) 5/6/91

LIB=	util
CFLAGS+=-DLIBC_SCCS
SRCS=   daemon.c devname.c disklabel.c getloadavg.c login.c \
	login_tty.c logout.c logwtmp.c pty.c pwcache.c setproctitle.c \
	setterm.c
MAN3=	getloadavg.0 pwcache.0

MLINKS+=pwcache.3 user_from_uid.3 pwcache.3 group_from_gid.3

.include <bsd.lib.mk>
