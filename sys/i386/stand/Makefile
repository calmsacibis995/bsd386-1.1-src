# Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI $Id: Makefile,v 1.19 1994/01/24 18:10:45 polk Exp $
#
# from:
#	@(#)Makefile	7.9 (Berkeley) 5/8/91


DESTDIR=/
STAND=	../../stand
INCPATH=-I../../sys -I../../  -I${STAND} -I.
VPATH=	${STAND}
STANDDIR= ${DESTDIR}/stand
MDEC= ${DESTDIR}/usr/bootstraps

CPP=	cpp ${INCPATH} -DLOCORE -DSTANDALONE -DAT386

# Block-0 bootstraps are loaded by BIOS at BIOSRELOC, and run there.
# Second-stage bootstraps are loaded at SMRELOC, and run there.
# Boot and other programs are loaded at 0; boot relocates to BRELOC.
BRELOC=	1E0000
SMRELOC= 80200
BIOSRELOC=7c00

DEFAULTBOOT="/bsd"
CFLAGS=	-DSTANDALONE -DAT386 '-DDEFAULTBOOT=${DEFAULTBOOT}' -O ${INCPATH}

DRIVERS=cga.c fd.c kbd.c wd.c sd.c aha.c eaha.c
SRCS=	ahaboot.s boot.c bootxx.c conf.c confxx.c eahaboot.s fd.c fdboot.s \
	dos_disksubr.c prf.c srt0.s wdboot.s ${DRIVERS} ${SASRC}
LIBS=	libsa/libsa.a libdrive.a -lc
SMLIBS=	libsmsa/libsmsa.a libdrive.a -lc

BOOTBLOCKS= wdboot bootwd genboot fdboot fd5boot bootfd \
	 ahaboot bootaha eahaboot booteaha
DISKUTILS= boot flboot copy cat ls
ALL=	${BOOTBLOCKS} ${DISKUTILS}

all: machine ${ALL}

machine:
	ln -s ../include machine

libsa/libsa.a::
	cd libsa; make ${MFLAGS}
libsmsa/libsmsa.a::
	cd libsmsa; make ${MFLAGS}

libdrive.a: conf.o prf.o dos_disksubr.o ${DRIVERS:.c=.o}
	ar crv $@ $?
	ranlib $@

# startups

srt0.o: srt0.s
	${CPP} -DSMRELOC=0x${SMRELOC} srt0.s | ${AS} -o srt0.o

wsrt0.o: srt0.s
	${CPP} -DSMALL -DSMRELOC=0x${SMRELOC} -DREL srt0.s | \
	    ${AS} -o wsrt0.o

relsrt0.o: srt0.s
	${CPP} -DBRELOC=0x${BRELOC} -DREL srt0.s | ${AS} -o relsrt0.o

# block 0 boots

wdboot.o: wdboot.s
	${CPP} -DSMRELOC=0x${SMRELOC} wdboot.s | ${AS} -o $@

fdboot.o: fdboot.s 
	${CPP} -DSMRELOC=0x${SMRELOC} fdboot.s | ${AS} -o $@

fd5boot.o: fdboot.s 
	${CPP} -DSMRELOC=0x${SMRELOC} -DFD5BOOT fdboot.s | ${AS} -o $@

ahaboot.o: ahaboot.s 
	${CPP} -DSMRELOC=0x${SMRELOC} ahaboot.s | ${AS} -o $@

eahaboot.o: eahaboot.s
	${CPP} -DSMRELOC=0x${SMRELOC} eahaboot.s | ${AS} -o $@

genboot.o: genboot.s
	${CPP} -DLOCORE -DRELOC=0x80000 -DHARD genboot.s | ${AS} -o $@

# bootable from floppy or real disks

boot:	boot.o bootconf.o cga.o relsrt0.o ${LIBS}
	ld -e __zero -N -T ${BRELOC} -o $@ relsrt0.o boot.o bootconf.o cga.o \
		${LIBS}

boot.o: boot.c
	${CC} -c ${CFLAGS} -DBRELOC=0x${BRELOC} boot.c

# boot for floppy that allows disk change after loading

flboot:	fl_boot.o bootconf.o cga.o relsrt0.o ${LIBS}
	ld -e __zero -N -T ${BRELOC} -o $@ relsrt0.o fl_boot.o bootconf.o cga.o \
		${LIBS}

bootconf.o: conf.o
	ln -s conf.c bootconf.c
	${CC} -c ${CFLAGS} -DBOOT bootconf.c
	rm -f bootconf.c

