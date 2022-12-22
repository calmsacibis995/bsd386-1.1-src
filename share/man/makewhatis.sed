#!/bin/sh -
#
# Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	$Id: makewhatis.sed,v 1.2 1992/09/24 16:22:46 sanders Exp $
#
# CAVET:
#     This script does not work as well as makewhatis.perl.  It is
#     suggested that you use makewhatis.perl.
#
# Copyright (c) 1988 The Regents of the University of California.
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
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
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
#	from @(#)makewhatis.sed	5.5 (Berkeley) 4/17/91
#

:section
    s/.*(\([^)]*\).*/(\1)/;	# match section number
    t ok
    d;				# no match, try again
:ok
    h;				# matched! copy section number to hold space

# look for 
:name
    s/.*//; N; s/\n//g;		# next line
    s/.//g;			# remove any bold/underlining
    /^NAME/!b name

# gobble lines upto a blank line
:desc
    s/.*//; N; s/\n//;		# next line
    s/^[ 	]*//;		# remove any leading white space
    /^$/b print
    s/.//g;			# remove any bold/underlining
    H;				# append to hold space
    b desc

:print
    g;				# dup hold into pattern

    # description =
    s/-\n[ 	]*//g;
    s/\n/ /g
    s/^(\([^)]*\))[^-]*-*[ 	]*\(.*\)/(\1) - \2/;
    s/^[ 	]*//; s/[ 	][ 	]*/ /g; s/[ 	]*$//;

    x;				# swap hold space and pattern space

    # name =
    s/\n/ /g
    s/^(\([^)]*\))\([^-]*\).*/\2/;
    s/^[ 	]*//; s/[ 	][ 	]*/ /g; s/[ 	]*$//;

    G; s/\n/ /; p; q
