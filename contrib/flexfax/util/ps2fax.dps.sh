#!/bin/sh
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/ps2fax.dps.sh,v 1.1.1.1 1994/01/14 23:10:51 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
# Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#
# Convert PostScript to facsimile.
#
# ps2fax [-o output] [-l pagelength] [-w pagewidth]
#	[-r resolution] [-*] file ...
#
# We need to process the arguments to extract the input
# files so that we can prepend a prologue file that defines
# LaserWriter-specific stuff as well as to insert and error
# handler that generates ASCII diagnostic messages when
# a problem is encountered in the interpreter.
#
# NB: this shell script is assumed to be run from the
#     top of the spooling hierarchy -- s.t. the etc directory
#     is present.
#
PS=/usr/local/bin/ps2fax
fil=
opt=
while test $# != 0
do case "$1" in
    -o)	shift; opt="$opt -o $1" ;;
    -l)	shift; opt="$opt -l $1" ;;
    -w)	shift; opt="$opt -w $1" ;;
    -r)	shift; opt="$opt -r $1" ;;
    -*)	opt="$opt $1" ;;
    *)	fil="$fil $1" ;;
    esac
    shift
done
/bin/cat etc/dpsprinter.ps $fil | $PS $opt
