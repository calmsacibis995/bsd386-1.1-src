#! /bin/sh
#
# systags.sh - construct a system tags file from a system Makefile.
#
# $Header: /bsdi/MASTER/BSDI_OS/usr.bin/gdb/gdb/grot/systags.sh,v 1.1.1.1 1992/08/27 17:03:02 trent Exp $ 
#
# Copyright (c) 1992 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by the University of
#      California, Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# First written May 16, 1992 by Van Jacobson, Lawrence Berkeley Laboratory.
#
	set -v
	rm -f tags tags.tmp tags.cfiles
	awk   '/^# DO NOT DELETE THIS LINE/ { independ = 1 }
		independ {
			for (i = 1; i <= NF; ++i) {
				if (index($i, "$*"))
					continue;
				if (j = index($i, "="))
					$i = substr($i, j+1);
				if ($i ~ /\.[chy]$/)
					cfiles[$i] = 1;
				else if ($i ~ /\.cc$/)
					cfiles[$i] = 1;
			}
		};
		END {
			for (i in cfiles)
				if (cfiles[i] > 0)
					print i > "tags.cfiles";
		}' Makefile
	/usr/src/ucb/ctags/ctags -C -t -d -w `cat tags.cfiles`
	mv tags tags.tmp
	sort -u tags.tmp > tags
	rm -f tags.tmp tags.cfiles
