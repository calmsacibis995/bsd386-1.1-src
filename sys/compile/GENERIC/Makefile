IDENT=-DGENERIC -Di386 -DDIAGNOSTIC -DFIFO -DQUOTA -DMSDOSFS -DISO9660 -DMFS -DNFS -DMULTICAST -DINET -DEISA -DSCSI -DFPE -DCOMPAT_43 -DKTRACE
PARAM=-DTIMEZONE=0 -DDST=0 -DMAXUSERS=10
DEBUG=-g
#
# Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI $Id: Makefile.i386,v 1.16 1994/02/03 15:53:33 karels Exp $
#

# Copyright 1990 W. Jolitz
# @(#)Makefile.i386	7.1 5/10/91
# Makefile for 4.3 BSD-Reno
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/i386/conf/``machineid''
# after which you should do
#	 config machineid
# Generic makefile changes should be made in
#	/sys/i386/conf/Makefile.i386
# after which config should be rerun for all machines.
#
# N.B.: NO DEPENDENCIES ON FOLLOWING FLAGS ARE INVISIBLE TO MAKEFILE
#	IF YOU CHANGE THE DEFINITION OF ANY OF THESE RECOMPILE EVERYTHING
#
# -DTRACE	compile in kernel tracing hooks
# -DQUOTA	compile in file system quotas
#

# DEBUG is set to -g by config if debugging is requested (config -g).
# PROF is set to -pg by config if profiling is requested (config -p).
AS=	as ${DEBUG}
CC=	cc ${DEBUG}
CPP=	cpp
LD=	ld
TOUCH=	touch -f -c

# source tree is located via $S relative to the compilation directory
S=	../..
I386=	$S/i386

INCLUDES= -I. -I$S -I$S/sys
COPTS=	${INCLUDES} ${IDENT} -DKERNEL ${WARN}
CFLAGS=	-O ${COPTS}

# compile rules: rules are named ${TYPE}_${SUFFIX}${CONFIG_DEP}
# where TYPE is NORMAL, DRIVER, or PROFILE}; SUFFIX is the file suffix,
# capitalized (e.g. C for a .c file), and CONFIG_DEP is _C if the file
# is marked as config-dependent.

