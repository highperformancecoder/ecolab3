#!newman.exe

set nsp 100
set threshold(random) true
set mutation 0.001
set mut1 0.01
set sigma 1e-2

set lifedat [open "lifetime.dat" "a+"]
#set nspdat  [open "nsp.dat" "w"]

#gets stdin restart
#if {$restart} {reload "ckpt"}
#trap {
#    checkpoint "ckpt"
#    flush $lifedat
#}
#

proc simulate {} \
{ uplevel #0 \
  {
    set running 1
    for {} {$running} { } \
	    { 
	for {set i 0} {$i<10} {incr i} {
	    step
	}

	set lt [lifetimes]
	if {[llength $lt] > 0} {
	    puts $lifedat $lt
	    flush $lifedat
	}
#	 puts $nspdat [get nsp]
#	 flush $nspdat
	histogram lifetimes $lt
	condense

	set nsp [get nsp]
	set tstep [get tstep]
#	plot nsp  -title "No. of Species" tstep nsp
	.statusbar configure -text "t=$tstep nsp=$nsp"
        update 
      }
  }
}

puts stdout "I'm here"
source Xecolab.tcl
#simulate
#exit
