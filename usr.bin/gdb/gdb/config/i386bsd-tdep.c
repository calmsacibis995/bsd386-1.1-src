/*	BSDI $Id: i386bsd-tdep.c,v 1.3 1993/03/08 07:21:26 polk Exp $	*/

/* Intel 386 target-dependent stuff.
   Copyright (C) 1988, 1989, 1991 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * Machine-dependent kernel debugging support for BSD/386.
 * Mainly taken from sparcbsd-tdep.c from LBL.
 */

#ifdef KERNELDEBUG
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/vmparam.h>

#include <kvm.h>

extern int kernel_debugging;

/*
 * Read the "thing" at address 'addr' into the space pointed to by P.
 * The length of the "thing" is determined by the type of P.
 * Result is non-zero if transfer fails.
 */
#define READMEM(addr, p) \
    (target_read_memory((CORE_ADDR)(addr), (char *)(p), sizeof(*(p))))
#endif

#include "defs.h"
#include "frame.h"
#include "value.h"
#include "target.h"
#include "gdbcore.h"

/*
 * Pop a dummy frame.
 * The call_function_by_hand() code will restore most of our variables;
 * we just fix up the stack here.
 */
void
i386_pop_dummy_frame ()
{
  FRAME frame = get_current_frame ();
  CORE_ADDR fp;
  struct frame_info *fi;
  
  fi = get_frame_info (frame);
  fp = fi->frame;

  write_register (FP_REGNUM, read_memory_integer (fp, 4));
  write_register (PC_REGNUM, read_memory_integer (fp + 4, 4));
  write_register (SP_REGNUM, fp + 8);
  flush_cached_frames ();
  set_current_frame ( create_new_frame (read_register (FP_REGNUM),
					read_pc ()));
}

/*
 * Return the address of the saved pc in frame.
 */
CORE_ADDR
addr_of_pc(struct frame_info *frame)
{
#ifdef KERNELDEBUG
	static CORE_ADDR tstart, tend, istart, iend;
	CORE_ADDR pc;
	unsigned long addr;

	if (kernel_debugging && frame->next) {
		if (tstart == 0) {
			tstart = ksym_lookup("Xdiv");
			tend = ksym_lookup("Xsyscall");
			istart = ksym_lookup("Vclk");
			iend = ksym_lookup("doreti");
		}
		pc = FRAME_SAVED_PC(frame->next);
		if (tstart <= pc && pc < tend) {
			struct trapframe *tfr = (struct trapframe *)
				(frame->next->frame + 8);
			return ((CORE_ADDR)&tfr->tf_eip);
		}
		if (istart <= pc && pc < iend) {
			struct intrframe *ifr = (struct intrframe *)
				(frame->next->frame + 8);
			return ((CORE_ADDR)&ifr->if_eip);
		}
	}
#endif
	return ((CORE_ADDR)(frame->next->frame + 4));
}

#ifdef KERNELDEBUG
/*
 * The code below implements kernel debugging of crashdumps (or /dev/kmem)
 * or remote systems (via a serial link).  For remote kernels, the remote
 * context does most the work, so there is very little to do -- we just
 * manage the kernel stack boundaries so we know where to stop a backtrace.
 *
 * The crashdump/kmem (kvm) support is a bit more grungy, but thanks to
 * libkvm (see kcore.c) not too bad.  The main work is kvm_fetch_registers
 * which sucks the register state out of the current processes pcb.
 * There is a command that let's you set the current process -- hopefully,
 * to something that's blocked (in the live kernel case).
 */
 
static CORE_ADDR kernstack_top;
static CORE_ADDR kernstack_bottom;
static struct proc *masterprocp;		/* kernel virtual address */
static struct proc fakeproc;
void set_curproc();

/*
 * Return true if ADDR is a valid stack address according to the
 * current boundaries (which are determined by the currently running 
 * user process).
 */
int
inside_kernstack(CORE_ADDR addr)
{
	if (masterprocp == 0)
		set_curproc();

	return (addr > kernstack_bottom && addr < kernstack_top);
}

/*
 * (re-)set the variables that make inside_kernstack() work.
 */
static void
set_kernel_boundaries(struct proc *p)
{
#if 0	/* fix this when we no longer map PCBs to a fixed address */
	CORE_ADDR a = (CORE_ADDR)pcb;
#else
	CORE_ADDR a = (CORE_ADDR)VM_MAXUSER_ADDRESS;
#endif

	kernstack_bottom = a;
	kernstack_top = a + UPAGES * NBPG;
}

