
/* Arrays.
This file contains the code for the array classes
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "BitSet.h"
#include <strstream>
#include "arrays.h"
#include "tcl++.h"
#include "globals.h"
#include <math.h>

/* because of bug in g++ 2.7.0, we must declare strchr and strstr in C
and write a wrapper */

extern "C"  char* strstr_(const char*,char*);
extern int ngcells;  /* total number of cells in system = sum_p(ncells(p)) 
			where p=0..nprocs */

/* list abbreviations are defined here, is_int is a flag that is 1 if list 
is an integer array, and 0 if it is a double array */

static void assign_list(const char* elementi, array& data)
{int repeat;
 double start, end, incr;
 char *liststr = new char[strlen(elementi)];
 const char **element;
 int elemc;

 if (sscanf(elementi,"%d*%s",&repeat,liststr)==2) /* Handle repeat counts */
      if (Tcl_SplitList(interp,liststr,&elemc,&element)==TCL_OK)
	{ int j,i;
	  for (j=0; j<repeat; j++)
	    for (i=0; i<elemc; assign_list(element[i++],data));    
          Tcl_Free((char*)element);
	}


 switch (sscanf(elementi,"%lg:%lg:%lg",&start,&end,&incr)) /* do range case */
   {
      case 1: end = start;   /* only a single value to assign */
      case 2: incr = 1;      /* a range to assign, default increment */
      case 3: 
	   for (double val=start; val<=end; val+=incr)
	       data <<= val;
    }
 delete [] liststr;
}



/* array initialize routine. */

int initialize(array& data, char *tclname)
{
  tclvar arr(tclname);
  double minval,maxval;

  
/* Random declaration.
random = whether the array is to be filled with random values
(boolean, default={\em false})
Random takes the following sub parameters:\newline
random,minval = minimum value of the range of random numbers (default=0) 
random,maxval = maximum value of the range of random numbers (default=1) 
random,seed   = seed used for random number generator (default={\em random}) 
*/

  if (exists(arr["random"]) && (int) arr["random"])
    { /* fill the array with random values */
      maxval = exists(arr["random,maxval"])? 
	(double) arr["random,maxval"]: 1.0;
      minval = exists(arr["random,minval"])? 
	(double) arr["random,minval"]: 0.0;
      if (exists(arr["random,seed"]))
	cs_srand((unsigned int) arr["random,seed"]);
      else
	cs_srand((unsigned int) clock());
      fillrand(data);
      data = data * (maxval-minval) + minval;
   }
  

/* list declaration.
list = list of values to initialize array 
The list can either be a single value, in which case every element of 
the array is initialized to that value, or it contains exactly enough
elements to fill the array. Certain abbreviations can be used to
reduce the storage required by the list:
\begin{itemize}
\item[cnt * sublist] Repeat sublist cnt times, where sublist could be
a single value, or a complete list.
\item[low : high : step] Specify a range of values. The step parameter
is optional.
\end{itemize}
Ecolab will delete the list once the array has been initialised.
*/

  else if (exists(arr["list"]))
    {int elemc,i;
     const char **element;
     if (Tcl_SplitList(interp,(const char*)arr["list"],&elemc,&element)==TCL_OK)
	{
         if (elemc==1 && strstr_(element[0],"*:")==NULL)
	   { /* single scalar value to be spread over array */
	    double val;
	    sscanf(element[0],"%lg",&val);
	    data=val;
	   }
	 else 
	   {
	     data = array(0);
	     for (i=0; i<elemc; assign_list(element[i++],data));
	   }
	 Tcl_Free((char*)element);
        }
    }
  else
    return 0;
  return 1;  /* 1 = successful initialisation */

}

int initialize(iarray& data, char *tclname)
{
  array tmp(data.size);
  int r=initialize(tmp,tclname);
  data=tmp;
  return r;
}

