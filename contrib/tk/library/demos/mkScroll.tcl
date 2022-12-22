# mkScroll w
#
# Create a top-level window containing a simple canvas that can
# be scrolled in two dimensions.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkScroll {{w .cscroll}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Scrollable Canvas Demonstration"
    wm iconname $w "Canvas"
    wm minsize $w 100 100
    set c $w.frame.c

    message $w.msg -font -Adobe-Times-Medium-R-Normal-*-180-* -aspect 300 \
	    -relief raised -bd 2 -text "This window displays a canvas widget that can be scrolled either using the scrollbars or by dragging with button 2 in the canvas.  If you click button 1 on one of the rectangles, its indices will be printed on stdout."
    frame $w.frame -relief raised -bd 2
    button $w.ok -text "OK" -command "destroy $w"
    pack $w.msg -side top -fill x
    pack $w.ok -side bottom -pady 5
    pack $w.frame -side top -expand yes -fill both

    canvas $c -scrollregion {-10c -10c 50c 20c} \
	    -xscroll "$w.frame.hscroll set" -yscroll "$w.frame.vscroll set"
    scrollbar $w.frame.vscroll  -relief sunken -command "$c yview"
    scrollbar $w.frame.hscroll -orient horiz -relief sunken -command "$c xview"
    pack $w.frame.vscroll -side right -fill y
    pack $w.frame.hscroll -side bottom -fill x
    pack $c -expand yes -fill both

    set bg [lindex [$c config -bg] 4]
    for {set i 0} {$i < 20} {incr i} {
	set x [expr {-10 + 3*$i}]
	for {set j 0; set y -10} {$j < 10} {incr j; incr y 3} {
	    $c create rect ${x}c ${y}c [expr $x+2]c [expr $y+2]c \
		    -outline black -fill $bg -tags rect
	    $c create text [expr $x+1]c [expr $y+1]c -text "$i,$j" \
		-anchor center -tags text
	}
    }

    $c bind all <Any-Enter> "scrollEnter $c"
    $c bind all <Any-Leave> "scrollLeave $c"
    $c bind all <1> "scrollButton $c"
    bind $c <2> "$c scan mark %x %y"
    bind $c <B2-Motion> "$c scan dragto %x %y"
}

proc scrollEnter canvas {
    global oldFill
    set id [$canvas find withtag current]
    if {[lsearch [$canvas gettags current] text] >= 0} {
	set id [expr $id-1]
    }
    set oldFill [lindex [$canvas itemconfig $id -fill] 4]
    if {[tk colormodel $canvas] == "color"} {
	$canvas itemconfigure $id -fill SeaGreen1
    } else {
	$canvas itemconfigure $id -fill black
	$canvas itemconfigure [expr $id+1] -fill white
    }
}

proc scrollLeave canvas {
    global oldFill
    set id [$canvas find withtag current]
    if {[lsearch [$canvas gettags current] text] >= 0} {
	set id [expr $id-1]
    }
    $canvas itemconfigure $id -fill $oldFill
    $canvas itemconfigure [expr $id+1] -fill black
}

proc scrollButton canvas {
    global oldFill
    set id [$canvas find withtag current]
    if {[lsearch [$canvas gettags current] text] < 0} {
	set id [expr $id+1]
    }
    puts stdout "You buttoned at [lindex [$canvas itemconf $id -text] 4]"
}
