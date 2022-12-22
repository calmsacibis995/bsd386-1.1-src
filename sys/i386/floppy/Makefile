#	@(#)Makefile	5.6 (Berkeley) 5/20/91

PROTOBIN?=bin

SUBDIR=	cat compress cp dd diskdefect disklabel fsck ftp ifconfig \
	init installsw ls mkdir mknod mount mt mv newfs ping pwd restore \
	rm route rsh scsicmd sh slattach stty sync tar telnet umount vi \
	mount_iso9660 disksetup tail pax gzip ln test chmod

.if make(clean) || make(cleandir)
SUBDIR+=bin
.endif

install:
	for i in ${SUBDIR}; do \
		cp $$i/obj/$$i ${PROTOBIN}/$$i; \
	done
	cd bin; strip [a-z]*

.include <bsd.subdir.mk>

