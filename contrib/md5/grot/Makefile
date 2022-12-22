#/bin/make
# @(#)Makefile	6.5 1/11/94 01:49:58
#
# hash - combined shs and md5 makefile
#
# This makefile was written by:
#
#	 Landon Curt Noll  (chongo@toad.com)	chongo <was here> /\../\
#
# This makefile has been placed in the public domain.  Please do not 
# copyright this makefile.
#
# LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH  REGARD  TO
# THIS  SOFTWARE,  INCLUDING  ALL IMPLIED WARRANTIES OF MER-
# CHANTABILITY AND FITNESS.  IN NO EVENT SHALL  LANDON  CURT
# NOLL  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM  LOSS  OF
# USE,  DATA  OR  PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# See shsdrvr.c and md5drvr.c for version and modification history.

SHELL= /bin/sh
CFLAGS= ${PROTO} ${OPT_DEBUG}
TR= tr
DIFF= diff
INSTALL= install
DESTDIR= /usr/local/bin
DESTLIBTOP= /usr/local/lib
SHSLIB= ${DESTLIBTOP}/shs
MD5LIB= ${DESTLIBTOP}/md5
DESTMAN= /usr/local/man/man1
MANEXT= 1

# select the proper compiler
CC= gcc -ansi -fomit-frame-pointer -fcaller-saves
#CC= gcc -ansi
#CC= gcc
#CC= cc

# select the proper optimization or debug level
#
OPT_DEBUG= -O
#OPT_DEBUG= -g

# Some architectures such as Sparc do not allow one to access long that
# are not alligned.  If make test dumpds core or fails to produce no
# output, use only -DMUST_ALIGN.
#
# ALIGN=		let must_align.c figure out if alignment is needed
# ALIGN=-DMUST_ALIGN 	force alignment (at the cost of performance}
# ALIGN=-UMUST_ALIGN	allow non-aligment (usually at a performance gain}
#
ALIGN=
#ALIGN=-DMUST_ALIGN
#ALIGN=-UMUST_ALIGN

# comment out the following line if your compiler doesn't support prototyping
#
PROTO=-DPROTO

all: shs md5

shs: shsdrvr.o shsdual.o shs.o
	${CC} ${CFLAGS} -o shs shsdrvr.o shsdual.o shs.o

md5: md5drvr.o md5dual.o md5.o
	${CC} ${CFLAGS} -o md5 md5drvr.o md5dual.o md5.o

# perform all tests
test: shs md5 shstest md5test

# perform the extended shs digest test
shstest: shs file1 file2 file3 shs.data shs.ddata Makefile
	@echo This shs test should produce no output
	@./shs -x | ${DIFF} - shs.data
	@./shs shs.data | \
	    sed '/^2dd09d381d4c3b73b4857ca4cce5c6f1b492c2f6 shs.data$$/d'
	@./shs -q -c file3 | \
	    sed '/^0xa9f2c8b901a7c8628133694a105fc760bdccad2c$$/d'
	@./shs -p abc -c file2 | \
	    sed '/^0xa9f2c8b901a7c8628133694a105fc760bdccad2c file2$$/d'
	@./shs -P file1 file2 | \
	    sed '/^a9f2c8b901a7c8628133694a105fc760bdccad2c file2$$/d'
	@./shs shs.ddata | \
	    sed '/^7f5fc1a68572ef19c29dab020eadce290633772b shs.ddata$$/d'
	@./shs -d -x -q | ${DIFF} - shs.ddata

