/*	BSDI $Id: kcore.c,v 1.2 1992/11/05 22:29:06 trent Exp $	*/

/* Work with kernel crash dumps and live systems through kvm.  This
   module was developed at Lawrence Berkeley Laboratory by Steven
   McCanne (mccanne@ee.lbl.gov).  It is derived from the gdb module core.c.
   The GNU copyright is therefore maintained.

   Copyright 1986, 1987, 1989, 1991 Free Software Foundation, Inc.
   
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

#ifdef KERNELDEBUG

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "defs.h"
#include "frame.h"  /* required by inferior.h */
#include "inferior.h"
#include "symtab.h"
#include "command.h"
#include "bfd.h"
#include "target.h"
#include "gdbcore.h"

#include <kvm.h>
#include <sys/stat.h>

extern int xfer_memory ();
extern void child_attach (), child_create_inferior ();

extern int sys_nerr;
extern char *sys_errlist[];
extern char *sys_siglist[];

extern char registers[];

/* KVM library handle */

kvm_t *kd;

/* Forward decl */
extern struct target_ops kernel_core_ops;


/* Discard all vestiges of any previous core file
   and mark data and stack spaces as empty.  */

/* ARGSUSED */
void
kernel_core_close (quitting)
	int quitting;
{
	if (kd != 0) {
		kvm_close(kd);
		kd = 0;
	}
	kernel_debugging = 0;
}

/* This routine opens and sets up the core file bfd */

void
kernel_core_open(filename, from_tty)
	char *filename;
	int from_tty;
{
	char *cp;
	struct cleanup *old_chain;
	int ontop;
	char *execfile;
	struct stat stb;
	char *get_exec_file_name();
	
	if (kd != 0) 
		error("kvm already target -- must detach first");

	target_preopen(from_tty);
	if (filename == 0)
		error ("No core file specified.");
	filename = tilde_expand (filename);
	if (filename[0] != '/') {
		cp = concat (current_directory, "/", filename, NULL);
		free(filename);
		filename = cp;
	}
	old_chain = make_cleanup (free, filename);
	execfile = get_exec_file_name();
	if (execfile == 0) 
		error("No executable image specified");

	/*
	 * If we want the active memory file, just use a null arg for kvm.
	 * The SunOS kvm can't read from the default swap device, unless
	 * /dev/mem is indicated with a null pointer.  This has got to be 
	 * a bug.
	 */
	if (stat(filename, &stb) == 0 && S_ISCHR(stb.st_mode))
		filename = 0;

	kd = kvm_open(execfile, filename, (char *)0,
		      write_files ? O_RDWR : O_RDONLY, "gdb");
	if (kd == 0)
		error("Cannot open kernel core image");

	kernel_debugging = 1;
	unpush_target (&kernel_core_ops);
	old_chain = make_cleanup(kernel_core_close, (int)kd);
#ifdef notdef
	validate_files ();
#endif
	ontop = !push_target (&kernel_core_ops);
	discard_cleanups (old_chain);
	
	if (ontop) {
		/* Fetch all registers from core file */
		target_fetch_registers (-1);
		
		/* Now, set up the frame cache, and print the top of stack */
		set_current_frame (create_new_frame (read_register (FP_REGNUM),
						     read_pc ()));
		select_frame (get_current_frame (), 0);
		print_stack_frame (selected_frame, selected_frame_level, 1);
	} else {
		printf (
			"Warning: you won't be able to access this core file until you terminate\n\
your %s; do ``info files''\n", current_target->to_longname);
	}
	/* Machine dependent call to print out panic string etc. */
	kerninfo();
}

void
kernel_core_detach (args, from_tty)
	char *args;
	int from_tty;
{
	if (args)
		error ("Too many arguments");
	unpush_target (&kernel_core_ops);
	if (from_tty)
		printf ("No kernel core file now.\n");
}

static void
kernel_core_files_info (t)
	struct target_ops *t;
{
#ifdef notdef
	struct section_table *p;
	
	printf_filtered ("\t`%s', ", bfd_get_filename(core_bfd));
	wrap_here ("        ");
	printf_filtered ("file type %s.\n", bfd_get_target(core_bfd));
	
	for (p = t->sections; p < t->sections_end; p++) {
		printf_filtered ("\t%s", local_hex_string_custom (p->addr, "08"));
		printf_filtered (" - %s is %s",
				 local_hex_string_custom (p->endaddr, "08"),
				 bfd_section_name (p->bfd, p->sec_ptr));
		if (p->bfd != core_bfd) {
			printf_filtered (" in %s", bfd_get_filename (p->bfd));
		}
		printf_filtered ("\n");
	}
#endif
}

/*
 * Called by the machine dependent module to set a user context.
 * We call kvm_getu() for this desired side effect.
 * BSD kvm doesn't need to do this.
 */
kernel_getu(p)
	u_long *p;
{
#if BSD < 199103 && __bsdi__ == 0
        if (kd != 0)
		return (kvm_getu(kd, p) != 0);
#endif
	return (0);
}

#ifndef KERNEL_XFER_MEMORY
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
	else
		return kvm_read(kd, addr, cp, len);
}
#endif


/* 
 * In target dependent module.
 */
extern void kernel_core_registers();

struct target_ops kernel_core_ops = {
	"kvm", "Kernel core file",
	"Use a kernel core file as a target.  Specify the filename of the core file.",
	kernel_core_open, kernel_core_close,
	0, kernel_core_detach, 0, 0, /* resume, wait */
	kernel_core_registers, 
	0, 0, 0, 0, /* store_regs, prepare_to_store, conv_to, conv_from */
	kernel_xfer_memory, kernel_core_files_info,
	0, 0, /* core_insert_breakpoint, core_remove_breakpoint, */
	0, 0, 0, 0, 0, /* terminal stuff */
	0, 0, 0, /* kill, load, lookup sym */
	child_create_inferior, 0, /* mourn_inferior */
	core_stratum, 0, /* next */
	0, 1, 1, 1, 0,	/* all mem, mem, stack, regs, exec */
	0, 0,			/* section pointers */
	OPS_MAGIC,		/* Always the last thing */
};
#endif

void
_initialize_kernel_core()
{
#ifdef KERNELDEBUG
#ifdef notdef
	
	add_com ("kcore", class_files, core_file_command,
		 "Use FILE as core dump for examining memory and registers.\n\
No arg means have no core file.  This command has been superseded by the\n\
`target core' and `detach' commands.");
#endif
	add_target (&kernel_core_ops);
#endif
}
