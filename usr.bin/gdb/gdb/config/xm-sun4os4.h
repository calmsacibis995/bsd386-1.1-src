/* Macro definitions for running gdb on a Sun 4 running sunos 4.
   Copyright (C) 1989, Free Software Foundation, Inc.

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

#include "xm-sparc.h"

/* Get rid of any system-imposed stack limit if possible.  */

#define SET_STACK_LIMIT_HUGE

/* Enable use of alternate code for Sun's format of core dump file.  */

#define NEW_SUN_CORE

/* Do implement the attach and detach commands.  */

#define ATTACH_DETACH

/* Override copies of {fetch,store}_inferior_registers in infptrace.c.  */

#define FETCH_INFERIOR_REGISTERS

/* Before storing, we need to read all the registers.  */

#define CHILD_PREPARE_TO_STORE() read_register_bytes (0, NULL, REGISTER_BYTES)

/* It does have a wait structure, and it might help things out . . . */

#define HAVE_WAIT_STRUCT

/* Optimization for storing registers to the inferior.  The hook
   DO_DEFERRED_STORES
   actually executes any deferred stores.  It is called any time
   we are going to proceed the child, or read its registers.
   The hook CLEAR_DEFERRED_STORES is called when we want to throw
   away the inferior process, e.g. when it dies or we kill it.
   FIXME, this does not handle remote debugging cleanly.  */

extern int deferred_stores;
#define	DO_DEFERRED_STORES	\
  if (deferred_stores)		\
    store_inferior_registers (-2);
#define	CLEAR_DEFERRED_STORES	\
  deferred_stores = 0;
#define FPU

/* Large alloca's fail because the attempt to increase the stack limit in
   main() fails because shared libraries are allocated just below the initial
   stack limit.  The SunOS kernel will not allow the stack to grow into
   the area occupied by the shared libraries.  Sun knows about this bug
   but has no obvious fix for it.  */
/* XXX is this true for BSD */
#define BROKEN_LARGE_ALLOCA

/* SunOS 4.x has memory mapped files.  */

#define HAVE_MMAP

/* If you expect to use the mmalloc package to obtain mapped symbol files,
   for now you have to specify some parameters that determine how gdb places
   the mappings in it's address space.  See the comments in map_to_address()
   for details.  This is expected to only be a short term solution.  Yes it
   is a kludge.
   FIXME:  Make this more automatic. */

#define MMAP_BASE_ADDRESS	0xE0000000	/* First mapping here */
#define MMAP_INCREMENT		0x01000000	/* Increment to next mapping */
