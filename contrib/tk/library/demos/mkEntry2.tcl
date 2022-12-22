# mkEntry2 -
#
# Create a top-level window that displays a bunch of entries with
# scrollbars.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkEntry2 {{w .e2}} {
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Entry Demonstration"
    wm iconname $w "Entries"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -aspect 200 \
	    -text "Three different entries are displayed below, with a scrollbar for each entry.  You can add characters by pointing, clicking and typing.  You can delete by selecting and typing Control-d.  Backspace, Control-h, and Delete may be typed to erase the character just before the insertion point, Control-W erases the word just before the insertion point, and Control-u clears the entry.  For entries that are too large to fit in the window all at once, you can scan through the entries using the scrollbars, or by dragging with mouse button 2 pressed.  Click the \"OK\" button when you've seen enough."
    frame $w.frame -borderwidth 10
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg $w.frame $w.ok -side top -fill both

    entry $w.frame.e1 -relief sunken -scroll "$w.frame.s1 set"
    scrollbar $w.frame.s1 -relief sunken -orient horiz -command \
	    "$w.frame.e1 view"
    frame $w.frame.f1 -geometry 20x10
    entry $w.frame.e2 -relief sunken -scroll "$w.frame.s2 set"
    scrollbar $w.frame.s2 -relief sunken -orient horiz -command \
	    "$w.frame.e2 view"
    frame $w.frame.f2 -geometry 20x10
    entry $w.frame.e3 -relief sunken -scroll "$w.frame.s3 set"
    scrollbar $w.frame.s3 -relief sunken -orient horiz -command \
	    "$w.frame.e3 view"
    pack $w.frame.e1 $w.frame.s1 $w.frame.f1 $w.frame.e2 $w.frame.s2 \
	    $w.frame.f2 $w.frame.e3 $w.frame.s3 -side top -fill x

    $w.frame.e1 insert 0 "Initial value"
    $w.frame.e2 insert end "This entry contains a long value, much too long "
    $w.frame.e2 insert end "to fit in the window at one time, so long in fact "
    $w.frame.e2 insert end "that you'll have to scan or scroll to see the end."
}
