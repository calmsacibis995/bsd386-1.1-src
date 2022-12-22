# mkLabel w
#
# Create a top-level window that displays a bunch of labels.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkLabel {{w .l1}} {
    global tk_library
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Label Demonstration"
    wm iconname $w "Labels"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "Five labels are displayed below: three textual ones on the left, and a bitmap label and a text label on the right.  Labels are pretty boring because you can't do anything with them.  Click the \"OK\" button when you've seen enough."
    frame $w.left
    frame $w.right
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg -side top
    pack $w.ok -side bottom -fill x
    pack $w.left $w.right -side left -expand yes -padx 10 -pady 10 -fill both

    label $w.left.l1 -text "First label"
    label $w.left.l2 -text "Second label, raised just for fun" -relief raised
    label $w.left.l3 -text "Third label, sunken" -relief sunken
    pack $w.left.l1 $w.left.l2 $w.left.l3 \
	    -side top -expand yes -pady 2 -anchor w

    label $w.right.bitmap -bitmap @$tk_library/demos/bitmaps/face \
	    -borderwidth 2 -relief sunken
    label $w.right.caption -text "Tcl/Tk Proprietor"
    pack $w.right.bitmap $w.right.caption -side top
}