int initialize_offdiag(sparse_mat& interact, char *nm, int size)
{
  tclvar tcl_offd(nm);
  int i;  
  /* offdiag,random keyword  */
  
  if (exists(tcl_offd["random"]) && (int) tcl_offd["random"])
    { /* fill the array with random values */
      double maxval,minval;
      int connectivity, seed;
      
      maxval = exists(tcl_offd["random,maxval"])? 
	(double) tcl_offd["random,maxval"]: 1.0;
      minval = exists(tcl_offd["random,minval"])? 
	(double) tcl_offd["random,minval"]: 0.0;
      connectivity =  (exists(tcl_offd["random,connectivity"])? 
	 (int)tcl_offd["random,connectivity"]: 2);
      /* clamp connectivity to < size */
      connectivity = connectivity<size? connectivity: size-1;
      
      if (exists(tcl_offd["random,seed"]))
	cs_srand(tcl_offd["random,seed"]);
      else
	cs_srand((unsigned int) clock());

      interact.val = array(size*connectivity);
      fillrand(interact.val);
      interact.val = interact.val * (maxval-minval) + minval;
      
      interact.row=interact.col=iarray(0);

      /* fill row and col such that each pair of integers is unique */
      BitSet prev_num;
      int r, csize;
      csize=connectivity*size;
      iarray rslice(csize), cslice;

      /* fill prev_num with the diagonal values to prevent
	 diagonals filling in */
      for (i=0; i<size; i++)
	prev_num.set(i+i*size);

      for (i=0; i<csize; i++)
	{
	  while (prev_num.test(
			       r=(int)(size*size*(float)rand()/RAND_MAX)
			       ));
	  prev_num.set(r);
	  rslice[i]=r;
	}

      interact.col = (cslice = rslice / size) ;
      interact.row = rslice - cslice * size ;
    }
  
  /*  offdiag,list declaration. */
  
  /* we must have a value, row and column list */
  if ((exists(tcl_offd["val"]) + exists(tcl_offd["row"]) + 
       exists(tcl_offd["col"])) % 3!=0 )
    return 0;
    //    error("offdiag,{val,row,col} incompletely specified");
      
   if (exists(tcl_offd["val"]))
    {int elemc;
     const char **element;
     if (Tcl_SplitList(interp,(const char*)tcl_offd["val"],&elemc,&element)==TCL_OK)
       {
	 for (i=0; i<elemc; i++)
	     assign_list(element[i],interact.val);
	 Tcl_Free((char*)element);
       }
   }
      
  if (exists(tcl_offd["row"]))
    {int elemc;
     const char **element;
     if (Tcl_SplitList(interp,(const char*)tcl_offd["row"],&elemc,&element)==TCL_OK)
       {
	 array tmp;
	 for (i=0; i<elemc; i++)
	   assign_list(element[i],tmp);
	 interact.row = tmp;
	 Tcl_Free((char*)element);
       }
   }
      
  if (exists(tcl_offd["col"]))
    {int elemc;
     const char **element;
     if (Tcl_SplitList(interp,(const char*)tcl_offd["col"],&elemc,&element)==TCL_OK)
       {
	 array tmp;
	 for (i=0; i<elemc; i++)
	     assign_list(element[i],tmp);
	 interact.col = tmp;
	 Tcl_Free((char*)element);
       }
   }
      
  if (interact.val.size != interact.row.size || 
      interact.val.size != interact.col.size )
    error("Size mismatch between offdiag val, row and col");
  return 1;
}

void factorize(int n, iarray& f, int& nf)
{
  /* put as many primes as you like here (up to sqrt(max nprocs))*/
  int i,prime[]={2,3,5,7,11,13,17,19,23,29,31,37};
  
  assert( sqrt(n) < prime[ sizeof(prime)/sizeof(int)-1 ]);
                                   
  
  for (i=0, nf=0; prime[i] <= sqrt(n); )
    {
      if (n%prime[i]==0)  /* the prime[i] is a factor */
	{
	  f[nf++]=prime[i];
	  n/=prime[i];
	}
      else i++;          /* try next prime */
    }
  
  if (n>1) /* then n must be prime */
    f[nf++]=n;
}

static iarray offset;

