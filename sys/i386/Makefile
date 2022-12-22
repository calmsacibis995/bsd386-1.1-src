#	@(#)Makefile	7.3 (Berkeley) 6/9/91

# Makefile for i386 tags file

all:
	@echo "make tags or links only"

SRCDIR=/sys

TI386=	tags
SI386=	../i386/i386/*.[ch] ../i386/include/*.h ../i386/isa/*.[ch] \
	../i386/isa/ic/*.h ../i386/eisa/*.[ch]
AI386=	../i386/i386/*.s ../i386/isa/*.s

# Directories in which to place i386 tags links
DI386=	eisa i386 isa include

tags: FRC
	-ctags -dtf ${TI386} ${COMM} ${SI386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TI386}
	egrep "^_[a-zA-Z0-9_]*:" ${AI386} | \
	    sed "s;\([^:]*\):_\([a-zA-Z0-9_]*\):\(.*\);\2 \1 /^_\2:\3$$/;" \
	        >> ${TI386}
.ifndef	RELNAMES
	sed -e 's?../?${SRCDIR}/?' ${TI386} | sort -o ${TI386}
.endif

links:
	-for i in ${DI386}; do \
	    cd $$i && { rm -f tags; ln -s ../tags tags; cd ..; } done

FRC:
