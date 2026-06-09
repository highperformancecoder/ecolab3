#!ecolab.exe

set host localhost
set port 7000
set palette {black red green blue magenta cyan yellow}
set model_valid 0

proc simulate {} {
    uplevel #0 {
        if {!$running} return
	get_global_vars $host $port
	after 1000 simulate
	if {$model_valid} {
	    display 0
	    .statusbar configure -text "tstep: [get tstep]"
	}
    }
}


source Xecolab.tcl
