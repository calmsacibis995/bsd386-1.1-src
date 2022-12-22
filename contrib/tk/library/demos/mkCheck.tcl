# mkCheck w
#
# Create a top-level window that displays a bunch of check buttons.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkCheck {{w .c1}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Checkbutton demonstration"
    wm iconname $w "Checkbuttons"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "Three checkbuttons are displayed below.  If you click on a button, it will toggle the button's selection state and set a Tcl variable to a value indicating the state of the checkbutton.  Click the \"See Variables\" button to see the current values of the variables.  Click the \"OK\" button when you've seen enough."
    frame $w.frame -borderwidth 10
    frame $w.frame2

    pack $w.msg -side top -fill both
    pack $w.frame -side top -expand yes -fill both
    pack $w.frame2 -side bottom -fill both

    checkbutton $w.frame.b1 -text "Wipers OK" -variable wipers -relief flat
    checkbutton $w.frame.b2 -text "Brakes OK" -variable brakes -relief flat
    checkbutton $w.frame.b3 -text "Driver Sober" -variable sober -relief flat
    pack $w.frame.b1 $w.frame.b2 $w.frame.b3 -side top -pady 2 -expand yes \
	    -anchor w

    button $w.frame2.ok -text OK -command "destroy $w"
    button $w.frame2.vars -text "See Variables" \
	    -command "showVars $w.dialog wipers brakes sober"
    pack $w.frame2.ok $w.frame2.vars -side left -expand yes -fill both
}
