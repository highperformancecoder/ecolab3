#!ecolab.exe


proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set i 0} {$i<100} {incr i} {
		generate
	    }
	    mutate
	    migrate
	    

	    connect_plot
	    condense
	    set nspl [get nsp]
	    set cell0 [lindex $nspl 0]
	    set cell1 [lindex $nspl 1]
	    set cell2 [lindex $nspl 2]
	    set cell3 [lindex $nspl 3]
	    set tstep [get tstep]
	    plot nsp -title "No. of Species" tstep cell0 cell1 cell2 cell3
	    display 0
	    display 1
	    display 2
	    display 3
	    histogram diversity $nspl
	    .statusbar configure -text "t:[get tstep] nsp:[get nsp]"
	}
    }
}

source model.tcl

set display_scale 3
set palette {black red green blue magenta cyan yellow}
source Xecolab.tcl
