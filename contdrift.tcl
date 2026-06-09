#!/bin/sh
#PBS -l cput=300:0:0 -l ncpus=4
#\
if [ ! -z "$PBS_O_WORKDIR" ]; then cd $PBS_O_WORKDIR; fi
#\
exec ./ecolab.exe contdrift.tcl -np 4

set density(list) 100
set dims(x) 2
set dims(y) 2
set nsp(0,0) 100
set nsp(1,0) 99
set nsp(0,1) 98
set nsp(1,1) 97
#set dims(x) 3
#set dims(y) 3
#set nsp(0,0) 100
#set nsp(1,0) 99
#set nsp(2,0) 98
#set nsp(0,1) 97
#set nsp(1,1) 96
#set nsp(2,1) 95
#set nsp(0,2) 94
#set nsp(1,2) 93
#set nsp(2,2) 92


set repro_rate(random) true
#set repro_rate(random,seed) 11
set repro_rate(random,seed) 21
set repro_rate(random,maxval) 1e-2
set repro_rate(random,minval) -5e-3
set interaction(diag,random) true
#set interaction(diag,random,seed) 9
set interaction(diag,random,seed) 19
set interaction(diag,random,minval) -1e-4
set interaction(diag,random,maxval) -5e-5
set interaction(offdiag,random) true
set interaction(offdiag,random,minval) -1e-4
set interaction(offdiag,random,maxval) 1e-4
set interaction(offdiag,random,connectivity) 2
#set interaction(offdiag,random,seed) 12
set interaction(offdiag,random,seed) 22
set mutation(random) true
#set mutation(random,seed) 12
set mutation(random,seed) 22
set mutation(random,maxval) 1e-4
set migration(north_south,list) .01
set migration(east_west,list) .01

set sp_sep .1 
# 0.1 seems to be sufficiently small here




set dat [open "contdrift.dat" w]

set migfact 0.9


proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set i 0} {$i<100} {incr i} {
		generate
	    }
	    mutate
	    set tstep [get tstep]
	    if {$tstep % 1000==0} migrate				       

#	    connect_plot
	    condense

	    if {$tstep % 1000==0} {
		set maxmig [max \
	    [concat [get migration(east_west)] [get migration(north_south)]]]
		set diversity [totdiversity]
		puts $dat "$tstep $maxmig $diversity"
		flush $dat
#		plot nsp tstep diversity
	    }

	    if {$tstep % 10000 == 0} {scalemig $migfact} 
	    if {$tstep % 1740000 == 0} {set migfact [expr 1/$migfact]}
	    if {$tstep % 1000000 == 0} {checkpoint "contdrift.$tstep.ckpt"}

#	     set nspl [get nsp]
#	     set cell0 [lindex $nspl 0]
#	     set cell1 [lindex $nspl 1]
#	     set cell2 [lindex $nspl 2]
#	     set cell3 [lindex $nspl 3]
#	     plot nsp -title "No. of Species" tstep cell0 cell1 cell2 cell3
#	     display 0
#	     display 1
#	     display 2
#	     display 3
#	     histogram diversity $nspl
#	     .statusbar configure -text "t:[get tstep] nsp:[get nsp]"
	    }
    }
}

#source "Xecolab.tcl"
simulate
