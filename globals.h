#include <rpc/types.h>
#include <rpc/xdr.h>
#include "Realloc.h"

/* allocate memory for glue items in chunks of this size */
const int BUFCHUNK=1024;  

/* XDR fixes for C++ */
#define xdr_getpos xdr_getpos_
extern "C" int xdr_getpos_(XDR*);
#define xdr_setpos xdr_setpos_
extern "C" int xdr_setpos_(XDR*,int);
#define xdr_destroy xdr_destroy_
extern "C" void xdr_destroy_(XDR*);

///A Class for grouping a whole bunch of binary things together
class glue
{
  XDR *input, *output;
  int asize;
public:
  char *data;
  int size;
  glue(int sz=BUFCHUNK);
  ~glue();
  glue(const glue&){error("no glue copy constructor");}
  void operator<<(int);
  void operator<<(long);
  void operator<<(double);
  void operator<<(iarray);
  void operator<<(array);
  void operator<<(sparse_mat);
  void operator>>(int&);
  void operator>>(long&);
  void operator>>(double&);
  void operator>>(iarray&);
  void operator>>(array&);
  void operator>>(sparse_mat&);
/* These are needed to get the par_addr classes to work properly */
  void operator>>(par_addr_int x) 
    {int y; *this>>y; x=y;}
  void operator>>(par_addr_double x) 
    {double y; *this>>y; x=y;}
  /* rewind output XDR stream */
  void rewind() {if (output!=NULL) xdr_setpos(output,0);}
};

/** These are references to the particular global variables */
class global;
extern global global_vars;
extern int &ocell, &ncells;
extern int &tstep;
extern iarray &nsp, &dims;
extern iarray proc_dims;
extern int ngcells;

///A class for storing the global variables used in the model
class global
{
public:
  ///origin cell and number of cells on this processor 
  int ocell, ncells; 
#if 0
#ifdef _GNUC__
  long long 
#endif
#endif
  int tstep;

  /// dimensions array and number of species in each cell
  iarray dims, nsp, *coords;  

  /// list of iarrays
  iarray *iarrays;

  ///list of arrays
  array *arrays;

  ///list of sparse_mats
  sparse_mat *sparse_mats;

  ///lengths of above lists
  int niarr, narr, nspars;

  global() 
    { 
      ocell=0; ncells=0; tstep=0; coords=NULL;
      niarr=0; iarrays=NULL; 
      narr=0; arrays=NULL; 
      nspars=0; sparse_mats=NULL;
    }

  global(int i, int a, int s)
    {
      ocell=0; ncells=0; tstep=0; coords=NULL;
      niarr=i; iarrays = new iarray[i]; 
      narr=a; arrays = new array[a]; 
      nspars=s; sparse_mats = new sparse_mat[s];
    }

  global(const global&); 

  global operator=(const global&);

  ~global()
    { delete [] coords; delete [] iarrays; 
    delete [] arrays; delete [] sparse_mats;} 

  /* get local equivalents of a global variable */
  iarray& operator[](const iarray& var) 
    {return iarrays[&var-global_vars.iarrays];}
  array& operator[](const array& var) 
    {return arrays[&var-global_vars.arrays];}
  sparse_mat& operator[](const sparse_mat& var) 
    {return sparse_mats[&var-global_vars.sparse_mats];}

  /** return just the global variables belonging to cell, or a number
      of cells cell, cell+1, ... cell+nc-1 */
  global get_cell_vars(int cell, int nc=1);
 
  ///replace cells variables by those contained in vars
  void put_cell_vars(global vars);
  void append(global vars) 
    {
      vars.ocell=ocell+ncells;
      put_cell_vars(vars);
    }

  ///pack up the global data into a contiguous package
  void packup(glue& buffer);
  ///unpack contiguous representation of global data
  void unpack(glue& buffer);
};

char *index_name(int i);
int which_cell(int i);

#ifndef RAND_MAX
#define RAND_MAX  2147483647  /* This is missing from <stdlib.h>? */
#endif


#if USE_MPI
///Assemble a full set of global variables by combining data on each processor
void get_all_globals(global&);
#else
inline void get_all_globals(global& g) {g=global_vars;}
#endif

void init_scalars();
void distribute_data();

