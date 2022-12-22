#	@(#)Makefile	5.1.1.2 (Berkeley) 5/9/91

# contrib is omitted since some packages are gzipped on the CDROM

SUBDIR=	include lib bin games libexec old sbin share \
	usr.bin usr.sbin

.include <bsd.subdir.mk>
