# mkVScale w
#
# Create a top-level window that displays a vertical scale.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkVScale {{w .scale1}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Vertical Scale Demonstration"
    wm iconname $w "Scale"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "A bar and a vertical scale are displayed below.  If you click or drag mouse button 1 in the scale, you can change the height of the bar.  Click the \"OK\" button when you're finished."
    frame $w.frame -borderwidth 10
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg $w.frame $w.ok

    scale $w.frame.scale -orient vertical -length 280 -from 0 -to 250 \
	    -command "setHeight $w.frame.right.inner" -tickinterval 50 \
	    -bg Bisque1
    frame $w.frame.right -borderwidth 15
    pack $w.frame.scale -side left -anchor ne
    pack $w.frame.right -side left -anchor nw
    $w.frame.scale set 20

    frame $w.frame.right.inner -geometry 40x20 -relief raised \
	    -borderwidth 2 -bg SteelBlue1
    pack $w.frame.right.inner -expand yes -anchor nw
}

proc setHeight {w height} {
    $w config -geometry 40x${height}
}
