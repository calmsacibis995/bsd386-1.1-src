#	BSDI $Id: Makefile,v 1.7 1994/01/05 21:26:26 polk Exp $
#	@(#)Makefile	5.7.1.1 (Berkeley) 5/7/91

MAN5=	a.out.0 acct.0 cat.0 core.0 dbx.0 dir.0 disktab.0 fs.0 fstab.0 \
	group.0 hosts.0 hosts.equiv.0 networks.0 passwd.0 phones.0 printcap.0 \
	protocols.0 remote.0 resolver.0 services.0 shells.0 stab.0 types.0 \
	utmp.0
MLINKS=	fs.5 inode.5 utmp.5 wtmp.5 utmp.5 lastlog.5

all: ${MAN5}

clean depend lint tags:

cleandir:
	rm -f ${MAN5}

install: maninstall afterinstall

afterinstall:
	rm -f ${DESTDIR}${MANDIR}5/resolv.conf.0
	ln ${DESTDIR}${MANDIR}5/resolver.0 \
		${DESTDIR}${MANDIR}5/resolv.conf.0
	rm -f ${DESTDIR}${MANDIR}5/rhosts.0
	ln ${DESTDIR}${MANDIR}5/hosts.equiv.0 \
		${DESTDIR}${MANDIR}5/rhosts.0
	

.include <bsd.prog.mk>
