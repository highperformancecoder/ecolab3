# Alter these variables
DEBUGGING=1
MEMDEBUG=
TK36=
BLT=1
LAPACK=
MPI=
CM5=
GCOV=
ZLIB=

# set this variable to force use of gcc
GCC=1


#FLAGS+=-DTIMECMDS
CC=gcc
CPLUSPLUS=g++
LINK=g++

# The following section uses GNU Make specific syntax. If not using
# GNU Make, edit the  FLAGS, LIBS, CC,
# CPLUSPLUS, LINK and ARRAYS string.
FLAGS+=-I/home/rks/usr/include -L/home/rks/usr/lib


HOST=$(shell hostname)
OS=$(shell uname)

# Turn on CM5 automatically
ifeq ($(HOST),nswcpc)
CM5=1
MPI=1
endif

ifdef DEBUGGING
FLAGS+=-w -g
else
FLAGS+= -w -O -DNDEBUG
endif

ifdef MEMDEBUG
FLAGS+=-DMEMDEBUG
endif

ifdef GCOV
FLAGS+=-fprofile-arcs -ftest-coverage
endif

ifdef TK36
FLAGS+= -DTK3
endif

ifdef BLT
FLAGS+= -DBLT 
LIBS+= -lBLT
endif

ifdef LAPACK
FLAGS+= -DLAPACK

ifneq ($(HOST),napier.pvl.edu.au)
LIBS+= lapack.a
else
LIBS+= -lcomplib.sgimath
endif

endif

ifeq ($(OS),IRIX64)
FLAGS+=-L/usr/local/mpi/lib32 -L/usr/lib32 -L/usr/local/lib32 
endif

ifdef MPI
FLAGS+=-DMPI -I/usr/local/mpi/include -L/usr/local/mpi/lib
LIBS+=-lmpi
endif

ifdef ZLIB
FLAGS+=-DZLIB
LIBS+=-lz
endif

ifdef CM5
ARRAYS=cs_arrays.o
else
ifndef GCOV
FLAGS+= -DCONTIGUOUS
endif
ARRAYS=c_arrays.o
endif

ifneq ($(HOST),nswcpc)
ifeq ($(OS),SunOS) 
LIBS+= -lsocket -lnsl -ldl
endif
endif

ifeq ($(OS),Linux)
LIBS+=-ldl
endif

ifndef GCC
ifeq ($(OS),IRIX64)
CC=cc
CPLUSPLUS=CC
LINK=CC
FLAGS+=-n32 -I/usr/local/include
endif
endif

ifdef CM5
LINK=cs -Zcmld -g++
endif

# End of GNU Make dependent section
# default flags go here
FLAGS+=-L/usr/lib -L/usr/local/lib
LIBS+= -L/usr/local/lib -L/usr/X11/lib -ltk -ltcl -lX11  -lm -lc
#LIBS+= -L/usr/X11/lib -ltk8.0 -ltcl8.0 -lX11  -lm -lc
#ARRAYS=c_arrays.o

OBJS=tclmain.o init_arrays.o analysis.o globals.o clientserver.o checkpoint.o strings.o aux.o arrays.o $(ARRAYS) 

# files to go into distribution
DIST=Makefile tclmain.cc init_arrays.cc analysis.cc globals.cc clientserver.cc\
 checkpoint.cc aux.cc arrays.cc ecolab.cc newman.cc testarrays.cc distrand.c \
 strings.c c_arrays.c \
 cs_arrays.cs wrap.c Xecolab.tcl console.tcl console2.tcl ecolab.tcl \
 engine.tcl model.tcl newman.tcl pred-prey.tcl BitSet.h analysis.h  arrays.h \
 c_arrays.h cs_arrays_defs.h ecolab.h globals.h maxmin.h newarrays.h tcl++.h \
 distrand.h Realloc.h shadow.cc bedau.tcl

MODELS=ecolab.exe newman.exe shadow.exe

.SUFFIXES: .cc .c .o .cs .h .d .exe

.cc.o: 
	$(CPLUSPLUS) -c $(FLAGS)  $<

.c.o:
	$(CC) -c $(FLAGS) $<

.cs.o:
	cs -c $(FLAGS) $<

.o.exe: 
	$(LINK) $(FLAGS) $*.o $(OBJS) $(LIBS) -o $@

ifdef AEGIS
aegis-all: clean
	$(MAKE) all
endif


all: depend $(MODELS) 
$(MODELS): $(OBJS) 

#Include dependencies
.c.d:
	gcc -w -MM $< >$@

.cc.d:
	gcc -w -MM $< >$@

depend: $(OBJS:.o=.d) $(MODELS:.exe=.d) testarrays.d
	cat *.d >depend

include depend

#C* kernel
cs_arrays.o: cs_arrays.cs cs_arrays_defs.exh
	cs -c $(FLAGS) cs_arrays.cs -o cs_arrays.o

cs_arrays_defs.exh: cs_arrays_defs.h wrap
	$(CC) -E cs_arrays_defs.h|wrap >cs_arrays_defs.exh

wrap: wrap.c
	$(CC) -o wrap wrap.c

newarrays.exh: newarrays.h wrap
	gcc -E -P $(CONT_FLAG) newarrays.h |wrap >newarrays.exh

arrays.h: newarrays.exh
	touch arrays.h


#Test Utilities
testarrays: testarrays.o c_arrays.o
	$(LINK) $(FLAGS) testarrays.o c_arrays.o -lm -o testarrays

clean:
	rm -f *.d depend *.o $(MODELS) testarrays wrap *~ \#*\# core *.exh 

doc/ecolab/ecolab.html: doc/*.tex
	(cd doc; ./Makedoc)

distrib: doc/ecolab/ecolab.html
	rm -rf ecolab/*	        
	cp -r doc $(DIST) ecolab
