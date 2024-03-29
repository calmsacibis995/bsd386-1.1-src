#	@(#)Makefile	7.13 (Berkeley) 5/9/91

# Makefile for kernel tags files, init_systent, etc.

#ARCH=	vax tahoe hp300 i386
ARCH=	i386

all:
	@echo "make tags, make links or init_sysent.c only"

init_sysent.c syscalls.c ../sys/syscall.h: makesyscalls.sh syscalls.master
	-mv -f init_sysent.c init_sysent.c.bak
	-mv -f syscalls.c syscalls.c.bak
	-mv -f ../sys/syscall.h ../sys/syscall.h.bak
	sh makesyscalls.sh syscalls.master

# Kernel tags:
# We need to have links to tags files from the generic directories
# that are relative to the machine type, even via remote mounts;
# therefore we use symlinks to $SYSTAGS, which points at
# ${SYSDIR}/${MACHINE}/tags.

SYSTAGS=/var/db/sys_tags
SYSDIR=/sys

# Put the ../sys stuff near the end so that subroutine definitions win when
# there is a struct tag with the same name (eg., vmmeter).  The real
# solution would probably be for ctags to generate "struct vmmeter" tags.

COMM=	../iso9660/*.[ch] ../msdosfs/*.[ch] ../net/*.[ch] ../netimp/*.[ch] \
	../netinet/*.[ch] ../netns/*.[ch] ../netiso/*.[ch] ../netccitt/*.[ch] \
	../kern/*.c ../nfs/*.[ch] ../scsi/*.[ch] ../ufs/*.[ch] ../vm/*.[ch] \
	 ../sys/*.h

# Directories in which to place tags links (other than machine-dependent)
DGEN=	iso9660 kern msdosfs sys net netimp netinet netns netccitt nfs ufs vm

tags: FRC
	-for i in ${ARCH}; do \
	    cd ../$$i &&  make COMM="${COMM}" SRCDIR=${SYSDIR} ${MFLAGS} tags; \
	    done

links: ${SYSTAGS}
	-for i in ${DGEN}; do \
	    cd ../$$i && { rm -f tags; ln -s ${SYSTAGS} tags; } done
	-for i in ${ARCH}; do \
	    cd ../$$i && make ${MFLAGS} links; done

${SYSTAGS}:
	ln -s ${SYSDIR}/${MACHINE}/tags ${SYSTAGS}

FRC:
