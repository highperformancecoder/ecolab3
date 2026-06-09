#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <strstream.h>
#include "arrays.h"
#include "globals.h"
#include "maxmin.h"
#include "tcl++.h"
#include "analysis.h"
#include "BitSet.h"
#include "Realloc.h"

extern global global_vars;

/* 
global variables for no. of species, and no. of grid points in 
x and y directions.
*/

int &ocell=global_vars.ocell, &ncells=global_vars.ncells;
int &tstep=global_vars.tstep;
iarray &nsp=global_vars.nsp, &dims=global_vars.dims;

void do_checkpoint();





