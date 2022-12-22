/* BFD backend for SunOS binaries.
   Copyright (C) 1990-1991 Free Software Foundation, Inc.
   Written by Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

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

/* $Id: sparcbsd.c,v 1.1.1.1 1992/08/27 17:04:49 trent Exp $ */

#define ARCH 32
#define TARGETNAME "a.out-sparcbsd-big"
#define MY(OP) CAT(sparcbsd_big_,OP)

#include <stdio.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/kinfo_proc.h>

/*
 * Code below originally extracted from aoutf1.h
 */
#include "bfd.h"
#ifdef notdef
#include "sysdep.h"
#endif
#include "libbfd.h"

#include "aout/sparcbsd.h"
#include "libaout.h"           /* BFD a.out internal data structures */

#include "aout/aout64.h"
#include "aout/stab_gnu.h"
#include "aout/ar.h"

void (*bfd_error_trap)();

static bfd_target *sparcbsd_callback ();

/*SUPPRESS558*/
/*SUPPRESS529*/

#ifdef notdef
bfd_target *
DEFUN(NAME(sparcbsd,object_p), (abfd),
     bfd *abfd)
{
  enum bfd_architecture arch;
  long machine;

  struct external_exec exec_bytes;	/* Raw exec header from file */
  struct internal_exec exec;		/* Cleaned-up exec header */

  if (bfd_read ((PTR) &exec_bytes, 1, EXEC_BYTES_SIZE, abfd)
      != EXEC_BYTES_SIZE) {
    bfd_error = wrong_format;
    return 0;
  }

  exec.a_info = bfd_h_get_32 (abfd, exec_bytes.e_info);


  if (N_BADMAG (exec)) return 0;


  NAME(aout,swap_exec_header_in)(abfd, &exec_bytes, &exec);

  /* Determine the architecture and machine type of the object file.  */
  bfd_set_arch_mach(abfd, bfd_arch_sparc, 0);

  return NAME(aout,some_aout_object_p) (abfd, &exec, sparcbsd_callback);
}
#endif

  /* Determine the size of a relocation entry, based on the architecture */
static void
DEFUN(choose_reloc_size,(abfd),
bfd *abfd)
{
  switch (bfd_get_arch(abfd)) {
  case bfd_arch_sparc:
  case bfd_arch_a29k:
    obj_reloc_entry_size (abfd) = RELOC_EXT_SIZE;
    break;
  default:
    obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;
    break;
  }
}

#ifdef notdef
/* Set parameters about this a.out file that are machine-dependent.
   This routine is called from some_aout_object_p just before it returns.  */

static bfd_target *
sparcbsd_callback (abfd)
     bfd *abfd;
{
  struct internal_exec *execp = exec_hdr (abfd);

  WORK_OUT_FILE_POSITIONS(abfd, execp);  

  choose_reloc_size(abfd);
  adata(abfd).page_size = PAGE_SIZE;
#ifdef SEGMENT_SIZE
  adata(abfd).segment_size = SEGMENT_SIZE;
#else
  adata(abfd).segment_size = PAGE_SIZE;
#endif
  adata(abfd).exec_bytes_size = EXEC_BYTES_SIZE;

  return abfd->xvec;
}
#endif


static boolean
DEFUN(NAME(aout,sparcbsd_write_object_contents),
      (abfd),
      bfd *abfd)
{
	abort();
}

/* core files */

#define CORE_MAGIC 0x080456
#define CORE_NAMELEN 16

