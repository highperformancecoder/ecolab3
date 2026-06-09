# X-window stuff to go here
Tkinit
set tcl_library /usr/lib/tcl8.0
set tk_library /usr/lib/tk8.0
source [info library]/init.tcl
source $tk_library/tk.tcl
wm geometry . 400x50
wm deiconify .
tk appname [file rootname [file tail $argv(0)]]
wm title . [file rootname [file tail $argv(0)]]

# stuff for BLT
if [file exists ../library] {
    set blt_library ../library
}
lappend auto_path $blt_library

if { [info commands "namespace"] == "namespace" } {
    if { $tcl_version >= 8.0 } {
        namespace import blt::*
    } else {
        catch { import add blt }
    }
    if { $tcl_version >= 8.0 } {
        namespace import -force blt::tile::*
    } else {
        import add blt::tile
    }
} else {
    foreach cmd { button checkbutton radiobutton frame label 
        scrollbar toplevel menubutton listbox } {
        if { [info command tile${cmd}] == "tile${cmd}" } {
            rename ${cmd} ""
            rename tile${cmd} ${cmd}
        }
    }
}

# Top level button bar
frame .buttonbar -background  "navajo white" -width 700 -height 30
pack append . .buttonbar top

button .quit -text quit -command exit_ecolab

button .run -text run -command {
    .run configure -relief sunken
    .stop configure -relief raised
    set running 1
    simulate}

button .stop -text stop -command {
    global running
    set running 0
    .run configure -relief raised
    .stop configure -relief sunken
}

button .user1 -text User1 

button .user2 -text User2

button .user3 -text User3

pack append  .buttonbar .quit  left  .run  left  .stop left 
pack append  .buttonbar .user1 left .user2 left  .user3 left

# Status bar
proc mem_avail {} {return "[lindex [exec vmstat] 24]KB"}

label .statusbar -text "Not Started Yet"
pack append . .statusbar top

# Connections Plot definitions 
if {![info exists .connections.scale]} {set .connections.scale 4}

proc init_connect {} \
{
global .connections.scale 
toplevel .connections -background aquamarine3

# Controls
frame .connections.buttonbar 
pack append .connections .connections.buttonbar top

button .connections.zoomi -text "Zoom in" -command {
    set .connections.scale [expr [set .connections.scale]*2]
    connect_plot
}

button .connections.zoomo -text "Zoom out" -command {
    set .connections.scale [expr [set .connections.scale]*.5]
    connect_plot
}

pack append .connections.buttonbar \
	.connections.zoomi left  .connections.zoomo left 


# Used for any of the BLT graph widgets
#proc TurnOnHairs { graph } {
#    bind $graph <Any-Motion> {%W crosshairs configure -position @%x,%y}
#}
#proc TurnOffHairs { graph } {
#    bind $graph <Any-Motion> {%W crosshairs configure -position @%x,%y}
#}


# Canvas widget
canvas .connections.graph -height 300  -width 300 -background aquamarine3 \
  -xscrollincrement 1 -yscrollincrement 1
pack append .connections .connections.graph top

# left mouse drags, right mouse zooms
bind .connections.graph <ButtonPress-1> {
    set oldx %x
    set oldy %y 
}
bind .connections.graph <Button1-Motion> {
    .connections.graph xview scroll [expr $oldx-%x] units
    .connections.graph yview scroll [expr $oldy-%y] units
    set oldx %x
    set oldy %y 
}
bind .connections.graph <ButtonPress-3> {
    set .connections.scale [expr [set .connections.scale]*2]
    .connections.graph xview scroll \
	    [expr round(2*[.connections.graph canvasx %x] \
	    - [.connections.graph canvasx \
	        [expr [.connections.graph cget -width]/2]\
	      ])\
            ] units
    .connections.graph yview scroll \
	    [expr round(2*[.connections.graph canvasy %y] \
	    - [.connections.graph canvasy \
	        [expr [.connections.graph cget -height]/2]\
	      ])\
            ] units
    connect_plot
}
bind .connections.graph <Configure> connect_plot

}

# called before plotting data points
proc update_connect {} \
{
# resize
.connections.graph configure \
  -height [expr [winfo height .connections ]-32] \
  -width [expr [winfo width .connections]-2]
.connections.graph delete all
}

# display widget definitions

proc  init_display {cell cell_idx} \
{
global display${cell}_time
graph .display$cell.graph -title "Species Density - Cell ($cell_idx)" 
vector display${cell}_time
.display$cell.graph xaxis configure -title "time"
.display$cell.graph yaxis configure -title ""
pack append .display$cell .display$cell.graph top
}

proc print_display {{filename display.ps} {cell 0}} \
	{.display$cell.graph postscript output $filename} 

# plot widget definitions