/*
 * Return the current proc.  masterprocp points to
 * current proc which points to current u area.
 */
static struct proc *
fetch_curproc()
{
	struct proc *p;
	static CORE_ADDR curproc_addr;

	if (!curproc_addr)
		curproc_addr = ksym_lookup("curproc");
	if (READMEM(curproc_addr, &p))
		error("cannot read curproc pointer at 0x%x\n", curproc_addr);
	return (p);
}

/*
 * All code below is exclusively for support of kernel core files.
 */

/*
 * Fetch registers from a crashdump or /dev/kmem.
 */
static void
kvm_fetch_registers(p)
	struct proc *p;
{
	struct pcb *ppcb, pcb;

	/* find the pcb for the current process */
	if (READMEM(&p->p_addr, &ppcb))
		error("cannot read pcb pointer at 0x%x", &p->p_addr);
	if (READMEM(ppcb, &pcb))
		error("cannot read pcb at 0x%x", ppcb);

	/*
	 * Invalidate all the registers then fill in the ones we know about.
	 */
	registers_changed();

	supply_register(PC_REGNUM, (char *)&pcb.pcb_pc);
	supply_register(FP_REGNUM, (char *)&pcb.pcb_fp);
	supply_register(SP_REGNUM, (char *)&pcb.pcb_ksp);
	supply_register(PS_REGNUM, (char *)&pcb.pcb_psl);

	supply_register(0, (char *)&pcb.pcb_tss.tss_eax);
	supply_register(1, (char *)&pcb.pcb_tss.tss_ecx);
	supply_register(2, (char *)&pcb.pcb_tss.tss_edx);
	supply_register(3, (char *)&pcb.pcb_tss.tss_ebx);
	supply_register(6, (char *)&pcb.pcb_tss.tss_esi);
	supply_register(7, (char *)&pcb.pcb_tss.tss_edi);
	supply_register(10, (char *)&pcb.pcb_tss.tss_cs);
	supply_register(11, (char *)&pcb.pcb_tss.tss_ss);
	supply_register(12, (char *)&pcb.pcb_tss.tss_ds);
	supply_register(13, (char *)&pcb.pcb_tss.tss_es);
	supply_register(14, (char *)&pcb.pcb_tss.tss_fs);
	supply_register(15, (char *)&pcb.pcb_tss.tss_gs);
}

/*
 * Called from remote_wait, after the remote kernel has stopped.
 * Look up the current proc, and set up boundaries.
 * This is for active kernels only.
 */
void
set_curproc()
{
	masterprocp = fetch_curproc();
	set_kernel_boundaries(masterprocp);
}

/*
 * Set the process context to that of the proc structure at
 * system address paddr.  Read in the register state.
 */
int
set_procaddr(CORE_ADDR paddr)
{
	struct pcb *ppcb;

	if (paddr == 0)
		masterprocp = fetch_curproc();
	else if (paddr != (CORE_ADDR)masterprocp) {
		if ((unsigned)paddr < KERNBASE)
			return (1);
		masterprocp = (struct proc *)paddr;
	}
	if (READMEM(&masterprocp->p_vmspace, &fakeproc.p_vmspace))
		error("cannot read vmspace pointer at 0x%x",
		    &masterprocp->p_vmspace);
	set_kernel_boundaries(masterprocp);
	kvm_fetch_registers(masterprocp);
	return (0);
}

/*
 * Get the registers out of a crashdump or /dev/kmem.
 * XXX This somehow belongs in kcore.c.
 *
 * We just get all the registers, so we don't use regno.
 */
void
kernel_core_registers(int regno)
{
	/*
	 * Need to find current u area to get kernel stack and pcb
	 * where "panic" saved registers.
	 * (libkvm also needs to know current u area to get user
	 * address space mapping).
	 */
	(void)set_procaddr((CORE_ADDR)masterprocp);
}

extern kvm_t *kd;

/*
 * We have to handle kernel stack references specially since they
 * fall in the user part of the page table.
 */
int
kernel_xfer_memory(addr, cp, len, write, target)
     CORE_ADDR addr;
     char *cp;
     int len;
     int write;
     struct target_ops *target;
{
	if (write)
		return kvm_write(kd, addr, cp, len);
	if (addr < (CORE_ADDR)KERNBASE) {
		if (masterprocp == 0 && set_procaddr(0))
			return (-1);
		return kvm_uread(kd, &fakeproc, addr, cp, len);
	}
	return kvm_read(kd, addr, cp, len);
}
#endif