struct internal_bsd_core {
#ifdef notdef
	int c_magic;		/* Corefile magic number */
#endif
	int c_len;		/* size of header info */
	long c_regs_pos;	/* file offset of General purpose registers */
	int c_regs_size;
	long c_fregs_pos;	/* file offset of external FPU state (regs) */
	int c_fregs_size;	/* Size of it */
#ifdef notdef
	struct internal_exec c_aouthdr; /* A.out header */
#endif
	int c_signo;		/* Killing signal, if any */
	int c_ucode;		/* Exception no. from u_code */
	int c_tsize;		/* Text size (bytes) */
	int c_dsize;		/* Data size (bytes) */
	int c_ssize;		/* Stack size (bytes) */
	long c_stacktop;	/* Stack top (address) */
	char c_cmdname[MAXCOMLEN+1]; /* Command name */
};

/* byte-swap in the Sparc core structure */
static void
swapcore_sparc(abfd, u, ic)
	bfd *abfd;
	struct user *u;
	struct internal_bsd_core *ic;
{
	struct eproc *ep;
	struct proc *p;
#ifdef notdef
	ic->c_magic = bfd_h_get_32 (abfd, (u_char *)&extcore->c_magic);
	ic->c_len   = bfd_h_get_32 (abfd, (u_char *)&extcore->c_len);
#endif
	ic->c_regs_pos = (long)&((struct user *)0)->u_md.md_tf;
	ic->c_regs_size = sizeof(u->u_md.md_tf);
#ifdef notdef
	NAME(aout,swap_exec_header_in)(abfd, &extcore->c_aouthdr,&ic->c_aouthdr);
#endif
	ic->c_signo = bfd_h_get_32(abfd, (u_char *)&u->u_sigacts.ps_sig);
	ic->c_ucode = bfd_h_get_32(abfd, (u_char *)&u->u_sigacts.ps_code);
	ep = &u->u_kproc.kp_eproc;
	ic->c_tsize = ctob(ep->e_vm.vm_tsize);
	ic->c_dsize = ctob(ep->e_vm.vm_dsize);
	ic->c_ssize = ctob(ep->e_vm.vm_ssize);

	p = &u->u_kproc.kp_proc;
	bcopy(p->p_comm, ic->c_cmdname, sizeof(ic->c_cmdname));

	ic->c_fregs_pos = (long)(long)&((struct user *)0)->u_md.md_fpstate;
	ic->c_fregs_size = sizeof(u->u_md.md_fpstate);

	/* XXX */
	ic->c_stacktop = 0xf8000000;
	ic->c_len = ctob(UPAGES);
}

/* These are stored in the bfd's tdata */
struct bsd_core_struct {
	asection *data_section;
	asection *stack_section;
	asection *reg_section;
	asection *reg2_section;
	struct internal_bsd_core internal;
};