# perform the extended md5 digest test
md5test: md5 file1 file2 file3 md5.data md5.ddata Makefile
	@echo This md5 test should produce no output
	@./md5 -x | ${DIFF} - md5.data
	@./md5 md5.data | \
	    sed '/^9638425fd6b565e9fddf642fa4889125 md5.data$$/d'
	@./md5 -q -c file3 | \
	    sed '/^0x68cfe1a8e5242c8e1e8152d0bc710631$$/d'
	@./md5 -p abc -c file2 | \
	    sed '/^0x68cfe1a8e5242c8e1e8152d0bc710631 file2$$/d'
	@./md5 -P file1 file2 | \
	    sed '/^68cfe1a8e5242c8e1e8152d0bc710631 file2$$/d'
	@./md5 md5.ddata | \
	    sed '/^78a3db06b2ea6d77fac09aa8b8afe959 md5.ddata$$/d'
	@./md5 -d -x -q | ${DIFF} - md5.ddata

file1: Makefile
	rm -f file1
	echo abc | ${TR} -d '\012' > file1

file3: file1 file2
	rm -f file3
	cat file1 file2 > file3

shsdrvr.o: shsdrvr.c shs.h Makefile
	${CC} ${CFLAGS} -DTLIB=\"${SHSLIB}\" -c shsdrvr.c

shsdual.o: shsdual.c shs.h Makefile
	${CC} ${CFLAGS} -DTLIB=\"${SHSLIB}\" -c shsdual.c

shs.o: shs.c shs.h align.h endian.h Makefile
	${CC} ${CFLAGS} ${LOOPS} -c shs.c

md5drvr.o: md5drvr.c md5.h Makefile
	${CC} ${CFLAGS} -DTLIB=\"${MD5LIB}\" -c md5drvr.c

md5dual.o: md5dual.c md5.h Makefile
	${CC} ${CFLAGS} -DTLIB=\"${MD5LIB}\" -c md5dual.c

md5.o: md5.c md5.h align.h endian.h Makefile
	${CC} ${CFLAGS} -c md5.c

align.h: must_align
	rm -f core core.must_align align.h
	@echo "=-=-= IT IS OK IF must_align DUMPS CORE =-=-="
	-./must_align > align.h 2>/dev/null
	rm -f core core.must_align

must_align: must_align.c
	${CC} ${CFLAGS} ${ALIGN} must_align.c -o must_align

endian.h: endian
	rm -f endian.h
	./endian > endian.h

endian: endian.c
	${CC} ${CFLAGS} endian.c -o endian

install: all file1 file2 shs.data md5.data shs.1 md5.1
	-rm -f ${DESTDIR}/shs
	${INSTALL} -c -m 0555 shs ${DESTDIR}
	-rm -f ${DESTDIR}/md5
	${INSTALL} -c -m 0555 md5 ${DESTDIR}
	-@if [ ! -d "${DESTLIBTOP}" ]; then \
	    echo "mkdir ${DESTLIBTOP}"; \
	    mkdir "${DESTLIBTOP}"; \
	fi
	-@if [ ! -d "${SHSLIB}" ]; then \
	    echo "mkdir ${SHSLIB}"; \
	    mkdir "${SHSLIB}"; \
	fi
	${INSTALL} -c -m 0444 file1 ${SHSLIB}
	${INSTALL} -c -m 0444 file2 ${SHSLIB}
	${INSTALL} -c -m 0444 shs.data ${SHSLIB}
	-@if [ ! -d "${MD5LIB}" ]; then \
	    echo "mkdir ${MD5LIB}"; \
	    mkdir "${MD5LIB}"; \
	fi
	${INSTALL} -c -m 0444 file1 ${MD5LIB}
	${INSTALL} -c -m 0444 file2 ${MD5LIB}
	${INSTALL} -c -m 0444 md5.data ${MD5LIB}
	${INSTALL} -c -m 0444 shs.1 ${DESTMAN}/shs.${MANEXT}
	${INSTALL} -c -m 0444 md5.1 ${DESTMAN}/md5.${MANEXT}

clean:
	rm -f shs.o shsdrvr.o shsdual.o 
	rm -f md5.o md5drvr.o md5dual.o 
	rm -f file1 file3 
	rm -f endian.h endian.o endian
	rm -f align.h must_align must_align.o
	rm -f core core.must_align core.shs core.md5

clobber: clean
	rm -f shs md5
