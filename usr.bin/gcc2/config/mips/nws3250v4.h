/* Definitions of target machine for GNU compiler.  Sony RISC NEWS (mips)
   Copyright (C) 1991 Free Software Foundation, Inc.

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

#define MIPS_NEWS

#define CPP_PREDEFINES "\
-Dmips -Dhost_mips -Dsony -Dsonyrisc -Dunix \
-DLANGUAGE_C -DMIPSEB -DSYSTYPE_SYSV \
-Asystem(unix) -Asystem(svr3) -Acpu(mips) -Amachine(mips)"

#define MD_STARTFILE_PREFIX "/usr/ccs/lib/"

#define ASM_SPEC "\
%{!mgas: \
	%{!mrnames: -nocpp} \
	%{pipe:%e:-pipe not supported} \
	%{EB} %{!EB:-EB} \
	%{EL: %e-EL not supported} \
	%{O:-O2} %{O1:-O2} %{O2:-O2} %{O3:-O3} \
	%{g} %{g1} %{g2} %{g3} %{g0} %{v} %{K} \
	%{ggdb:-g} %{ggdb0:-g0} %{ggdb1:-g1} %{ggdb2:-g2} %{ggdb3:-g3} \
	%{gstabs:-g} %{gstabs0:-g0} %{gstabs1:-g1} %{gstabs2:-g2} %{gstabs3:-g3} \
	%{gstabs+:-g} %{gstabs+0:-g0} %{gstabs+1:-g1} %{gstabs+2:-g2} %{gstabs+3:-g3} \
	%{gcoff:-g} %{gstabs0:-g0} %{gcoff1:-g1} %{gcoff2:-g2} %{gcoff3:-g3}} \
%{G*}"

#define CPP_SPEC "\
%{.cc:	-D__LANGUAGE_C_PLUS_PLUS -D_LANGUAGE_C_PLUS_PLUS} \
%{.cxx:	-D__LANGUAGE_C_PLUS_PLUS -D_LANGUAGE_C_PLUS_PLUS} \
%{.C:	-D__LANGUAGE_C_PLUS_PLUS -D_LANGUAGE_C_PLUS_PLUS} \
%{.m:	-D__LANGUAGE_OBJECTIVE_C -D_LANGUAGE_OBJECTIVE_C} \
%{.S:	-D__LANGUAGE_ASSEMBLY -D_LANGUAGE_ASSEMBLY %{!ansi:-DLANGUAGE_ASSEMBLY}} \
%{.s:	-D__LANGUAGE_ASSEMBLY -D_LANGUAGE_ASSEMBLY %{!ansi:-DLANGUAGE_ASSEMBLY}} \
%{!.S:%{!.s:	-D__LANGUAGE_C -D_LANGUAGE_C %{!ansi:-DLANGUAGE_C}}}"

#define LINK_SPEC "\
%{G*} \
%{!mgas: \
	%{EB} %{!EB:-EB} \
	%{EL: %e-EL not supported} \
	%{bestGnum}}"

#define LIB_SPEC "%{p:-lprof1} %{pg:-lprof1} -lc crtn.o%s values-Xt.o%s"

#define STARTFILE_SPEC "%{pg:gcrt0.o%s}%{!pg:%{p:mcrt0.o%s}%{!p:crt1.o%s}}"

#define MACHINE_TYPE "RISC NEWS-OS SVr4"

#include "mips/mips.h"