void init_scalars()
{
  int i;
  tstep=0;
  tclvar tcl_dims("dims"), tcl_nsp("nsp");

  if (!exists(tcl_nsp) && tcl_nsp.size()==0)  
    /* we have an irrecoverable error */
    //    error("nsp is not defined");
    tcl_nsp=1;
  ocell=0;

  if (tcl_dims.size()>0)  /* multicellular case */
    {
      /* calculate ncells */
      tclindex idx;
      for (dims <<= (int)idx.start(tcl_dims); !idx.last(); 
	   dims <<= (int)idx.incr() );
      ncells=prod(dims);
      
      nsp=iarray(ncells);
      if (tcl_nsp.size()==0) nsp=tcl_nsp; /* broadcast nsp */
      else
	for (int i=0; i<ncells; i++)
	  nsp[i] = tcl_nsp[index_name(i)];
    }
  else /* single cell case */
    {
      dims=iarray(0);
      nsp=iarray(ncells=1);
      nsp[0]=tcl_nsp;
    }

  ngcells=ncells;

  /* set up coord arrays */
  global_vars.coords=new iarray[dims.size+1];
  for (i=0; i<dims.size+1; i++)
    global_vars.coords[i]=iarray(ncells);

  if (ncells==1) return; /* do not need to layout a single cell */

  tclvar tcl_coords("coords");
  if (tcl_coords.size()>0)  /* User specified data layout */
    {
      char n[20];
      for (i=0; i<dims.size; i++) 
	{
	  sprintf(n,"coords(%d)",i);
	  initialize(global_vars.coords[i],n);
	}
    }
  else                      /* perform block layout */
    {
#ifndef USE_MPI
      const int nprocs=1;
#endif
      int i,j,d, tpk, nfactors, sn, tsn;
      iarray nproc_factors((int)ceil(log(nprocs)/log(2)));
      iarray n(dims.size), k(dims.size), tn(dims.size);
      factorize(nprocs,nproc_factors,nfactors);
      
      /* initialize n */
      n[0]=nprocs;
      int bsize=(int)ceil((float)dims[0]/nprocs);
      for (i=1; i<dims.size; i++) 
	{
	  n[i]=1;
	  bsize*=dims[i];
	  sn=nprocs+dims.size-1;
	}

      /* index over factors */
      for (i=1; i<(int)pow(dims.size,nfactors); i++)
	{
	  /* build trial factorization */
	  tn=1;
	  for (j=0, d=1; j<nfactors; j++, d*=dims.size)
	    tn[(i/d)%dims.size] *= nproc_factors[j];
	  
	  /* calculate trial product of ks */
	  for (j=0, tpk=1, tsn=0; j<dims.size; j++)
	    {
	      tpk *= (int)ceil((float)dims[j]/(float)tn[j]);
	      tsn += tn[j];
	    }

	  /* update factorization by trial if trial product is less */
	  /* product of ks is the block size */
	  if (tpk<bsize)
	    {
	      bsize=tpk;
	      sn=tsn;
	      n=tn;
	    }
	  else if (tpk==bsize && tsn < sn) /* secondarily optimise 
					      communication */
	    {
	      sn=tsn;
	      n=tn;
	    }
	}

      /* initialize product arrays */
      iarray pn(dims.size+1), pk(dims.size+1);
      for (i=0, pn[0]=1, pk[0]=1; i<dims.size; i++) 
	{
	  pn[i+1] = pn[i] * n[i];
	  k[i] = (int)ceil((float)dims[i]/(float)n[i]);
	  pk[i+1] = pk[i] * k[i];
	}
      
      /* assign global coordinates */
      int cell=0, proc, oob;
      offset=iarray(nprocs+1);
      for (proc=0; proc<nprocs; proc++)
	{
	  offset[proc]=cell;
	  for (i=0; i<bsize && cell<ncells; i++)
	    {
	      oob=0;  /* flag to indicate if we're out of bounds */
	      for (int d=0; d<dims.size; d++)
		{
		  global_vars.coords[d][cell]= ((proc%pn[d+1])/pn[d])*k[d] +
		    ((i%pk[d+1])/pk[d]);
		  if (global_vars.coords[d][cell]>=dims[d]) oob=1; //set oob
		}
	      if (!oob)  cell++; /* commit coordinates of cell */
	    }
	}
      global_vars.coords[dims.size]=pcoord(ncells);
      assert(cell==ncells);
      offset[nprocs]=ncells;
      proc_dims=n;
    }
}

#if USE_MPI
/* distribute global data out to other processors */
void distribute_data()
{
  if (nprocs==1 || ncells==1) return; /* nothing to be done */
  for (int i=1; i<nprocs; i++)
    {
      tclcmd cmd;
      glue parcel;
      global_vars.get_cell_vars(offset[i],
				offset[i+1]-offset[i]).packup(parcel);
      cmd << "take_it" << i << "\n";
      MPI_Send(parcel.data,parcel.size,MPI_CHAR,i,TAG_PUSH,MPI_COMM_WORLD);
      /* broadcast ngcells and proc_dims */
      MPI_Send(&ngcells,1,MPI_INT,i,TAG_PUSH,MPI_COMM_WORLD);
      MPI_Send((int*)proc_dims,proc_dims.size,MPI_INT,i,TAG_PUSH,
	       MPI_COMM_WORLD);
    }
  /* now extract processor 0's cells */
  global_vars = global_vars.get_cell_vars(0, offset[1] );
}
#endif
