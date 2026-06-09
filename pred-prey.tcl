#!ecolab.exe

set density(list) 100
set nsp 2
set repro_rate(list) {.1 -.1}
set interaction(diag,list) {-.0001 0}
set interaction(offdiag,val) {-0.001 0.001}
set interaction(offdiag,row) {0 1}
set interaction(offdiag,col) {1 0}
set palette {black red}

proc simulate {} \
{ uplevel #0 \
	    {
	set running 1
	for {} {$running} { } \
		{ 
	    generate
	    display 0
	    .statusbar configure -text [get tstep]
	}
    }
}

source Xecolab.tcl