proc init_plot {name title xdata} {
    graph .$name.graph -title $title 
    pack append .$name .$name.graph top
#    bind .$name.graph <B1-ButtonRelease> { %W crosshairs toggle }
#    bind .$name.graph <Enter> { TurnOnHairs %W }
#    bind .$name.graph <Leave> { TurnOffHairs %W }
    Blt_ZoomStack .$name.graph
    Blt_Crosshairs .$name.graph
    Blt_ActiveLegend .$name.graph
    Blt_ClosestPoint .$name.graph
    .$name.graph xaxis configure -title $xdata
    global $name$xdata .$name.lastx
    vector $name$xdata
    set .$name.lastx 0
}

proc create_element {plotname xdata name colour} {
    global $plotname$name $plotname$xdata
    vector $plotname$name
    .$plotname.graph element create .$name -xdata $plotname$xdata \
	    -ydata $plotname$name -color $colour -pixels 0
}

proc plot {name args} {
    global .$name.exists

# grab out title argument and remove title from args list
    if {[lindex $args 0] == "-title"} {
	set title [lindex $args 1]
	set args [lreplace $args 0 1]
    } else {
	set title $name
    }

# declare all args variable names as global
    for {set i 0} {$i<[llength $args]} {incr i} {global [lindex $args $i]}
    global palette .$name.lastx

    if {! [info exists .$name.exists]} {
	newwin .$name
	set .$name.exists 1
	init_plot $name $title [lindex $args 0]
	if {[info exists palette]} {
	    for {set i 1} {$i<[llength $args]} {incr i} {
		create_element $name [lindex $args 0] [lindex $args $i] \
			[lindex $palette [expr $i%[llength $palette]]]
	    } 
	} else {
	    for {set i 1} {$i<[llength $args]} {incr i} {
		create_element $name [lindex $args 0] [lindex $args $i] black
	    }
	}
    }
    
# don't add unnecessary data
    if {[set .$name.lastx] == [set [lindex $args 0]]} return
    set .$name.lastx [set [lindex $args 0]]

    for {set i 0} {$i<[llength $args]} {incr i} {
	$name[lindex $args $i] append [set [lindex $args $i]]
    }
}

# winnow out excess elements (remove 9 out of 10) from a plot.
# This doesn't work too well - lets give up!!
proc winnow_display {cell} {
    global .display$cell.time
    set elements [.display$cell.graph element names]
    set size [.display$cell.time length]
    set time .display$cell.time
    set incr [expr ([set [set time](end)] - [set [set time](0)])/10]
    puts stdout $incr
    for {set i 0; set cntr 0} {$i < $size} {incr i} {
	if {[set [set time]($cntr)] > $cntr * $incr } then {
	    incr cntr
	} else {
	    $time delete $cntr
	    for {set j 0} {$j < [llength elements]} {incr j} {
		[lindex $elements $j] delete $cntr
		.display$cell.graph element configure [lindex $elements $j] \
			-xdata $time -ydata [lindex $elements $j]
	    }
	}
    }
}

# histogram plot

proc setnbins {name x} {
    upvar #0 .$name.nbins nbins 
    upvar #0 .$name.reread reread
    set nbins [expr int(exp($x/10.0))]
    .$name.nbincontrol configure -label "No. Bins:$nbins"
    set reread 1
}

proc xlogscale {name} {
    global .$name.xlogison .$name.reread .$name.max .$name.min
    if [set .$name.xlogison] {
	.$name.xlogscale configure -relief raised
	set .$name.xlogison 0
	set .$name.reread 1
#	.$name.graph xaxis configure -logscale false
	.$name.graph xaxis configure -title "X"
    } else {
	if { [set .$name.max] < 0 || [set .$name.min] < 0} {
	    return -code error "negative x scale"}
	.$name.xlogscale configure -relief sunken
	set .$name.xlogison 1
	set .$name.reread 1
#	.$name.graph xaxis configure -logscale true
	.$name.graph xaxis configure -title "log_10 X"
    }
}

proc ylogscale {name} {
    if [.$name.graph yaxis cget -logscale] {
	.$name.ylogscale configure -relief raised
	.$name.graph yaxis configure -logscale false
    } else {
	.$name.ylogscale configure -relief sunken
	.$name.graph yaxis configure -logscale true
    }
}

# Used to format the x tick labels
proc fmttick {name val} {
return [format "%6.1f" $val]
}

