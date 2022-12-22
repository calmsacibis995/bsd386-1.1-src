#
# help - makefile for calc help files
#
# Copyright (c) 1993 David I. Bell and Landon Curt Noll
# Permission is granted to use, distribute, or modify this source,
# provided that this copyright notice remains intact.
#
# Arbitrary precision calculator.
#
# calculator by David I. Bell
# makefile by Landon Curt Noll

SHELL= /bin/sh

# Normally, the upper level makefile will set these values.  We provide
# a default here just in case you want to build from this directory.
#
# where to install things
HELPDIR= /usr/local/lib/calc/help
# the usually LIBDIR value from the upper level Makefile
LIBDIR= /usr/local/lib/calc

# how to build a directory
MKDIR=mkdir -p
#MKDIR=mkdir

# standard tools
SED= sed

# Standard help files
#
STD_HELP_FILES=intro command expression define variable statement \
	operator types mat list file builtin config interrupt \
	history usage credit bindings assoc \
	overview stdlib environment todo credit help

# Help files that are constructed from other sources
#
BUILT_HELP_FILES=bindings stdlib changes libcalc

all: ${STD_HELP_FILES} obj.file ${BUILT_HELP_FILES}

bindings: ../lib/bindings
	rm -f bindings
	cp ../lib/bindings bindings
	chmod 0444 bindings

stdlib: ../lib/README
	rm -f stdlib
	cp ../lib/README stdlib
	chmod 0444 stdlib

changes: ../CHANGES
	rm -f changes
	cp ../CHANGES changes
	chmod 0444 changes

libcalc: ../LIBRARY
	rm -f libcalc
	${SED} -e 's:$${LIBDIR}:${LIBDIR}:g' < ../LIBRARY > libcalc
	chmod 0444 libcalc

clean:

clobber:
	rm -f ${BUILT_HELP_FILES}

install: all
	-@if [ ! -d ${HELPDIR} ]; then \
		echo ${MKDIR} ${HELPDIR}; \
		${MKDIR} ${HELPDIR}; \
	fi
	@for i in ${STD_HELP_FILES} ${BUILT_HELP_FILES}; do \
		echo rm -f ${HELPDIR}/$$i; \
		rm -f ${HELPDIR}/$$i; \
		echo cp $$i ${HELPDIR}; \
		cp $$i ${HELPDIR}; \
		echo chmod 0444 ${HELPDIR}/$$i; \
		chmod 0444 ${HELPDIR}/$$i; \
	done
	rm -f ${HELPDIR}/obj
	cp obj.file ${HELPDIR}/obj
	chmod 0444 ${HELPDIR}/obj
