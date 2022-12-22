# entry.tcl --
#
# This file contains Tcl procedures used to manage Tk entries.
#
# $Header: /bsdi/MASTER/BSDI_OS/contrib/tk/library/entry.tcl,v 1.1.1.1 1994/01/03 23:16:06 polk Exp $ SPRITE (Berkeley)
#
# Copyright (c) 1992-1993 The Regents of the University of California.
# All rights reserved.
#
# Permission is hereby granted, without written agreement and without
# license or royalty fees, to use, copy, modify, and distribute this
# software and its documentation for any purpose, provided that the
# above copyright notice and the following two paragraphs appear in
# all copies of this software.
#
# IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
# OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
# CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
#

# The procedure below is invoked to backspace over one character
# in an entry widget.  The name of the widget is passed as argument.

proc tk_entryBackspace w {
    set x [expr {[$w index insert] - 1}]
    if {$x != -1} {$w delete $x}
}

# The procedure below is invoked to backspace over one word in an
# entry widget.  The name of the widget is passed as argument.

proc tk_entryBackword w {
    set string [$w get]
    set curs [expr [$w index insert]-1]
    if {$curs < 0} return
    for {set x $curs} {$x > 0} {incr x -1} {
	if {([string first [string index $string $x] " \t"] < 0)
		&& ([string first [string index $string [expr $x-1]] " \t"]
		>= 0)} {
	    break
	}
    }
    $w delete $x $curs
}

# The procedure below is invoked after insertions.  If the caret is not
# visible in the window then the procedure adjusts the entry's view to
# bring the caret back into the window again.  Also, try to keep at
# least one character visible to the left of the caret.

proc tk_entrySeeCaret w {
    set c [$w index insert]
    set left [$w index @0]
    if {$left >= $c} {
	if {$c > 0} {
	    $w view [expr $c-1]
	} else {
	    $w view $c
	}
	return
    }
    set x [expr [winfo width $w] - [lindex [$w config -bd] 4] - 1]
    while {([$w index @$x] < $c) && ($left < $c)} {
	set left [expr $left+1]
	$w view $left
    }
}
