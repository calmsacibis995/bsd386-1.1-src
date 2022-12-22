#	BSDI	$Id: Makefile,v 1.3 1994/01/30 21:11:56 polk Exp $

PROG=	xntpd
MAN8=	xntpd.0
SRCS=	ntp_config.c ntp_control.c ntp_io.c ntp_leap.c \
	ntp_loopfilter.c ntp_monitor.c ntp_peer.c ntp_proto.c \
	ntp_refclock.c ntp_request.c ntp_restrict.c ntp_timer.c \
	ntp_unixclock.c ntp_util.c ntpd.c refclock_chu.c \
	refclock_conf.c refclock_local.c refclock_pst.c \
	refclock_wwvb.c refclock_goes.c refclock_mx4200.c \
	refclock_parse.c refclock_as2201.c refclock_omega.c \
	refclock_tpro.c refclock_leitch.c refclock_irig.c \
	refclock_msfees.c ntp_intres.c ntp_filegen.c version.c
CFLAGS+=-DSYS_BSDI -DDEBUG -DREFCLOCK -DSYSLOG_FILE -I${.CURDIR}/../include
LDADD+=	${.CURDIR}/../lib/obj/libntp.a -lkvm
DPADD+=	${.CURDIR}/../lib/obj/libntp.a ${LIBKVM}
CLEANFILES+=.version version.c

.include "../../Makefile.inc"

.depend: version.c

version.c: ${.CURDIR}/../VERSION
	${.CURDIR}/../mkversion ${PROG} ${.CURDIR}/../VERSION

.include <bsd.prog.mk>