proc histogram {name args} {
    # assign class binding
    upvar #0 .$name.data data 
    upvar #0 .$name.nbins nbins 
    upvar #0 .$name.max max 
    upvar #0 .$name.min min
    upvar #0 ${name}_x x
    upvar #0 ${name}_y y
    upvar #0 .$name.reread reread
    global .$name.exists ${name}_x ${name}_y .$name.xlogison

    # grab out title argument and remove title from args list
    if {[lindex $args 0] == "-title"} {
	set title [lindex $args 1]
	set value [lindex $args 2]
    } else {
	set title $name
	set value [lindex $args 0]
    }

    if {! [info exists .$name.exists]} {
	newwin .$name
	set .$name.exists 1

# log scale controls
	frame .$name.buttonbar 
	pack .$name.buttonbar -fill x -side top -in .$name 
	button .$name.xlogscale -text "x logscale" -command "xlogscale $name"
	button .$name.ylogscale -text "y logscale" -command "ylogscale $name"
	pack append .$name.buttonbar .$name.xlogscale left \
		.$name.ylogscale left
	set .$name.xlogison 0

# barchart widget
	barchart .$name.graph -title $title
	pack append .$name .$name.graph left
#	 bind .$name.graph <B1-ButtonRelease> { %W crosshairs toggle }
#	 bind .$name.graph <Enter> { TurnOnHairs %W }
#	 bind .$name.graph <Leave> { TurnOffHairs %W }
	Blt_ZoomStack .$name.graph
	Blt_Crosshairs .$name.graph
	Blt_ActiveLegend .$name.graph
	Blt_ClosestPoint .$name.graph
	.$name.graph xaxis configure -command fmttick

	set nbins 100
	# scroll widget to control number of bins
	scale .$name.nbincontrol -from 0 -to 100 \
		-length [expr [.$name.graph cget -height]-7] \
		-showvalue false -command "setnbins $name"
#	.$name.nbincontrol configure -label "No. Bins:$nbins"
	.$name.nbincontrol set [expr int(10*log($nbins))]
	pack append .$name .$name.nbincontrol right 

	vector ${name}_x($nbins)
	vector ${name}_y($nbins)
	for {set i 0} {$i < $nbins} {incr i} {set .$name.x($i) $i}
	.$name.graph xaxis configure -stepsize [expr $nbins/5]
	.$name.graph element create data -xdata ${name}_x -ydata ${name}_y

	set max -1E38
	set min 1E38
	set reread 0
	if {[file exists "$name.dat"]} {
	    # reread data values to reset max and min
	    set data [open "$name.dat" "r"]
	    setmaxmin $name
	    close $data
	    set reread 1
	}
	
	set data [open "$name.dat" "a+"]
    }

# value is now potentially a list of values
    for {set j 0} {$j < [llength $value]} {incr j} {
	set v [lindex $value $j]
	puts $data $v
	if {$v > $max || $v < $min} {
	    # reset max & min val and reread file
	    set reread 1
	    if {$v > $max} {set max $v}
	    if {$v < $min} {set min $v}
	}
    }
   
    if {$reread} {
	#  reread file
	set reread 0
	${name}_x length $nbins
	${name}_y length $nbins
	if [set .$name.xlogison] {
	    set delta [expr (log($max)-log($min))/($nbins-.1)]
	    if {$delta==0} {return}
	    .$name.graph configure -barwidth [expr $delta/log(10)]
	    .$name.graph xaxis configure -stepsize \
		    [expr $delta*$nbins/5/log(10)]
	    for {set i 0} {$i < $nbins} {incr i} {
		set ${name}_x($i) \
			[expr ($i*$delta+log($min))/log(10)]
	    }
	} else {
	    set delta [expr ($max-$min)/($nbins-.1)]
	    if {$delta==0} {return}
	    .$name.graph configure -barwidth $delta
	    .$name.graph xaxis configure -stepsize [expr $delta*$nbins/5]
	    for {set i 0} {$i < $nbins} {incr i} {
		set ${name}_x($i) \
			[expr $i*$delta+$min]
	    }
	}
	flush $data
	fillyarray $name $min $delta
    } else {	    
	for {set j 0} {$j < [llength $value]} {incr j} {
	    set v [lindex $value $j]
	    if [set .$name.xlogison] {
		set delta [expr (log($max)-log($min))/($nbins-.1)]
		if {$delta==0} {return}
		set idx [expr int((log($v)-log($min))/$delta)]
	    } else {
		set delta [expr ($max-$min)/($nbins-.1)]
		if {$delta==0} {return}
		set idx [expr int(($v-$min)/$delta)]
	    }
	    if {$idx==$nbins} {incr idx -1}
	    set ${name}_y($idx) [expr [set ${name}_y($idx)]+1]
	}
    }
}

# memory exhausted dialog box
label .mem_exhausted -text "Memory is Exhausted" -height 5 -relief raised
button .mem_exhausted.ok -text OK -command "place forget .mem_exhausted"
place .mem_exhausted.ok -relx .5 -rely .6 
    

if {![info exists more_ecolab]} map_mainwin


