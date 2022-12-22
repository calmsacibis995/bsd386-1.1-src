# This file contains a default version of the tkError procedure.  It
# posts a dialog box with the error message and gives the user a chance
# to see a more detailed stack trace.

proc tkerror err {
    global errorInfo
    set info $errorInfo
    if {[tk_dialog .tkerrorDialog "Error in Tcl Script" \
	    "Error: $err" error 0 OK "See Stack Trace"] == 0} {
	return
    }

    set w .tkerrorTrace
    catch {destroy $w}
    toplevel $w -class ErrorTrace
    wm minsize $w 1 1
    wm title $w "Stack Trace for Error"
    wm iconname $w "Stack Trace"
    button $w.ok -text OK -command "destroy $w"
    text $w.text -relief raised -bd 2 -yscrollcommand "$w.scroll set" \
	    -setgrid true -width 40 -height 10
    scrollbar $w.scroll -relief flat -command "$w.text yview"
    pack $w.ok -side bottom -padx 3m -pady 3m -ipadx 2m -ipady 1m
    pack $w.scroll -side right -fill y
    pack $w.text -side left -expand yes -fill both
    $w.text insert 0.0 $info
    $w.text mark set insert 0.0

    # Center the window on the screen.

    wm withdraw $w
    update idletasks
    set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - [winfo vrootx [winfo parent $w]]]
    set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - [winfo vrooty [winfo parent $w]]]
    wm geom $w +$x+$y
    wm deiconify $w
}
