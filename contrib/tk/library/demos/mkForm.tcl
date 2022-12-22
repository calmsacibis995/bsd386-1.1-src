# mkForm w
#
# Create a top-level window that displays a bunch of entries with
# tabs set up to move between them.
#
# Arguments:
#    w -	Name to use for new top-level window.

proc mkForm {{w .form}} {
    global tabList
    catch {destroy $w}
    toplevel $w
    dpos $w
    wm title $w "Form Demonstration"
    wm iconname $w "Form"
    message $w.msg -font -Adobe-times-medium-r-normal--*-180* -width 4i \
	    -text "This window contains a simple form where you can type in the various entries and use tabs to move circularly between the entries.  Click the \"OK\" button or type return when you're done."
    foreach i {f1 f2 f3 f4 f5} {
	frame $w.$i -bd 1m
	entry $w.$i.entry -relief sunken -width 40
	bind $w.$i.entry <Tab> "Tab \$tabList"
	bind $w.$i.entry <Return> "destroy $w"
	label $w.$i.label
	pack $w.$i.entry -side right
	pack $w.$i.label -side left
    }
    $w.f1.label config -text Name:
    $w.f2.label config -text Address:
    $w.f5.label config -text Phone:
    button $w.ok -text OK -command "destroy $w"
    pack $w.msg $w.f1 $w.f2 $w.f3 $w.f4 $w.f5 $w.ok -side top -fill x
    set tabList "$w.f1.entry $w.f2.entry $w.f3.entry $w.f4.entry $w.f5.entry"
}

# The procedure below is invoked in response to tabs in the entry
# windows.  It moves the focus to the next window in the tab list.
# Arguments:
#
# list -	Ordered list of windows to receive focus

proc Tab {list} {
    set i [lsearch $list [focus]]
    if {$i < 0} {
	set i 0
    } else {
	incr i
	if {$i >= [llength $list]} {
	    set i 0
	}
    }
    focus [lindex $list $i]
}
