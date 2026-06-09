#!ecolab.exe

set density(list) 100
set nsp 2
set repro_rate(list) {.1 -.1}
set interaction(diag,list) {-.0001 0}
set interaction(offdiag,val) {-0.001 0.001}
set interaction(offdiag,row) {0 1}
set interaction(offdiag,col) {1 0}
set palette {black red}

proc send_density {channel client port} {
    puts stdout "$channel [get tstep] [get density]"
    puts $channel "[get tstep] [get density]"
    close $channel
}

data_server 7000
socket -server send_density 7001

while 1 { 
    generate
    if {[get tstep] % 1000 == 0} {puts stdout [get density]}
}