fl_boot.o: boot.c
	rm -f fl_boot.c
	ln -s boot.c fl_boot.c
	${CC} -c -DCHANGEFLOPPY ${CFLAGS} -DBRELOC=0x${BRELOC} fl_boot.c
	rm -f fl_boot.c

# utilities

cat:	cat.o srt0.o conf.o ${LIBS}
	ld -N -o $@ srt0.o cat.o conf.o ${LIBS}

ls:	ls.o srt0.o conf.o ${LIBS}
	ld -N -o $@ srt0.o ls.o conf.o ${LIBS}

copy:	copy.o srt0.o conf.o ${LIBS}
	ld -N -o $@ srt0.o copy.o conf.o ${LIBS}

# getting booted from disc

wdboot: wdboot.o
	ld -N -T ${BIOSRELOC} wdboot.o
	cp a.out wdb
	rm -f wdboot
	strip a.out; dd if=a.out of=wdboot ibs=32 skip=1; rm -f a.out
	ls -l wdboot

bootxx.o: machine

bootwd: wsrt0.o bootxx.o sm_confwd.o sm_wd.o sm_cga.o ${SMLIBS}
	ld -N -T ${SMRELOC} wsrt0.o bootxx.o sm_confwd.o sm_wd.o \
	    sm_cga.o ${SMLIBS}
	size a.out
	cp a.out bwd
	rm -f bootwd
	strip a.out; dd if=a.out of=bootwd ibs=32 skip=1; rm -f a.out
	ls -l bootwd

sm_confwd.o: confwd.c
	rm -f sm_confwd.c
	ln -s confwd.c sm_confwd.c
	${CC} -c -DSMALL ${CFLAGS} sm_confwd.c
	rm -f sm_confwd.c

fdboot: fdboot.o
	ld -N -T ${BIOSRELOC} fdboot.o
	cp a.out fdb
	rm -f fdboot
	strip a.out; dd if=a.out of=fdboot ibs=32 skip=1; rm -f a.out
	ls -l fdboot

fd5boot: fd5boot.o
	ld -N -T ${BIOSRELOC} fd5boot.o
	cp a.out fd5b
	rm -f fd5boot
	strip a.out; dd if=a.out of=fd5boot ibs=32 skip=1; rm -f a.out
	ls -l fd5boot

bootfd: wsrt0.o bootxx.o sm_fd.o sm_cga.o sm_conffd.o ${SMLIBS}
	ld -N -T ${SMRELOC} wsrt0.o bootxx.o sm_fd.o sm_cga.o sm_conffd.o ${SMLIBS}
	size a.out
	cp a.out bfd
	rm -f bootfd
	strip a.out; dd if=a.out of=bootfd ibs=32 skip=1; rm -f a.out
	ls -l bootfd

sm_conffd.o: conffd.c
	rm -f sm_conffd.c
	ln -s conffd.c sm_conffd.c
	${CC} -c -DSMALL ${CFLAGS} sm_conffd.c
	rm -f sm_conffd.c

ahaboot: ahaboot.o
	ld -N -T ${BIOSRELOC} ahaboot.o
	cp a.out ahab
	rm -f ahaboot
	strip a.out; dd if=a.out of=ahaboot ibs=32 skip=1; rm -f a.out
	ls -l ahaboot

bootaha: wsrt0.o bootxx.o sm_sd.o sm_aha.o sm_cga.o sm_confsd.o ${SMLIBS}
	ld -N -T ${SMRELOC} wsrt0.o bootxx.o sm_sd.o sm_aha.o sm_cga.o \
		sm_confsd.o ${SMLIBS}
	size a.out
	cp a.out baha
	rm -f bootaha
	strip a.out; dd if=a.out of=bootaha ibs=32 skip=1; rm -f a.out
	ls -l bootaha

eahaboot: eahaboot.o
	ld -N -T ${BIOSRELOC} eahaboot.o
	cp a.out eahab
	rm -f eahaboot
	strip a.out; dd if=a.out of=eahaboot ibs=32 skip=1; rm -f a.out
	ls -l eahaboot

booteaha: wsrt0.o bootxx.o sm_sd.o sm_eaha.o sm_cga.o sm_confsd.o ${SMLIBS}
	ld -N -T ${SMRELOC} wsrt0.o bootxx.o sm_sd.o sm_eaha.o sm_cga.o \
		sm_confsd.o ${SMLIBS}
	size a.out
	cp a.out beaha
	rm -f booteaha
	strip a.out; dd if=a.out of=booteaha ibs=32 skip=1; rm -f a.out
	ls -l booteaha