NORMAL_C=	${CC} -c ${CFLAGS} ${PROF} $<
NORMAL_C_C=	${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

DRIVER_C=	${CC} -c ${CFLAGS} ${PROF} $<
DRIVER_C_C=	${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

PROFILE_C=	${CC} -c ${COPTS} $<

NORMAL_S=	${CPP} -DLOCORE ${COPTS} $< | ${AS} -o $@
NORMAL_S_C=	${CPP} -DLOCORE ${COPTS} ${PARAM} $< | ${AS} -o $@

OBJS=init_sysent.o kern_acct.o kern_descrip.o kern_exec.o kern_fork.o \
	kern_kinfo.o kern_ktrace.o kern_malloc.o kern_proc.o kern_prot.o \
	kern_resource.o kern_subr.o kern_time.o kern_xxx.o dead_vnops.o \
	spec_vnops.o fifo_vnops.o subr_autoconf.o subr_pdev.o subr_log.o \
	subr_prf.o subr_xxx.o sys_socket.o tty_compat.o tty_conf.o \
	tty_pty.o tty_tty.o vfs_bio.o vfs_cache.o vfs_conf.o \
	vfs_lookup.o vfs_subr.o vfs_vnops.o uipc_domain.o uipc_mbuf.o \
	uipc_proto.o uipc_socket.o uipc_socket2.o uipc_syscalls.o \
	uipc_usrreq.o kern_lock.o vm_fault.o vm_glue.o vm_init.o \
	vm_kern.o vm_map.o vm_meter.o vm_mmap.o vm_object.o vm_page.o \
	vm_pageout.o vm_pager.o vm_swap.o vm_unix.o vm_user.o \
	device_pager.o swap_pager.o vnode_pager.o ufs_alloc.o \
	ufs_lockf.o ufs_lookup.o ufs_subr.o ufs_tables.o ufs_vfsops.o \
	ufs_quota.o mfs_vnops.o mfs_vfsops.o iso9660_lookup.o \
	iso9660_node.o iso9660_util.o iso9660_vfsops.o iso9660_vnops.o \
	msdosfs_conv.o msdosfs_denode.o msdosfs_fat.o msdosfs_lookup.o \
	msdosfs_vfsops.o msdosfs_vnops.o nfs_bio.o nfs_node.o nfs_serv.o \
	nfs_socket.o nfs_srvcache.o nfs_subs.o nfs_syscalls.o \
	nfs_vfsops.o nfs_vnops.o af.o bpf.o bpf_filter.o if.o \
	if_ethersubr.o if_loop.o if_sl.o raw_cb.o raw_usrreq.o radix.o \
	route.o rtsock.o slcompress.o if_ether.o igmp.o in.o in_pcb.o \
	in_proto.o ip_icmp.o ip_input.o ip_output.o raw_ip.o tcp_debug.o \
	tcp_input.o tcp_output.o tcp_subr.o tcp_timer.o tcp_usrreq.o \
	udp_usrreq.o scsi_subr.o sd.o st.o init_main.o kern_clock.o \
	kern_exit.o kern_physio.o kern_sig.o kern_synch.o subr_rmap.o \
	sys_generic.o sys_process.o tty.o tty_subr.o vfs_syscalls.o \
	ufs_bmap.o ufs_disksubr.o ufs_inode.o ufs_vnops.o autoconf.o \
	conf.o cons.o copy.o dos_disksubr.o exec_machdep.o fpe_cmd.o \
	fpe_fmt.o fpe_main.o fpe_s.o i386_param.o in_cksum.o machdep.o \
	mem.o pmap.o process_machdep.o random.o segments.o sys_machdep.o \
	trap.o vm_machdep.o aha.o clock.o com.o fd.o if_ef.o if_el.o \
	if_eo.o if_ep.o if_ex.o if_ne.o if_pe.o if_petbl.o bms.o if_tn.o \
	if_we.o isa.o lms.o lp.o mcd.o npx.o pccons.o vga.o wd.o wt.o \
	xir.o eisa.o eaha.o 

CFILES=$S/kern/init_sysent.c $S/kern/kern_acct.c $S/kern/kern_descrip.c \
	$S/kern/kern_exec.c $S/kern/kern_fork.c $S/kern/kern_kinfo.c \
	$S/kern/kern_ktrace.c $S/kern/kern_malloc.c $S/kern/kern_proc.c \
	$S/kern/kern_prot.c $S/kern/kern_resource.c $S/kern/kern_subr.c \
	$S/kern/kern_time.c $S/kern/kern_xxx.c $S/kern/dead_vnops.c \
	$S/kern/spec_vnops.c $S/kern/fifo_vnops.c \
	$S/kern/subr_autoconf.c $S/kern/subr_pdev.c $S/kern/subr_log.c \
	$S/kern/subr_prf.c $S/kern/subr_xxx.c $S/kern/sys_socket.c \
	$S/kern/tty_compat.c $S/kern/tty_conf.c $S/kern/tty_pty.c \
	$S/kern/tty_tty.c $S/kern/vfs_bio.c $S/kern/vfs_cache.c \
	$S/kern/vfs_conf.c $S/kern/vfs_lookup.c $S/kern/vfs_subr.c \
	$S/kern/vfs_vnops.c $S/kern/uipc_domain.c $S/kern/uipc_mbuf.c \
	$S/kern/uipc_proto.c $S/kern/uipc_socket.c \
	$S/kern/uipc_socket2.c $S/kern/uipc_syscalls.c \
	$S/kern/uipc_usrreq.c $S/vm/kern_lock.c $S/vm/vm_fault.c \
	$S/vm/vm_glue.c $S/vm/vm_init.c $S/vm/vm_kern.c $S/vm/vm_map.c \
	$S/vm/vm_meter.c $S/vm/vm_mmap.c $S/vm/vm_object.c \
	$S/vm/vm_page.c $S/vm/vm_pageout.c $S/vm/vm_pager.c \
	$S/vm/vm_swap.c $S/vm/vm_unix.c $S/vm/vm_user.c \
	$S/vm/device_pager.c $S/vm/swap_pager.c $S/vm/vnode_pager.c \
	$S/ufs/ufs_alloc.c $S/ufs/ufs_lockf.c $S/ufs/ufs_lookup.c \
	$S/ufs/ufs_subr.c $S/ufs/ufs_tables.c $S/ufs/ufs_vfsops.c \
	$S/ufs/ufs_quota.c $S/ufs/mfs_vnops.c $S/ufs/mfs_vfsops.c \
	$S/iso9660/iso9660_lookup.c $S/iso9660/iso9660_node.c \
	$S/iso9660/iso9660_util.c $S/iso9660/iso9660_vfsops.c \
	$S/iso9660/iso9660_vnops.c $S/msdosfs/msdosfs_conv.c \
	$S/msdosfs/msdosfs_denode.c $S/msdosfs/msdosfs_fat.c \
	$S/msdosfs/msdosfs_lookup.c $S/msdosfs/msdosfs_vfsops.c \
	$S/msdosfs/msdosfs_vnops.c $S/nfs/nfs_bio.c $S/nfs/nfs_node.c \
	$S/nfs/nfs_serv.c $S/nfs/nfs_socket.c $S/nfs/nfs_srvcache.c \
	$S/nfs/nfs_subs.c $S/nfs/nfs_syscalls.c $S/nfs/nfs_vfsops.c \
	$S/nfs/nfs_vnops.c $S/net/af.c $S/net/bpf.c $S/net/bpf_filter.c \
	$S/net/if.c $S/net/if_ethersubr.c $S/net/if_loop.c \
	$S/net/if_sl.c $S/net/raw_cb.c $S/net/raw_usrreq.c \
	$S/net/radix.c $S/net/route.c $S/net/rtsock.c \
	$S/net/slcompress.c $S/netinet/if_ether.c $S/netinet/igmp.c \
	$S/netinet/in.c $S/netinet/in_pcb.c $S/netinet/in_proto.c \
	$S/netinet/ip_icmp.c $S/netinet/ip_input.c \
	$S/netinet/ip_output.c $S/netinet/raw_ip.c \
	$S/netinet/tcp_debug.c $S/netinet/tcp_input.c \
	$S/netinet/tcp_output.c $S/netinet/tcp_subr.c \
	$S/netinet/tcp_timer.c $S/netinet/tcp_usrreq.c \
	$S/netinet/udp_usrreq.c $S/scsi/scsi_subr.c $S/scsi/sd.c \
	$S/scsi/st.c $S/i386/i386/autoconf.c $S/i386/i386/conf.c \
	$S/i386/i386/cons.c $S/i386/i386/dos_disksubr.c \
	$S/i386/i386/exec_machdep.c $S/i386/i386/fpe_cmd.c \
	$S/i386/i386/fpe_fmt.c $S/i386/i386/fpe_main.c \
	$S/i386/i386/i386_param.c $S/i386/i386/in_cksum.c \
	$S/i386/i386/machdep.c $S/i386/i386/mem.c $S/i386/i386/pmap.c \
	$S/i386/i386/process_machdep.c $S/i386/i386/segments.c \
	$S/i386/i386/sys_machdep.c $S/i386/i386/trap.c \
	$S/i386/i386/vm_machdep.c $S/i386/isa/aha.c $S/i386/isa/clock.c \
	$S/i386/isa/com.c $S/i386/isa/fd.c $S/i386/isa/if_ef.c \
	$S/i386/isa/if_el.c $S/i386/isa/if_eo.c $S/i386/isa/if_ep.c \
	$S/i386/isa/if_ex.c $S/i386/isa/if_ne.c $S/i386/isa/bms.c \
	$S/i386/isa/if_tn.c $S/i386/isa/if_we.c $S/i386/isa/isa.c \
	$S/i386/isa/lms.c $S/i386/isa/lp.c $S/i386/isa/mcd.c \
	$S/i386/isa/npx.c $S/i386/isa/pccons.c $S/i386/isa/vga.c \
	$S/i386/isa/wd.c $S/i386/isa/wt.c $S/i386/eisa/eisa.c \
	$S/i386/eisa/eaha.c $S/i386/i386/swapgeneric.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ=	locore.o ${OBJS} param.o ioconf.o
#SYSTEM_DEP=	Makefile symbols.sort ${SYSTEM_OBJ}
SYSTEM_DEP=	Makefile ${SYSTEM_OBJ}
SYSTEM_LD_HEAD=	rm -f $@; \
		sh $S/conf/newvers.sh; \
		${CPP} -DLOCORE ${COPTS} ${I386}/i386/sysinfo.s | ${AS} -o sysinfo.o
SYSTEM_LD=	@strip=-x; [ X${DEBUG} = X-g ] && strip=-X || true; \
		echo ${LD} $$strip -n -T F0000000 -o $@ -e start '$${SYSTEM_OBJ}' sysinfo.o; \
		${LD} $$strip -n -T F0000000 -o $@ -e start ${SYSTEM_OBJ} sysinfo.o
#SYSTEM_LD_TAIL= @echo rearranging symbols;\
#		symorder symbols.sort $@;\
SYSTEM_LD_TAIL=	@size $@; chmod 755 $@; \
		[ X${DEBUG} = X-g ] && { \
		echo cp $@ $@.gdb; rm -f $@.gdb; cp $@ $@.gdb; \
		echo strip -d $@; strip -d $@; } || true

bsd: ${SYSTEM_DEP} swapgeneric.o newvers
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapgeneric.o
	${SYSTEM_LD_TAIL}

swapgeneric.o: $S/i386/i386/swapgeneric.c
	${NORMAL_C}

all: bsd

objs:	${OBJS}

newvers:	 #temp

clean:
	rm -f eddep *bsd bsd.gdb tags genassym *.o *.s \
		errs linterrs makelinks

lint: /tmp param.c
	@lint -hbxn -I. -DGENERIC -Dvolatile= ${COPTS} ${PARAM} \
	  ${I386}/i386/Locore.c ${CFILES} ioconf.c param.c | \
	    grep -v 'struct/union .* never defined' | \
	    grep -v 'possible pointer alignment problem'

symbols.sort: ${I386}/i386/symbols.raw
	grep -v '^#' ${I386}/i386/symbols.raw \
	    | sed 's/^	//' | sort -u > symbols.sort

locore.o: ${I386}/i386/locore.s assym.s machine/psl.h machine/pte.h \
    ${I386}/i386/specialreg.h $S/sys/syscall.h $S/sys/errno.h \
    machine/trap.h ${I386}/isa/isa.h ${I386}/isa/icu.h \
    ${I386}/isa/icu.s $S/net/netisr.h 
	${NORMAL_S}

copy.o: $S/i386/i386/copy.s assym.s machine/pte.h 

# depend on network configuration
uipc_proto.o in_proto.o ns_proto.o locore.o: Makefile

# depend on file system configuration
vfs_conf.o: Makefile

# depend on maxusers and config options
assym.s i386_param.o: Makefile

assym.s: genassym
	./genassym >assym.s

genassym:
	${CC} ${INCLUDES} -DKERNEL ${IDENT} ${PARAM} \
		 ${I386}/i386/genassym.c -o genassym

depend: assym.s param.c
	mkdep ${COPTS} ${CFILES} ioconf.c
	mkdep -a -p ${INCLUDES} ${IDENT} ${PARAM} ${I386}/i386/genassym.c

links:
	egrep '#if' ${CFILES} | sed -f $S/conf/defines | \
	  sed -e 's/:.*//' -e 's/\.c/.o/' | sort -u > dontlink
	echo ${CFILES} | tr -s ' ' '\12' | sed 's/\.c/.o/' | \
	  sort -u | comm -23 - dontlink | \
	  sed 's,../.*/\(.*.o\),rm -f \1;ln -s ../GENERIC/\1 \1,' > makelinks
	sh makelinks && rm -f dontlink

tags:
	@echo "see $S/kern/Makefile for tags"

ioconf.o: ioconf.c $S/sys/param.h machine/pte.h $S/sys/buf.h $S/sys/map.h \
    ${I386}/isa/isavar.h ${I386}/isa/isa.h ${I386}/isa/icu.h
	${CC} -c ${CFLAGS} ioconf.c

param.c: $S/conf/param.c
	-rm -f param.c
	cp $S/conf/param.c .

param.o: param.c Makefile
	${CC} -c ${CFLAGS} ${PARAM} param.c

init_sysent.o: $S/kern/init_sysent.c
	${NORMAL_C}

kern_acct.o: $S/kern/kern_acct.c
	${NORMAL_C}

kern_descrip.o: $S/kern/kern_descrip.c
	${NORMAL_C}

kern_exec.o: $S/kern/kern_exec.c
	${NORMAL_C}

kern_fork.o: $S/kern/kern_fork.c
	${NORMAL_C}

kern_kinfo.o: $S/kern/kern_kinfo.c
	${NORMAL_C}

kern_ktrace.o: $S/kern/kern_ktrace.c
	${NORMAL_C}

kern_malloc.o: $S/kern/kern_malloc.c
	${NORMAL_C}

kern_proc.o: $S/kern/kern_proc.c
	${NORMAL_C}

kern_prot.o: $S/kern/kern_prot.c
	${NORMAL_C}

kern_resource.o: $S/kern/kern_resource.c
	${NORMAL_C}

kern_subr.o: $S/kern/kern_subr.c
	${NORMAL_C}

kern_time.o: $S/kern/kern_time.c
	${NORMAL_C}

kern_xxx.o: $S/kern/kern_xxx.c
	${NORMAL_C}

dead_vnops.o: $S/kern/dead_vnops.c
	${NORMAL_C}

spec_vnops.o: $S/kern/spec_vnops.c
	${NORMAL_C}

fifo_vnops.o: $S/kern/fifo_vnops.c
	${NORMAL_C}

subr_autoconf.o: $S/kern/subr_autoconf.c
	${NORMAL_C}

subr_pdev.o: $S/kern/subr_pdev.c
	${NORMAL_C}

subr_log.o: $S/kern/subr_log.c
	${NORMAL_C}

subr_prf.o: $S/kern/subr_prf.c
	${NORMAL_C}

subr_xxx.o: $S/kern/subr_xxx.c
	${NORMAL_C}

sys_socket.o: $S/kern/sys_socket.c
	${NORMAL_C}

tty_compat.o: $S/kern/tty_compat.c
	${NORMAL_C}

tty_conf.o: $S/kern/tty_conf.c
	${NORMAL_C}

tty_pty.o: $S/kern/tty_pty.c
	${NORMAL_C}

tty_tty.o: $S/kern/tty_tty.c
	${NORMAL_C}

vfs_bio.o: $S/kern/vfs_bio.c
	${NORMAL_C}

vfs_cache.o: $S/kern/vfs_cache.c
	${NORMAL_C}

vfs_conf.o: $S/kern/vfs_conf.c
	${NORMAL_C}

vfs_lookup.o: $S/kern/vfs_lookup.c
	${NORMAL_C}

vfs_subr.o: $S/kern/vfs_subr.c
	${NORMAL_C}

vfs_vnops.o: $S/kern/vfs_vnops.c
	${NORMAL_C}

uipc_domain.o: $S/kern/uipc_domain.c
	${NORMAL_C}

uipc_mbuf.o: $S/kern/uipc_mbuf.c
	${NORMAL_C}

uipc_proto.o: $S/kern/uipc_proto.c
	${NORMAL_C}

uipc_socket.o: $S/kern/uipc_socket.c
	${NORMAL_C}

uipc_socket2.o: $S/kern/uipc_socket2.c
	${NORMAL_C}

uipc_syscalls.o: $S/kern/uipc_syscalls.c
	${NORMAL_C}

uipc_usrreq.o: $S/kern/uipc_usrreq.c
	${NORMAL_C}

kern_lock.o: $S/vm/kern_lock.c
	${NORMAL_C}

vm_fault.o: $S/vm/vm_fault.c
	${NORMAL_C}

vm_glue.o: $S/vm/vm_glue.c
	${NORMAL_C}

vm_init.o: $S/vm/vm_init.c
	${NORMAL_C}

vm_kern.o: $S/vm/vm_kern.c
	${NORMAL_C}

vm_map.o: $S/vm/vm_map.c
	${NORMAL_C}

vm_meter.o: $S/vm/vm_meter.c
	${NORMAL_C}

vm_mmap.o: $S/vm/vm_mmap.c
	${NORMAL_C}

vm_object.o: $S/vm/vm_object.c
	${NORMAL_C}

vm_page.o: $S/vm/vm_page.c
	${NORMAL_C}

vm_pageout.o: $S/vm/vm_pageout.c
	${NORMAL_C}

vm_pager.o: $S/vm/vm_pager.c
	${NORMAL_C}

vm_swap.o: $S/vm/vm_swap.c
	${NORMAL_C}

vm_unix.o: $S/vm/vm_unix.c
	${NORMAL_C}

vm_user.o: $S/vm/vm_user.c
	${NORMAL_C}

device_pager.o: $S/vm/device_pager.c
	${NORMAL_C}

swap_pager.o: $S/vm/swap_pager.c
	${NORMAL_C}

vnode_pager.o: $S/vm/vnode_pager.c
	${NORMAL_C}

ufs_alloc.o: $S/ufs/ufs_alloc.c
	${NORMAL_C}

ufs_lockf.o: $S/ufs/ufs_lockf.c
	${NORMAL_C}

ufs_lookup.o: $S/ufs/ufs_lookup.c
	${NORMAL_C}

ufs_subr.o: $S/ufs/ufs_subr.c
	${NORMAL_C}

ufs_tables.o: $S/ufs/ufs_tables.c
	${NORMAL_C}

ufs_vfsops.o: $S/ufs/ufs_vfsops.c
	${NORMAL_C}

ufs_quota.o: $S/ufs/ufs_quota.c
	${NORMAL_C}

mfs_vnops.o: $S/ufs/mfs_vnops.c
	${NORMAL_C}

mfs_vfsops.o: $S/ufs/mfs_vfsops.c
	${NORMAL_C}

iso9660_lookup.o: $S/iso9660/iso9660_lookup.c
	${NORMAL_C}

iso9660_node.o: $S/iso9660/iso9660_node.c
	${NORMAL_C}

iso9660_util.o: $S/iso9660/iso9660_util.c
	${NORMAL_C}

iso9660_vfsops.o: $S/iso9660/iso9660_vfsops.c
	${NORMAL_C}

iso9660_vnops.o: $S/iso9660/iso9660_vnops.c
	${NORMAL_C}

msdosfs_conv.o: $S/msdosfs/msdosfs_conv.c
	${NORMAL_C}

msdosfs_denode.o: $S/msdosfs/msdosfs_denode.c
	${NORMAL_C}

msdosfs_fat.o: $S/msdosfs/msdosfs_fat.c
	${NORMAL_C}

msdosfs_lookup.o: $S/msdosfs/msdosfs_lookup.c
	${NORMAL_C}

msdosfs_vfsops.o: $S/msdosfs/msdosfs_vfsops.c
	${NORMAL_C}

msdosfs_vnops.o: $S/msdosfs/msdosfs_vnops.c
	${NORMAL_C}

nfs_bio.o: $S/nfs/nfs_bio.c
	${NORMAL_C}

nfs_node.o: $S/nfs/nfs_node.c
	${NORMAL_C}

nfs_serv.o: $S/nfs/nfs_serv.c
	${NORMAL_C}

nfs_socket.o: $S/nfs/nfs_socket.c
	${NORMAL_C}

nfs_srvcache.o: $S/nfs/nfs_srvcache.c
	${NORMAL_C}

nfs_subs.o: $S/nfs/nfs_subs.c
	${NORMAL_C}

nfs_syscalls.o: $S/nfs/nfs_syscalls.c
	${NORMAL_C}

nfs_vfsops.o: $S/nfs/nfs_vfsops.c
	${NORMAL_C}

nfs_vnops.o: $S/nfs/nfs_vnops.c
	${NORMAL_C}

af.o: $S/net/af.c
	${NORMAL_C}

bpf.o: $S/net/bpf.c
	${NORMAL_C}

bpf_filter.o: $S/net/bpf_filter.c
	${NORMAL_C}

if.o: $S/net/if.c
	${NORMAL_C}

if_ethersubr.o: $S/net/if_ethersubr.c
	${NORMAL_C}

if_loop.o: $S/net/if_loop.c
	${NORMAL_C}

if_sl.o: $S/net/if_sl.c
	${NORMAL_C}

raw_cb.o: $S/net/raw_cb.c
	${NORMAL_C}

raw_usrreq.o: $S/net/raw_usrreq.c
	${NORMAL_C}

radix.o: $S/net/radix.c
	${NORMAL_C}

route.o: $S/net/route.c
	${NORMAL_C}

rtsock.o: $S/net/rtsock.c
	${NORMAL_C}

slcompress.o: $S/net/slcompress.c
	${NORMAL_C}

if_ether.o: $S/netinet/if_ether.c
	${NORMAL_C}

igmp.o: $S/netinet/igmp.c
	${NORMAL_C}

in.o: $S/netinet/in.c
	${NORMAL_C}

in_pcb.o: $S/netinet/in_pcb.c
	${NORMAL_C}

in_proto.o: $S/netinet/in_proto.c
	${NORMAL_C}

ip_icmp.o: $S/netinet/ip_icmp.c
	${NORMAL_C}

ip_input.o: $S/netinet/ip_input.c
	${NORMAL_C}

ip_output.o: $S/netinet/ip_output.c
	${NORMAL_C}

raw_ip.o: $S/netinet/raw_ip.c
	${NORMAL_C}

tcp_debug.o: $S/netinet/tcp_debug.c
	${NORMAL_C}

tcp_input.o: $S/netinet/tcp_input.c
	${NORMAL_C}

tcp_output.o: $S/netinet/tcp_output.c
	${NORMAL_C}

tcp_subr.o: $S/netinet/tcp_subr.c
	${NORMAL_C}

tcp_timer.o: $S/netinet/tcp_timer.c
	${NORMAL_C}

tcp_usrreq.o: $S/netinet/tcp_usrreq.c
	${NORMAL_C}

udp_usrreq.o: $S/netinet/udp_usrreq.c
	${NORMAL_C}

scsi_subr.o: $S/scsi/scsi_subr.c
	${NORMAL_C}

sd.o: $S/scsi/sd.c
	${DRIVER_C}

st.o: $S/scsi/st.c
	${DRIVER_C}

init_main.o: $S/i386/OBJ/init_main.o
	cp $S/i386/OBJ/init_main.o .

kern_clock.o: $S/i386/OBJ/kern_clock.o
	cp $S/i386/OBJ/kern_clock.o .

kern_exit.o: $S/i386/OBJ/kern_exit.o
	cp $S/i386/OBJ/kern_exit.o .

kern_physio.o: $S/i386/OBJ/kern_physio.o
	cp $S/i386/OBJ/kern_physio.o .

kern_sig.o: $S/i386/OBJ/kern_sig.o
	cp $S/i386/OBJ/kern_sig.o .

kern_synch.o: $S/i386/OBJ/kern_synch.o
	cp $S/i386/OBJ/kern_synch.o .

subr_rmap.o: $S/i386/OBJ/subr_rmap.o
	cp $S/i386/OBJ/subr_rmap.o .

sys_generic.o: $S/i386/OBJ/sys_generic.o
	cp $S/i386/OBJ/sys_generic.o .

sys_process.o: $S/i386/OBJ/sys_process.o
	cp $S/i386/OBJ/sys_process.o .

tty.o: $S/i386/OBJ/tty.o
	cp $S/i386/OBJ/tty.o .

tty_subr.o: $S/i386/OBJ/tty_subr.o
	cp $S/i386/OBJ/tty_subr.o .

vfs_syscalls.o: $S/i386/OBJ/vfs_syscalls.o
	cp $S/i386/OBJ/vfs_syscalls.o .

ufs_bmap.o: $S/i386/OBJ/ufs_bmap.o
	cp $S/i386/OBJ/ufs_bmap.o .

ufs_disksubr.o: $S/i386/OBJ/ufs_disksubr.o
	cp $S/i386/OBJ/ufs_disksubr.o .

ufs_inode.o: $S/i386/OBJ/ufs_inode.o
	cp $S/i386/OBJ/ufs_inode.o .

ufs_vnops.o: $S/i386/OBJ/ufs_vnops.o
	cp $S/i386/OBJ/ufs_vnops.o .

autoconf.o: $S/i386/i386/autoconf.c
	${DRIVER_C}

conf.o: $S/i386/i386/conf.c
	${NORMAL_C}

cons.o: $S/i386/i386/cons.c
	${NORMAL_C}

copy.o: $S/i386/i386/copy.s
	${NORMAL_S}

dos_disksubr.o: $S/i386/i386/dos_disksubr.c
	${NORMAL_C}

exec_machdep.o: $S/i386/i386/exec_machdep.c
	${NORMAL_C}

fpe_cmd.o: $S/i386/i386/fpe_cmd.c
	${NORMAL_C}

fpe_fmt.o: $S/i386/i386/fpe_fmt.c
	${NORMAL_C}

fpe_main.o: $S/i386/i386/fpe_main.c
	${NORMAL_C}

fpe_s.o: $S/i386/i386/fpe_s.s
	${NORMAL_S}

i386_param.o: $S/i386/i386/i386_param.c
	${NORMAL_C}

in_cksum.o: $S/i386/i386/in_cksum.c
	${NORMAL_C}

machdep.o: $S/i386/i386/machdep.c
	${NORMAL_C_C}

mem.o: $S/i386/i386/mem.c
	${NORMAL_C}

pmap.o: $S/i386/i386/pmap.c
	${NORMAL_C}

process_machdep.o: $S/i386/i386/process_machdep.c
	${NORMAL_C}

random.o: $S/i386/i386/random.s
	${NORMAL_S}

segments.o: $S/i386/i386/segments.c
	${NORMAL_C}

sys_machdep.o: $S/i386/i386/sys_machdep.c
	${NORMAL_C}

trap.o: $S/i386/i386/trap.c
	${NORMAL_C}

vm_machdep.o: $S/i386/i386/vm_machdep.c
	${NORMAL_C}

aha.o: $S/i386/isa/aha.c
	${DRIVER_C}

clock.o: $S/i386/isa/clock.c
	${NORMAL_C}

com.o: $S/i386/isa/com.c
	${DRIVER_C}

fd.o: $S/i386/isa/fd.c
	${DRIVER_C}

if_ef.o: $S/i386/isa/if_ef.c
	${DRIVER_C}

if_el.o: $S/i386/isa/if_el.c
	${DRIVER_C}

if_eo.o: $S/i386/isa/if_eo.c
	${DRIVER_C}

if_ep.o: $S/i386/isa/if_ep.c
	${DRIVER_C}

if_ex.o: $S/i386/isa/if_ex.c
	${DRIVER_C}

if_ne.o: $S/i386/isa/if_ne.c
	${DRIVER_C}

if_pe.o: $S/i386/OBJ/if_pe.o
	cp $S/i386/OBJ/if_pe.o .

if_petbl.o: $S/i386/OBJ/if_petbl.o
	cp $S/i386/OBJ/if_petbl.o .

bms.o: $S/i386/isa/bms.c
	${DRIVER_C}

if_tn.o: $S/i386/isa/if_tn.c
	${DRIVER_C}

if_we.o: $S/i386/isa/if_we.c
	${DRIVER_C}

isa.o: $S/i386/isa/isa.c
	${DRIVER_C}

lms.o: $S/i386/isa/lms.c
	${DRIVER_C}

lp.o: $S/i386/isa/lp.c
	${DRIVER_C}

mcd.o: $S/i386/isa/mcd.c
	${DRIVER_C}

npx.o: $S/i386/isa/npx.c
	${DRIVER_C}

pccons.o: $S/i386/isa/pccons.c
	${DRIVER_C}

vga.o: $S/i386/isa/vga.c
	${DRIVER_C}

wd.o: $S/i386/isa/wd.c
	${DRIVER_C}

wt.o: $S/i386/isa/wt.c
	${DRIVER_C}

xir.o: $S/i386/OBJ/xir.o
	cp $S/i386/OBJ/xir.o .

eisa.o: $S/i386/eisa/eisa.c
	${NORMAL_C}

eaha.o: $S/i386/eisa/eaha.c
	${DRIVER_C}

