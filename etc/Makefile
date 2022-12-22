#	BSDI	$Id: Makefile,v 1.15 1994/01/06 16:12:00 polk Exp $
#	@(#)Makefile	5.11 (Berkeley) 5/21/91

# utmp is group writable by UTMPGRP
UTMPGRP=utmp

NOOBJ=	oobj

# disktab may be wrong -- hcx9 is a tahoe, but gets its own.
# -rw-r--r--
BIN=	aliases csh.cshrc csh.login csh.logout crontab daily dm.conf \
	ftpusers gettytab group hosts hosts.equiv hosts.lpd inetd.conf \
	man.conf monthly motd netstart phones printcap protocols rc \
	rc.local remote security services shells syslog.conf ttys weekly \
	mtoolsrc networks etc.${MACHINE}/disktab \
	daily.local monthly.local weekly.local

MTREE=	BSDI.root.dist BSDI.usr.dist BSDI.var.dist
KRB=	README krb.conf krb.realms
NAMEDB=	localhost.rev named.boot root.cache
PCS=	pcs750.bin
WCS1=	wcs fppwcs poc poc1 poc2 fppoc
WCS2=	fpevent fppwcs fppwcs_dual hdcwcs load_diags start_fpp wcs wcs_dual

all clean cleandir depend etc install lint:

#	(cd ${DESTDIR}/etc; \
#	    mkpasswd -p master.passwd; \
#	    mv master.passwd.pag passwd.pag; \
#	    mv master.passwd.dir passwd.dir; \
#	    mv master.passwd.orig passwd)

distribution:
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN} ${DESTDIR}/etc
	install -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	(cd root; \
		install -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		install -c -o root -g wheel -m 644 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		install -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		install -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${MTREE} \
	    ${DESTDIR}/etc/mtree
	cd namedb; install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
	    ${DESTDIR}/etc/namedb
	cd kerberosIV; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${KRB} \
	    ${DESTDIR}/etc/kerberosIV
	install -c -o ${BINOWN} -g operator -m 664 /dev/null \
	    ${DESTDIR}/etc/dumpdates
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/etc/sendmail.cw
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/messages
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/maillog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lpd-errs
	install -c -o ${BINOWN} -g ${UTMPGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/run/utmp
	install -c -o ${BINOWN} -g ${UTMPGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/cron
	install -c -o nobody -g nogroup -m 664 /dev/null \
	    ${DESTDIR}/var/db/locate.database
	(cd etc.${MACHINE}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    fstab.* ${DESTDIR}/etc)
	(cd amd_samples; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    start-amd amd-* ${DESTDIR}/etc/amd_samples)
.if ${MACHINE} == "tahoe"
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS1} \
	    ${DESTDIR}/)
.endif
.if ${MACHINE} == "vax"
	(cd etc.vax; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)
.endif

hcx9-distribution:
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS2} \
	    ${DESTDIR}/)

.include <bsd.prog.mk>
