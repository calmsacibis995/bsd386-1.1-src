# tk.tcl --
#
# Initialization script normally executed in the interpreter for each
# Tk-based application.  Arranges class bindings for widgets.
#
# $Header: /bsdi/MASTER/BSDI_OS/contrib/tk/library/tk.tcl,v 1.1.1.1 1994/01/03 23:16:11 polk Exp $ SPRITE (Berkeley)
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

# Insist on running with compatible versions of Tcl and Tk.

scan [info tclversion] "%d.%d" a b
if {$a != 7} {
    error "wrong version of Tcl loaded ([info tclversion]): need 7.x"
}
scan $tk_version "%d.%d" a b
if {($a != 3) || ($b < 5)} {
    error "wrong version of Tk loaded ($tk_version): need 3.5 or later"
}
unset a b

# Add Tk's directory to the end of the auto-load search path:

lappend auto_path $tk_library

# Turn off strict Motif look and feel as a default.

set tk_strictMotif 0

# ----------------------------------------------------------------------
# Class bindings for various flavors of button widgets.  $tk_priv(window)
# keeps track of the button containing the mouse $tk_priv(relief) saves
# the original relief of the button so it can be restored when the mouse
# button is released, and $tk_priv(buttonWindow) keeps track of the
# window in which the mouse button was pressed.
# ----------------------------------------------------------------------

bind Button <Any-Enter> {tk_butEnter %W}
bind Button <Any-Leave> {tk_butLeave %W}
bind Button <1> {tk_butDown %W}
bind Button <ButtonRelease-1> {tk_butUp %W}

bind Checkbutton <Any-Enter> {tk_butEnter %W}
bind Checkbutton <Any-Leave> {tk_butLeave %W}
bind Checkbutton <1> {tk_butDown %W}
bind Checkbutton <ButtonRelease-1> {tk_butUp %W}

bind Radiobutton <Any-Enter> {tk_butEnter %W}
bind Radiobutton <Any-Leave> {tk_butLeave %W}
bind Radiobutton <1> {tk_butDown %W}
bind Radiobutton <ButtonRelease-1> {tk_butUp %W}

# ----------------------------------------------------------------------
# Class bindings for entry widgets.
# ----------------------------------------------------------------------

bind Entry <1> {
    %W icursor @%x
    %W select from @%x
    if {[lindex [%W config -state] 4] == "normal"} {focus %W}
}
bind Entry <B1-Motion> {%W select to @%x}
bind Entry <Shift-1> {%W select adjust @%x}
bind Entry <Shift-B1-Motion> {%W select to @%x}
bind Entry <2> {%W scan mark %x}
bind Entry <B2-Motion> {%W scan dragto %x}
bind Entry <Any-KeyPress> {
    if {"%A" != ""} {
	%W insert insert %A
	tk_entrySeeCaret %W
    }
}
bind Entry <Delete> {tk_entryBackspace %W; tk_entrySeeCaret %W}
bind Entry <BackSpace> {tk_entryBackspace %W; tk_entrySeeCaret %W}
bind Entry <Control-h> {tk_entryBackspace %W; tk_entrySeeCaret %W}
bind Entry <Control-d> {%W delete sel.first sel.last; tk_entrySeeCaret %W}
bind Entry <Control-u> {%W delete 0 end}
bind Entry <Control-v> {%W insert insert [selection get]; tk_entrySeeCaret %W}
bind Entry <Control-w> {tk_entryBackword %W; tk_entrySeeCaret %W}
tk_bindForTraversal Entry

# ----------------------------------------------------------------------
# Class bindings for listbox widgets.
# ----------------------------------------------------------------------

bind Listbox <1> {%W select from [%W nearest %y]}
bind Listbox <B1-Motion> {%W select to [%W nearest %y]}
bind Listbox <Shift-1> {%W select adjust [%W nearest %y]}
bind Listbox <Shift-B1-Motion> {%W select to [%W nearest %y]}
bind Listbox <2> {%W scan mark %x %y}
bind Listbox <B2-Motion> {%W scan dragto %x %y}

