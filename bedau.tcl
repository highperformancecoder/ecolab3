#!shadow.exe

set dims(x) 2
set dims(y) 2
set density(list) 100
set nsp 100

set repro_rate(random) true
set repro_rate(random,seed) 11
set repro_rate(random,maxval) .01
set repro_rate(random,minval) -.005
set interaction(diag,random) true
set interaction(diag,random,seed) 9
set interaction(diag,random,minval) -0.0001
set interaction(diag,random,maxval) -0.00005
set interaction(offdiag,random) true
set interaction(offdiag,random,minval) -0.0001
set interaction(offdiag,random,maxval) 0.0001
set interaction(offdiag,random,connectivity) 2
set interaction(offdiag,random,seed) 12
set mutation(random) true
set mutation(random,seed) 12
set mutation(random,maxval) .02
set migration(north_south,list) .01
set migration(east_west,list) .01

set sp_sep .1 
# 0.1 seems to be sufficiently small here

proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set j 0} {$j<100} {incr j} {
		for {set i 0} {$i<10} {incr i} {
		    generate
		}
		condense
		mutate
		migrate
	    }
	    set tstep [get tstep]
	    set nsp [totdiversity]
	    set activity [expr [act]/$nsp]
	    set new_activity [newact]

#	    puts stdout "$tstep $nsp $activity $new_activity"
	    plot nsp -title "Diversity/Activity" tstep nsp activity new_activity
	    .statusbar configure -text "t:[get tstep] nsp:[get nsp]"
	}
    }
}

set display_scale 3
set palette {black red green blue magenta cyan yellow}

source Xecolab.tcl

