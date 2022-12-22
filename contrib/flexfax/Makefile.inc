#	BSDI $Id: Makefile.inc,v 1.4 1994/01/30 20:46:36 polk Exp $

# FlexFAX installation directories.  Note, util/config.h contains some
# of these as well (we really should get them through -D flags, but...).
# (${SPOOLBINDIR} was not my idea, either.)
AFMDIR?=	/usr/contrib/lib/flexfax/afm
BINDIR?=	/usr/contrib/bin
SBINDIR?=	/usr/contrib/bin
MANDIR?=	/usr/contrib/man/cat
MANSRCDIR?=	/usr/contrib/man/man
DOCDIR?=	/usr/contrib/lib/flexfax
LIBDATADIR?=	/usr/contrib/lib/flexfax
LIBEXECDIR?=	/usr/contrib/lib/flexfax
SPOOLDIR?=	/var/spool/flexfax
SPOOLBINDIR?=	/var/spool/flexfax/bin
SPOOLETC?=	/var/spool/flexfax/etc
FONTDIR?=	/usr/contrib/lib/flexfax/afm

# -I and -D flags for everyone
FAXINCS=-I${.CURDIR}/.. -I${.CURDIR}/../util -I${.CURDIR}/../libtiff/obj \
	-I${.CURDIR}/../libtiff -I${.CURDIR}/../iv -I${.CURDIR}/../port/bsdi \
	-I${.CURDIR}/../port/generic -D__ANSI_CPP__

# UUCP locking info
FAXUUCP=-DUUCP_LOCKTYPE=1 -DUUCP_LOCKMODE=0644 \
	-DUUCP_LOCKDIR=\"/var/spool/uucp\"

# Libraries for everyone
FAXLIBS=${.CURDIR}/../util/obj/libutil.a \
	${.CURDIR}/../libtiff/obj/libtiff.a \
	${.CURDIR}/../iv/obj/libivhack.a \
	${.CURDIR}/../port/bsdi/obj/libport.a

# man page massaging
MANSED=	-e "s;\$${AFMDIR};${AFMDIR};g" \
	-e "s;\$${BIN};${BINDIR};g" \
	-e "s;\$${LIBDATA};${LIBDATADIR};g" \
	-e "s;\$${LIBEXEC};${LIBEXECDIR};g" \
	-e "s;\$${SPOOL};${SPOOLDIR};g" \
	-e "s;\$${USRETC};/etc;g" \
	-e s/1M/8/ -e s/4F/5/