# ----------------------------------------------------------------------
# Class bindings for scrollbar widgets.  When strict Motif is requested,
# the bindings use $tk_priv(buttons) and $tk_priv(activeFg) to set the
# -activeforeground color to -foreground when the mouse is in the window
# and restore it when the mouse leaves.
# ----------------------------------------------------------------------

bind Scrollbar <Any-Enter> {
    if $tk_strictMotif {
	set tk_priv(activeFg) [lindex [%W config -activeforeground] 4]
	%W config -activeforeground [lindex [%W config -foreground] 4]
    }
}
bind Scrollbar <Any-Leave> {
    if {$tk_strictMotif && ($tk_priv(buttons) == 0)} {
	%W config -activeforeground $tk_priv(activeFg)
    }
}
bind Scrollbar <Any-ButtonPress> {incr tk_priv(buttons)}
bind Scrollbar <Any-ButtonRelease> {incr tk_priv(buttons) -1}

# ----------------------------------------------------------------------
# Class bindings for scale widgets.  When strict Motif is requested,
# the bindings use $tk_priv(buttons) and $tk_priv(activeFg) to set the
# -activeforeground color to -foreground when the mouse is in the window
# and restore it when the mouse leaves.
# ----------------------------------------------------------------------

bind Scale <Any-Enter> {
    if $tk_strictMotif {
	set tk_priv(activeFg) [lindex [%W config -activeforeground] 4]
	%W config -activeforeground [lindex [%W config -sliderforeground] 4]
    }
}
bind Scale <Any-Leave> {
    if {$tk_strictMotif && ($tk_priv(buttons) == 0)} {
	%W config -activeforeground $tk_priv(activeFg)
    }
}
bind Scale <Any-ButtonPress> {incr tk_priv(buttons)}
bind Scale <Any-ButtonRelease> {incr tk_priv(buttons) -1}

# ----------------------------------------------------------------------
# Class bindings for menubutton widgets.  Variables used:
# $tk_priv(posted) -		keeps track of the menubutton whose menu is
#				currently posted (or empty string, if none).
# $tk_priv(inMenuButton)-	if non-null, identifies menu button
#				containing mouse pointer.
# $tk_priv(relief) -		keeps track of original relief of posted
#				menu button, so it can be restored later.
# $tk_priv(dragging) -		if non-null, identifies menu button whose
#				menu is currently being dragged in a tear-off
#				operation.
# $tk_priv(focus) -		records old focus window so focus can be
#				returned there after keyboard traversal
#				to menu.
# ----------------------------------------------------------------------

bind Menubutton <Any-Enter> {
    set tk_priv(inMenuButton) %W
    if {[lindex [%W config -state] 4] != "disabled"} {
	if {!$tk_strictMotif} {
	    %W config -state active
	}
    }
}
bind Menubutton <Any-Leave> {
    set tk_priv(inMenuButton) {}
    if {[lindex [%W config -state] 4] == "active"} {
	%W config -state normal
    }
}
bind Menubutton <1> {tk_mbButtonDown %W}
bind Menubutton <Any-ButtonRelease-1> {
    if {($tk_priv(posted) == "%W") && ($tk_priv(inMenuButton) == "%W")} {
	[lindex [$tk_priv(posted) config -menu] 4] activate 0
    } else {
	tk_mbUnpost
    }
}

# The binding below is trickier than it looks.  It's important to check
# to see that another menu is posted in the "if" statement below.
# The check is needed because some window managers (e.g. mwm in
# click-to-focus mode) cause a button-press event to be preceded by
# a B1-Enter event;  we don't want to process that B1-Enter event (if
# we do, the grab may get mis-set so that the menu is non-responsive).

bind Menubutton <B1-Enter> {
    set tk_priv(inMenuButton) %W
    if {([lindex [%W config -state] 4] != "disabled")
	    && ($tk_priv(posted) != "")} {
	if {!$tk_strictMotif} {
	    %W config -state active
	}
	tk_mbPost %W
    }
}
bind Menubutton <2> {
    if {($tk_priv(posted) == "")
	    && ([lindex [%W config -state] 4] != "disabled")} {
	set tk_priv(dragging) %W
	[lindex [$tk_priv(dragging) config -menu] 4] post %X %Y
    }
}
bind Menubutton <B2-Motion> {
    if {$tk_priv(dragging) != ""} {
	[lindex [$tk_priv(dragging) config -menu] 4] post %X %Y
    }
}
bind Menubutton <ButtonRelease-2> {set tk_priv(dragging) ""}

