#	BSDI $Id: Makefile,v 1.3 1993/03/08 16:42:06 polk Exp $

LIB=	cdrom
SRCS=	common.c misc.c scsi2.c toshiba.c panasonic.c mitsumi.c susp.c
MAN3=	cdrom.0
MLINKS+=cdrom.3 cdopen.3 cdrom.3 cdclose.3 cdrom.3 cdplay.3 cdrom.3 cdstop.3 \
	cdrom.3 cdstatus.3 cdrom.3 cdeject.3 cdrom.3 cdvolume.3 \
	cdrom.3 frame_to_msf.3 cdrom.3 msf_to_frame.3

CFLAGS+=	-I/sys

.include <bsd.lib.mk>
