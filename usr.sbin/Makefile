#	@(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	BSDI $Id: Makefile,v 1.18 1994/01/05 02:33:46 torek Exp $

# Missing:
#	ac mkproto quot 

SUBDIR=	accton amd arp chown chroot config configsl cron dev_mkdb \
	diskpart edquota gettable htable inetd installsw iostat \
	kgmon kvm_mkdb lpr mrouted mtree named portmap pwd_mkdb \
	quotaon repquota rmt rstatd rwhod sa sendmail sliplogin \
	syslogd tcpdump traceroute trpt trsp update vipw xntp

.if   make(clean) || make(cleandir)
SUBDIR+=dlmpcc flcopy rxformat timed
.elif ${MACHINE} == "hp300"
SUBDIR+=timed
.elif ${MACHINE} == "i386"
SUBDIR+=dosfirstpage ioport msconfig timed
.elif ${MACHINE} == "tahoe"
SUBDIR+=dlmpcc timed
.elif ${MACHINE} == "vax"
SUBDIR+=flcopy implog implogd rxformat timed
.endif

.include <bsd.subdir.mk>
