/* Macro definitions for GDB for a Sun 4 running sunos 4.
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

#include <sys/param.h>
#define KERNEL			/* need trapframe */
#include <machine/reg.h>
#undef KERNEL

#include "tm-sparc.h"

#undef STACK_END_ADDRESS
#define STACK_END_ADDRESS 0xf8000000
