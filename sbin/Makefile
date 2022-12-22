#	@(#)Makefile	5.4.1.1 (Berkeley) 5/7/91
#	BSDI $Id: Makefile,v 1.14 1994/01/15 00:54:58 polk Exp $

# Missing: icheck, ncheck

SUBDIR=	XNSrouted badsect chkconfig clri disklabel disksetup dmesg dump \
	dumpfs fastboot fsck halt ifconfig init mknod mount mount_iso9660 \
	mount_msdos mountd newfs nfsd nfsiod ping pppattach pppconfig \
	quotacheck reboot restore route routed savecore scsicmd shutdown \
	slattach startslip swapon tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+=bad144 drck enpload
.elif ${MACHINE} == "tahoe"
SUBDIR+=enpload
.elif ${MACHINE} == "vax"
SUBDIR+=diskdefect
.elif ${MACHINE} == "i386"
SUBDIR+=diskdefect
.endif

.include <bsd.subdir.mk>