# ----------------------------------------------------------------------
# Class bindings for menu widgets.  $tk_priv(x) and $tk_priv(y) are used
# to keep track of the position of the mouse cursor in the menu window
# during dragging of tear-off menus.  $tk_priv(window) keeps track of
# the menu containing the mouse, if any.
# ----------------------------------------------------------------------

bind Menu <Any-Enter> {set tk_priv(window) %W; %W activate @%y}
bind Menu <Any-Leave> {set tk_priv(window) {}; %W activate none}
bind Menu <Any-Motion> {
    if {$tk_priv(window) == "%W"} {
	%W activate @%y
    }
}
bind Menu <1> {
    if {$tk_priv(grab) != ""} {
	grab $tk_priv(grab)
    }
}
bind Menu <ButtonRelease-1> {tk_invokeMenu %W}
bind Menu <2> {set tk_priv(x) %x; set tk_priv(y) %y}
bind Menu <B2-Motion> {
    if {$tk_priv(posted) == ""} {
	%W post [expr %X-$tk_priv(x)] [expr %Y-$tk_priv(y)]
    }
}
bind Menu <B2-Leave> { }
bind Menu <B2-Enter> { }
bind Menu <Escape> {tk_mbUnpost}
bind Menu <Any-KeyPress> {tk_traverseWithinMenu %W %A}
bind Menu <Left> {tk_nextMenu -1}
bind Menu <Right> {tk_nextMenu 1}
bind Menu <Up> {tk_nextMenuEntry -1}
bind Menu <Down> {tk_nextMenuEntry 1}
bind Menu <Return> {tk_invokeMenu %W}

# ----------------------------------------------------------------------
# Class bindings for text widgets. $tk_priv(selectMode) holds one of
# "char", "word", or "line" to indicate which selection mode is active.
# ----------------------------------------------------------------------

bind Text <1> {
    set tk_priv(selectMode) char
    %W mark set insert @%x,%y
    %W mark set anchor insert
    if {[lindex [%W config -state] 4] == "normal"} {focus %W}
}
bind Text <Double-1> {
    set tk_priv(selectMode) word
    %W mark set insert "@%x,%y wordstart"
    tk_textSelectTo %W insert
}
bind Text <Triple-1> {
    set tk_priv(selectMode) line
    %W mark set insert "@%x,%y linestart"
    tk_textSelectTo %W insert
}
bind Text <B1-Motion> {tk_textSelectTo %W @%x,%y}
bind Text <Shift-1> {
    tk_textResetAnchor %W @%x,%y
    tk_textSelectTo %W @%x,%y
}
bind Text <Shift-B1-Motion> {tk_textSelectTo %W @%x,%y}
bind Text <2> {%W scan mark %y}
bind Text <B2-Motion> {%W scan dragto %y}
bind Text <Any-KeyPress> {
    if {"%A" != ""} {
	%W insert insert %A
	%W yview -pickplace insert
    }
}
bind Text <Return> {%W insert insert \n; %W yview -pickplace insert}
bind Text <BackSpace> {tk_textBackspace %W; %W yview -pickplace insert}
bind Text <Delete> {tk_textBackspace %W; %W yview -pickplace insert}
bind Text <Control-h> {tk_textBackspace %W; %W yview -pickplace insert}
bind Text <Control-d> {%W delete sel.first sel.last}
bind Text <Control-v> {
    %W insert insert [selection get]
    %W yview -pickplace insert
}
tk_bindForTraversal Text

# Initialize the elements of tk_priv that require initialization.

set tk_priv(buttons) 0
set tk_priv(buttonWindow) {}
set tk_priv(dragging) {}
set tk_priv(focus) {}
set tk_priv(grab) {}
set tk_priv(inMenuButton) {}
set tk_priv(posted) {}
set tk_priv(selectMode) char
set tk_priv(window) {}
