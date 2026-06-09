set density(list) 100
#set nsp 100
set dims(x) 2
set dims(y) 2
set nsp(0,0) 10
set nsp(1,0) 11
set nsp(0,1) 12
set nsp(1,1) 13

set repro_rate(random) true
set repro_rate(random,seed) 11
set repro_rate(random,maxval) 1e-2
set repro_rate(random,minval) -5e-3
set interaction(diag,random) true
set interaction(diag,random,seed) 9
set interaction(diag,random,minval) -1e-4
set interaction(diag,random,maxval) -4e-5
set interaction(offdiag,random) true
set interaction(offdiag,random,minval) -1e-4
set interaction(offdiag,random,maxval) 1e-4
set interaction(offdiag,random,connectivity) 2
set interaction(offdiag,random,seed) 12
set mutation(random) true
set mutation(random,seed) 12
set mutation(random,maxval) 1e-3
set migration(north_south,list) .01
set migration(east_west,list) .01

set sp_sep .1 
# 0.1 seems to be sufficiently small here