genboot: genboot.o
	${LD} -N -T ${BIOSRELOC} genboot.o
	@echo "check: bootbase == 7c00; ebootblkcode <= 7dfe; edata == 7e00"
	@nm -p a.out | grep bootbase
	@nm -p a.out | grep ebootblkcode
	@nm -p a.out | grep edata
	@nm -p a.out | grep ' _end'
	cp a.out genb
	rm -f genboot
	strip a.out; dd if=a.out of=genboot ibs=32 skip=1; rm -f a.out
	ls -l genboot

sm_confsd.o: confsd.c
	rm -f sm_confsd.c
	ln -s confsd.c sm_confsd.c
	${CC} -c -DSMALL ${CFLAGS} sm_confsd.c
	rm -f sm_confsd.c

sm_cga.o: cga.c
	rm -f sm_cga.c
	ln -s cga.c sm_cga.c
	${CC} -c -DSMALL ${CFLAGS} sm_cga.c
	rm -f sm_cga.c

sm_wd.o: wd.c
	rm -f sm_wd.c
	ln -s wd.c sm_wd.c
	${CC} -c -DSMALL ${CFLAGS} sm_wd.c
	rm -f sm_wd.c

sm_fd.o: fd.c
	rm -f sm_fd.c
	ln -s fd.c sm_fd.c
	${CC} -c -DSMALL ${CFLAGS} sm_fd.c
	rm -f sm_fd.c

sm_sd.o: sd.c
	rm -f sm_sd.c
	ln -s sd.c sm_sd.c
	${CC} -c -DSMALL ${CFLAGS} sm_sd.c
	rm -f sm_sd.c

sm_aha.o: aha.c
	rm -f sm_aha.c
	ln -s aha.c sm_aha.c
	${CC} -c -DSMALL ${CFLAGS} sm_aha.c
	rm -f sm_aha.c

sm_eaha.o: eaha.c
	rm -f sm_eaha.c
	ln -s eaha.c sm_eaha.c
	${CC} -c -DSMALL ${CFLAGS} sm_eaha.c
	rm -f sm_eaha.c

confwd.o: confwd.c confxx.o
conffd.o: conffd.c confxx.o

confxx.o:
	touch confxx.o

confwd.c: confxx.c
	rm -f confwd.c
	sed -e 's/io->i_dev/0/g'		< ${STAND}/dev.c > confwd.c
	sed -e 's/XX/wd/' -e 's/xx/wd/g'	< confxx.c >> confwd.c

conffd.c: confxx.c
	rm -f conffd.c
	sed -e 's/io->i_dev/0/g'		< ${STAND}/dev.c > conffd.c
	sed -e 's/XX/fd/' -e 's/xx/fd/g'	< confxx.c >> conffd.c

confsd.c: confxx.c
	rm -f confsd.c
	sed -e 's/io->i_dev/0/g'		< ${STAND}/dev.c > confsd.c
	sed -e 's/XX/sd/' -e 's/xx/sd/g'	< confxx.c >> confsd.c

clean:
	rm -f *.o *.exe *.i sm_*.c libdrive.a
	rm -f a.out bfd bwd baha beaha fdb fd5b wdb ahab eahab ${ALL}
	rm -f boot[a-wyz]? boot[a-wyz]?? boot[a-wyz]?.c boot[a-wyz]??.c \
		conf[a-wyz]?.c conf[a-wyz]??.c
	rm -f format core sboot bootconf.c
	cd libsa; make ${MFLAGS} clean
	cd libsmsa; make ${MFLAGS} clean

cleandir: clean
	rm -f ${MAN} tags .depend
	cd libsa; make ${MFLAGS} cleandir
	cd libsmsa; make ${MFLAGS} cleandir

depend: ${SRCS}
	mkdep ${INCPATH} -DSTANDALONE ${SRCS} ${DUMMIES}
	cd libsa; make ${MFLAGS} depend
	cd libsmsa; make ${MFLAGS} depend

install: ${ALL}
	test -d ${MDEC} || mkdir -p ${MDEC}
	test -d ${STANDDIR} || mkdir -p ${STANDDIR}
	install -c -s boot ${DESTDIR}
	cp ${BOOTBLOCKS} ${MDEC}
	cp ${DISKUTILS} ${STANDDIR}
