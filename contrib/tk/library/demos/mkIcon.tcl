# mkIcon w
#
# Create a top-level window that displays a bunch of iconic
# buttons.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkIcon {{w .icon}} {
    global tk_library
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Iconic Button Demonstration"
    wm iconname $w "Icons"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 300 \
	    -text "This window shows three buttons that display bitmaps instead of text.  On the left is a regular button, which changes its bitmap when you click on it.  On the right are two radio buttons.  Click the \"OK\" button when you're done."
    frame $w.frame -borderwidth 10
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg -side top
    pack $w.frame $w.ok -side top -fill x

    button $w.frame.b1 -bitmap @$tk_library/demos/bitmaps/flagdown  \
		-command "iconCmd $w.frame.b1"
    frame $w.frame.right
    pack $w.frame.b1 $w.frame.right -side left -expand yes

    radiobutton $w.frame.right.b2 -bitmap @$tk_library/demos/bitmaps/letters \
	    -variable letters
    radiobutton $w.frame.right.b3 -bitmap @$tk_library/demos/bitmaps/noletters \
	    -variable letters
    pack $w.frame.right.b2 $w.frame.right.b3 -side top -expand yes
}

proc iconCmd {w} {
    global tk_library
    set bitmap [lindex [$w config -bitmap] 4]
    if {$bitmap == "@$tk_library/demos/bitmaps/flagdown"} {
	$w config -bitmap @$tk_library/demos/bitmaps/flagup
    } else {
	$w config -bitmap @$tk_library/demos/bitmaps/flagdown
    }
}
