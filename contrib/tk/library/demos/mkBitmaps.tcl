# mkBitmaps w
#
# Create a top-level window that displays all of Tk's built-in bitmaps.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkBitmaps {{w .bitmaps}} {
    global tk_library
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Bitmap Demonstration"
    wm iconname $w "Bitmaps"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -width 4i \
	    -text "This window displays all of Tk's built-in bitmaps, along with the names you can use for them in Tcl scripts.  Click the \"OK\" button when you've seen enough."
    frame $w.frame
    bitmapRow $w.frame.0 error gray25 gray50 hourglass
    bitmapRow $w.frame.1 info question questhead warning
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg -side top -anchor center
    pack $w.frame -side top -expand yes -fill both
    pack $w.ok -side bottom -fill both
}

# The procedure below creates a new row of bitmaps in a window.  Its
# arguments are:
#
# w -		The window that is to contain the row.
# args -	The names of one or more bitmaps, which will be displayed
#		in a new row across the bottom of w along with their
#		names.

proc bitmapRow {w args} {
    frame $w
    pack $w -side top -fill both
    set i 0
    foreach bitmap $args {
	frame $w.$i
	pack $w.$i -side left -fill both -pady .25c -padx .25c
	label $w.$i.bitmap -bitmap $bitmap
	label $w.$i.label -text $bitmap -width 9
	pack $w.$i.label $w.$i.bitmap -side bottom
	incr i
    }
}
