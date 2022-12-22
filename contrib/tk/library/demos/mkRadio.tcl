# mkRadio w
#
# Create a top-level window that displays a bunch of radio buttons.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkRadio {{w .r1}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Radiobutton Demonstration"
    wm iconname $w "Radiobuttons"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "Two groups of radiobuttons are displayed below.  If you click on a button then the button will become selected exclusively among all the buttons in its group.  A Tcl variable is associated with each group to indicate which of the group's buttons is selected.  Click the \"See Variables\" button to see the current values of the variables.  Click the \"OK\" button when you've seen enough."
    frame $w.frame -borderwidth 10
    frame $w.frame2
    pack $w.msg -side top
    pack $w.msg -side top
    pack $w.frame -side top -fill x -pady 10
    pack $w.frame2 -side bottom -fill x

    frame $w.frame.left
    frame $w.frame.right
    pack $w.frame.left $w.frame.right -side left -expand yes

    radiobutton $w.frame.left.b1 -text "Point Size 10" -variable size \
	    -relief flat -value 10
    radiobutton $w.frame.left.b2 -text "Point Size 12" -variable size \
	    -relief flat -value 12
    radiobutton $w.frame.left.b3 -text "Point Size 18" -variable size \
	    -relief flat -value 18
    radiobutton $w.frame.left.b4 -text "Point Size 24" -variable size \
	    -relief flat -value 24
    pack $w.frame.left.b1 $w.frame.left.b2 $w.frame.left.b3 $w.frame.left.b4 \
	    -side top -pady 2 -anchor w

    radiobutton $w.frame.right.b1 -text "Red" -variable color \
	    -relief flat -value red
    radiobutton $w.frame.right.b2 -text "Green" -variable color \
	    -relief flat -value green
    radiobutton $w.frame.right.b3 -text "Blue" -variable color \
	    -relief flat -value blue
    radiobutton $w.frame.right.b4 -text "Yellow" -variable color \
	    -relief flat -value yellow
    radiobutton $w.frame.right.b5 -text "Orange" -variable color \
	    -relief flat -value orange
    radiobutton $w.frame.right.b6 -text "Purple" -variable color \
	    -relief flat -value purple
    pack $w.frame.right.b1 $w.frame.right.b2 $w.frame.right.b3 \
	    $w.frame.right.b4 $w.frame.right.b5 $w.frame.right.b6 \
	    -side top -pady 2 -anchor w

    button $w.frame2.ok -text OK -command "destroy $w" -width 12
    button $w.frame2.vars -text "See Variables" -width 12\
	    -command "showVars $w.dialog size color"
    pack $w.frame2.ok $w.frame2.vars -side left -expand yes -fill x
}