static bfd_target *
sparcbsd_core_file_p(abfd)
	bfd *abfd;
{
	int core_size;
	int core_mag;
	struct bsd_core_struct *p;
	struct internal_bsd_core *core;
	struct user user;
	
	bfd_error = system_call_error;
	if (bfd_seek (abfd, 0L, false) < 0) 
		return 0;
	
	p = (struct bsd_core_struct *)bfd_zalloc(abfd, sizeof(*p));
	if (p == NULL) {
		bfd_error = no_memory;
		return 0;
	}
	if ((bfd_read ((PTR)&user, 1, sizeof(user), abfd)) != sizeof(user)) {
		bfd_error = system_call_error;
		bfd_release (abfd, (char *)p);
		return 0;
	}
	core = &p->internal;
	swapcore_sparc(abfd, &user, core);
	abfd->tdata.bsd_core_data = p;
	/*
	 * create the sections.  This is raunchy, but bfd_close wants to 
	 * reclaim them
	 */
	p->stack_section = (asection *) bfd_zalloc (abfd, sizeof (asection));
	if (p->stack_section == NULL)
		goto bail1;
	p->data_section = (asection *) bfd_zalloc (abfd, sizeof (asection));
	if (p->data_section == NULL)
		goto bail2;
	p->reg_section = (asection *) bfd_zalloc (abfd, sizeof (asection));
	if (p->reg_section == NULL)
		goto bail3;
	p->reg2_section = (asection *) bfd_zalloc (abfd, sizeof (asection));
	if (p->reg2_section == NULL)
		goto bail4;
	
	p->stack_section->name = ".stack";
	p->data_section->name = ".data";
	p->reg_section->name = ".reg";
	p->reg2_section->name = ".reg2";
	
	p->stack_section->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
	p->data_section->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
	p->reg_section->flags = SEC_ALLOC + SEC_HAS_CONTENTS;
	p->reg2_section->flags = SEC_ALLOC + SEC_HAS_CONTENTS;
	
	p->stack_section->_raw_size = core->c_ssize;
	p->data_section->_raw_size = core->c_dsize;
	p->reg_section->_raw_size = core->c_regs_size;
	p->reg2_section->_raw_size = core->c_fregs_size;
	
	p->stack_section->vma = (core->c_stacktop - core->c_ssize);
	/* XXX */
	p->data_section->vma = core->c_tsize;
	p->reg_section->vma = 0;
	p->reg2_section->vma = 0;
	
	p->stack_section->filepos = core->c_len + core->c_dsize;
	p->data_section->filepos = core->c_len;
	/* We'll access the regs afresh in the core file, like any section: */
	p->reg_section->filepos = (file_ptr)core->c_regs_pos;
	p->reg2_section->filepos = (file_ptr)core->c_fregs_pos;
	
	/* Align to word at least */
	p->stack_section->alignment_power = 2;
	p->data_section->alignment_power = 2;
	p->reg_section->alignment_power = 2;
	p->reg2_section->alignment_power = 2;
	
	abfd->sections = p->stack_section;
	p->stack_section->next = p->data_section;
	p->data_section->next = p->reg_section;
	p->reg_section->next = p->reg2_section;
	
	abfd->section_count = 4;
	
	return (abfd->xvec);

bail4:	bfd_release(abfd, p->reg_section);
bail3:	bfd_release(abfd, p->data_section);
bail2:	bfd_release(abfd, p->stack_section);
bail1:	bfd_error = no_memory;
	bfd_release(abfd, (char *)p);

	return (0);
}

#define coreinfo(p) (&(p)->tdata.bsd_core_data->internal)

static char *
sparcbsd_core_file_failing_command (abfd)
	bfd *abfd;
{
	return coreinfo(abfd)->c_cmdname;
}

static int
sparcbsd_core_file_failing_signal(abfd)
	bfd *abfd;
{
	return coreinfo(abfd)->c_signo;
}

static boolean
sparcbsd_core_file_matches_executable_p(core_bfd, exec_bfd)
      bfd *core_bfd;
      bfd *exec_bfd;
{
	/* XXX */
	return (1);
#ifdef notdef
	if (core_bfd->xvec != exec_bfd->xvec) {
		bfd_error = system_call_error;
		return false;
	}

	return (bcmp((char *)&coreinfo(core_bfd)->c_aouthdr, 
		     (char *)exec_hdr(exec_bfd),
		     sizeof (struct internal_exec)) == 0) ? true : false;
#endif
}


#define	MY_core_file_failing_command 	sparcbsd_core_file_failing_command
#define	MY_core_file_failing_signal	sparcbsd_core_file_failing_signal
#define	MY_core_file_matches_executable_p sparcbsd_core_file_matches_executable_p

#define MY_bfd_debug_info_start		bfd_void
#define MY_bfd_debug_info_end		bfd_void
#define MY_bfd_debug_info_accumulate	(PROTO(void,(*),(bfd*, struct sec *))) bfd_void
#ifdef notdef
#define MY_object_p NAME(sparcbsd,object_p)
#endif
#define MY_core_file_p sparcbsd_core_file_p
#define MY_write_object_contents NAME(aout,sparcbsd_write_object_contents)

#define TARGET_IS_BIG_ENDIAN_P

#define DEFAULT_ARCH bfd_arch_sparc

#include "aout-target.h"
