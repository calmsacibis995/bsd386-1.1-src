/* Definitions of target machine for GNU compiler.  ARM RISCiX(stabs) version.
   Copyright (C) 1993 Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe11@cl.cam.ac.uk), based on original
	      work by Pieter `Tiggr' Schoenmakers (rcpieter@win.tue.nl)
   	      and Martin Simmons (@harleqn.co.uk).

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "arm/riscix.h"

/* The native RISCiX assembler does not support stabs of any kind; because
   the native assembler is not used by the compiler, Acorn didn't feel it was
   necessary to put them in!  
   However, this file assumes that we have an assembler that does have stabs,
   so we put them back in.  */

#define DBX_DEBUGGING_INFO

/* Unfortunately dbx doesn't understand these */
#define DEFAULT_GDB_EXTENSIONS 0
/* RISCiX dbx doesn't accept xrefs */
#define DBX_NO_XREFS 1
/* dbx wants lbracs before variables */
/* #define DBX_LBRAC_FIRST 1*/
