DESTDIR=/
MDEC=	${DESTDIR}/usr/bootstraps

bootany.sys: bootany.exe
	doscmd exe2bin bootany bootany.sys

bootany.exe: bootany.obj
	doscmd link /noi /noe /farcall bootany,bootany.exe\;

bootany.obj: bootany.asm bootany.inc
	doscmd masm bootany,bootany,bootany,bootany\;

clean:
	rm -f bootany.obj bootany.sys bootany.lst bootany.crf bootany.exe

install: bootany.sys
	cp bootany.sys ${MDEC}
