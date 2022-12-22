# mkButton w
#
# Create a top-level window that displays a bunch of buttons.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkButton {{w .b1}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Button Demonstration"
    wm iconname $w "Buttons"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "Four buttons are displayed below.  If you click on a button, it will change the background of the button area to the color indicated in the button.  Click the \"OK\" button when you've seen enough."
    frame $w.frame -borderwidth 10
    button $w.ok -text OK -command "destroy $w"

    pack $w.msg -side top -fill both
    pack $w.frame -side top -expand yes -fill both
    pack $w.ok -side bottom -fill both

    button $w.frame.b1 -text "Peach Puff" \
	    -command "$w.frame config -bg PeachPuff1"
    button $w.frame.b2 -text "Light Blue" \
	    -command "$w.frame config -bg LightBlue1"
    button $w.frame.b3 -text "Sea Green" \
	    -command "$w.frame config -bg SeaGreen2"
    button $w.frame.b4 -text "Yellow" \
	    -command "$w.frame config -bg Yellow1"
    pack $w.frame.b1 $w.frame.b2 $w.frame.b3 $w.frame.b4 -side top \
	    -expand yes -pady 2
}
