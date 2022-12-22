# showVars w var var var ...
#
# Create a top-level window that displays a bunch of global variable values
# and keeps the display up-to-date even when the variables change value
#
# Arguments:
#    w -	Name to use for new top-level window.
#    var -	Name of variable to monitor.

proc showVars {w args} {
    catch {destroy $w}
    toplevel $w
    wm title $w "Variable values"
    label $w.title -text "Variable values:" -width 20 -anchor center \
	    -font -Adobe-helvetica-medium-r-normal--*-180*
    pack $w.title -side top -fill x
    foreach i $args {
	frame $w.$i
	label $w.$i.name -text "$i: "
	label $w.$i.value -textvar $i
	pack $w.$i.name $w.$i.value -side left
	pack $w.$i -side top -anchor w
    }
    button $w.ok -text OK -command "destroy $w"
    pack $w.ok -side bottom -pady 2
}
