# button.tcl --
#
# This file contains Tcl procedures used to manage Tk buttons.
#
# $Header: /bsdi/MASTER/BSDI_OS/contrib/tk/library/button.tcl,v 1.1.1.1 1994/01/03 23:16:05 polk Exp $ SPRITE (Berkeley)
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

# The procedure below is invoked when the mouse pointer enters a
# button widget.  It records the button we're in and changes the
# state of the button to active unless the button is disabled.

proc tk_butEnter w {
    global tk_priv tk_strictMotif
    if {[lindex [$w config -state] 4] != "disabled"} {
	if {!$tk_strictMotif} {
	    $w config -state active
	}
	set tk_priv(window) $w
    }
}

# The procedure below is invoked when the mouse pointer leaves a
# button widget.  It changes the state of the button back to
# inactive.

proc tk_butLeave w {
    global tk_priv tk_strictMotif
    if {[lindex [$w config -state] 4] != "disabled"} {
	if {!$tk_strictMotif} {
	    $w config -state normal
	}
    }
    set tk_priv(window) ""
}

# The procedure below is invoked when the mouse button is pressed in
# a button/radiobutton/checkbutton widget.  It records information
# (a) to indicate that the mouse is in the button, and
# (b) to save the button's relief so it can be restored later.

proc tk_butDown w {
    global tk_priv
    set tk_priv(relief) [lindex [$w config -relief] 4]
    set tk_priv(buttonWindow) $w
    if {[lindex [$w config -state] 4] != "disabled"} {
	$w config -relief sunken
    }
}

# The procedure below is invoked when the mouse button is released
# for a button/radiobutton/checkbutton widget.  It restores the
# button's relief and invokes the command as long as the mouse
# hasn't left the button.

proc tk_butUp w {
    global tk_priv
    if {$w == $tk_priv(buttonWindow)} {
	$w config -relief $tk_priv(relief)
	if {($w == $tk_priv(window))
		&& ([lindex [$w config -state] 4] != "disabled")} {
	    uplevel #0 [list $w invoke]
	}
	set tk_priv(buttonWindow) ""
    }
}
