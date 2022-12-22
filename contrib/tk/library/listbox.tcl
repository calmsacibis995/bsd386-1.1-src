# listbox.tcl --
#
# This file contains Tcl procedures used to manage Tk listboxes.
#
# $Header: /bsdi/MASTER/BSDI_OS/contrib/tk/library/listbox.tcl,v 1.1.1.1 1994/01/03 23:16:06 polk Exp $ SPRITE (Berkeley)
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

# The procedure below may be invoked to change the behavior of
# listboxes so that only a single item may be selected at once.
# The arguments give one or more windows whose behavior should
# be changed;  if one of the arguments is "Listbox" then the default
# behavior is changed for all listboxes.

proc tk_listboxSingleSelect args {
    foreach w $args {
	bind $w <B1-Motion> {%W select from [%W nearest %y]} 
	bind $w <Shift-1> {%W select from [%W nearest %y]}
	bind $w <Shift-B1-Motion> {%W select from [%W nearest %y]}
    }
}
