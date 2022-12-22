#	BSDI $Id: Makefile,v 1.3 1994/01/24 16:54:50 polk Exp $

PROG=	faxd
MAN8=	faxd.0
SRCS=	Class1.c++ Class1Poll.c++ Class1Recv.c++ Class1Send.c++ Class2.c++ \
	Class2Params.c++ Class2Poll.c++ Class2Recv.c++ Class2Send.c++ \
	Everex.c++ EverexPoll.c++ EverexRecv.c++ EverexSend.c++ \
	FaxMachineInfo.c++ FaxMachineLog.c++ FaxModem.c++ FaxPoll.c++ \
	FaxRecv.c++ FaxRecvInfo.c++ FaxRequest.c++ FaxSend.c++ FaxServer.c++ \
	Getty.c++ GettyBSD.c++ HDLCFrame.c++ ModemConfig.c++ UUCPLock.c++ \
	faxServerApp.c++ tif_compress.c
CFLAGS+=${FAXINCS} ${FAXUUCP}
C++FLAGS+=${FAXINCS} ${FAXUUCP}
LDCC=	g++
DPADD=	${FAXLIBS} ${LIBUTIL} ${LIBM}
LDADD=	${FAXLIBS} -lutil -lm
MKDEP=	mkdep -2

BINDIR=	${SBINDIR}

${PROG}.8: ${.CURDIR}/../man/${PROG}.1m
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.8

afterinstall:
	[ -f ${DESTDIR}${SPOOLDIR}/sendq/seqf ] || \
		cp /dev/null ${DESTDIR}${SPOOLDIR}/sendq/seqf
	chown uucp.uucp ${DESTDIR}${SPOOLDIR}/sendq/seqf
	chmod 0644 ${DESTDIR}${SPOOLDIR}/sendq/seqf

.include <bsd.prog.mk>
