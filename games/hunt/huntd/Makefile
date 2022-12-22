# BSDI	$Id: Makefile,v 1.1.1.1 1992/11/22 23:54:45 polk Exp $
#
#  Hunt
#  Copyright (c) 1985 Conrad C. Huang, Gregory S. Couch, Kenneth C.R.C. Arnold
#  San Francisco, California
#
#  Copyright (c) 1985 Regents of the University of California.
#  All rights reserved.  The Berkeley software License Agreement
#  specifies the terms and conditions for redistribution.
#
# Game parameter flags are:
#	RANDOM	Include doors which disperse shots randomly
#	REFLECT	Include diagonal walls that reflect shots
#	MONITOR	Include code for watching the game from the sidelines
#	OOZE	Include slime shots
#	FLY	Make people fly when walls regenerate under them
#	VOLCANO	Include occasional large slime explosions
#	DRONE	Include occasional drone shots
#	BOOTS	Include boots (which makes you immune to slime)
#	OTTO	Reserved for CGL automatic player
#
PROG=	huntd
SRCS= answer.c draw.c driver.c execute.c expl.c extern.c makemaze.c \
	shots.c terminal.c pathname.c
MAN6=	huntd.0
CFLAGS+=	-DRANDOM -DREFLECT -DMONITOR -DOOZE -DFLY -DBOOTS -DOTTO -DVOLCANO
CFLAGS+=	-DINTERNET -DLOG -DBSD_RELEASE=44 -DBSD386
CFLAGS+=	-traditional -I. -I${.CURDIR} 
	
.PATH: ${.CURDIR}/../hunt

.include "../../Makefile.inc"
BINDIR=${LIBDIR}

.include <bsd.prog.mk>
