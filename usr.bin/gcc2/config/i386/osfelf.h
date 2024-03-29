/* Definitions of target machine for GNU compiler.
   Intel 386 (OSF/1 with ELF) version.
   Copyright (C) 1993 Free Software Foundation, Inc.

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

#include "config/i386/osfrose.h"

#undef	CPP_PREDEFINES
#define CPP_PREDEFINES "-DOSF -DOSF1 -Dunix -Di386 -Asystem(unix) -Asystem(xpg4) -Acpu(i386) -Amachine(i386)"

#undef  CPP_SPEC
#define CPP_SPEC "\
%{mrose: -D__ROSE__ %{!pic-none: -D__SHARED__}} \
%{!mrose: -D__ELF__ %{fpic: -D__SHARED__}} \
%{mno-underscores: -D__NO_UNDERSCORES__} \
%{!mrose: %{!munderscores: -D__NO_UNDERSCORES__}} \
%{.S:	%{!ansi:%{!traditional:%{!traditional-cpp:%{!ftraditional: -traditional}}}}} \
%{.S:	-D__LANGUAGE_ASSEMBLY %{!ansi:-DLANGUAGE_ASSEMBLY}} \
%{.cc:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.cxx:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.C:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.m:	-D__LANGUAGE_OBJECTIVE_C} \
%{!.S:	-D__LANGUAGE_C %{!ansi:-DLANGUAGE_C}}"

/* Turn on -mpic-extern by default (change to later use -fpic.  */
#undef  CC1_SPEC
#define CC1_SPEC "\
%{!melf: %{!mrose: -melf }} \
%{!mrose: %{!munderscores: %{!mno-underscores: -mno-underscores }}} \
%{gline:%{!g:%{!g0:%{!g1:%{!g2: -g1}}}}} \
%{mrose: %{pic-none: -mno-half-pic} \
	 %{pic-extern: } %{pic-lib: } %{pic-calls: } %{pic-names*: } \
	 %{!pic-none: -mhalf-pic }}"

#undef	ASM_SPEC
#define ASM_SPEC       "%{v*: -v}"

#undef  LINK_SPEC
#define LINK_SPEC      "%{v*: -v} \
%{mrose:	%{!noshrlib: %{pic-none: -noshrlib} %{!pic-none: -warn_nopic}} \
		%{nostdlib} %{noshrlib} %{glue}} \
%{!mrose:	%{dy} %{dn} %{glue: } \
		%{h*} %{z*} \
		%{static:-dn -Bstatic} \
		%{shared:-G -dy} \
		%{symbolic:-Bsymbolic -G -dy} \
		%{G:-G} \
		%{!dy: %{!dn: %{!static: %{!shared: %{!symbolic: \
			%{noshrlib: -dn } %{pic-none: -dn } \
			%{!noshrlib: %{!pic-none: -dy}}}}}}}}"

#undef TARGET_VERSION_INTERNAL
#undef TARGET_VERSION

#undef	I386_VERSION
#define I386_VERSION " 80386, ELF objects"

#define TARGET_VERSION_INTERNAL(STREAM) fputs (I386_VERSION, STREAM)
#define TARGET_VERSION TARGET_VERSION_INTERNAL (stderr)

#undef OBJECT_FORMAT_ROSE
