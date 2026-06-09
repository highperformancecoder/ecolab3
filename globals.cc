/* definitions for the global class */

#include "tcl++.h"
#include "arrays.h"
#include "globals.h"

extern "C"
{
#include <rpc/types.h>
#include <rpc/xdr.h>
}

iarray proc_dims;
int ngcells;

glue::glue(int sz)
{
  size=0; asize=0;
  data=Realloc(NULL,sz); 
  input = new XDR; output=NULL;
  xdrmem_create(input,(const caddr_t)data,sz,XDR_ENCODE);
}

glue::~glue()
{
  xdr_destroy(input);
  delete input;
  if (output!=NULL)
    {
      xdr_destroy(output);
      delete output;
    }
  Realloc(data,0);
}

void glue::operator<<(int x)
{
  if (BUFCHUNK-xdr_getpos(input)<4)
    {
      asize += xdr_getpos(input);
      data = Realloc(data,asize+BUFCHUNK);
      xdr_destroy(input);
      xdrmem_create(input,(const caddr_t)data+asize,BUFCHUNK,XDR_ENCODE);
    }
  if (!xdr_int(input,&x)) error("Error encoding XDR stream");
  size = asize+xdr_getpos(input);
}

void glue::operator<<(long x)
{
  if (BUFCHUNK-xdr_getpos(input)<sizeof(x))
    {
      asize += xdr_getpos(input);
      data = Realloc(data,asize+BUFCHUNK);
      xdr_destroy(input);
      xdrmem_create(input,(const caddr_t)data+asize,BUFCHUNK,XDR_ENCODE);
    }
  if (!xdr_long(input,&x)) error("Error encoding XDR stream");
  size = asize+xdr_getpos(input);
}

#ifdef _GNUC__
void glue::operator<<(long long int x)
{
  if (BUFCHUNK-xdr_getpos(input)<sizeof(x))
    {
      asize += xdr_getpos(input);
      data = Realloc(data,asize+BUFCHUNK);
      xdr_destroy(input);
      xdrmem_create(input,(const caddr_t)data+asize,BUFCHUNK,XDR_ENCODE);
    }
  if (!xdr_int(input,(int*)&x)) error("Error encoding XDR stream");
  if (sizeof(x)!=2*sizeof(int) || !xdr_int(input,((int*)&x)+1)) 
    error("Error encoding XDR stream");
  size = asize+xdr_getpos(input);
}
#endif

void glue::operator<<(double x)
{
  if (BUFCHUNK-xdr_getpos(input)<8)
    {
      asize += xdr_getpos(input);
      data = Realloc(data,asize+BUFCHUNK);
      xdr_destroy(input);
      xdrmem_create(input,(const caddr_t)data+asize,BUFCHUNK,XDR_ENCODE);
    }
  if (!xdr_double(input,&x)) error("Error encoding XDR stream");
  size = asize+xdr_getpos(input);
}

void glue::operator<<(iarray x)
{
  *this<<x.size;
  for (int i=0; i<x.size; i++) *this<<x[i];
}

void glue::operator<<(array x)
{
  *this<<x.size;
  for (int i=0; i<x.size; i++) *this<<x[i];
}

void glue::operator<<(sparse_mat x)
{
  *this<<x.diag;
  *this<<x.row;
  *this<<x.col;
  *this<<x.val;
}

void glue::operator>>(int& r)
{
  if (output==NULL)
    {
      output = new XDR;
      xdrmem_create(output,(const caddr_t)data,size,XDR_DECODE);
    }
  if (!xdr_int(output,&r)) error("error decoding XDR stream");
}

void glue::operator>>(long& r)
{
  if (output==NULL)
    {
      output = new XDR;
      xdrmem_create(output,(const caddr_t)data,size,XDR_DECODE);
    }
  if (!xdr_long(output,&r)) error("error decoding XDR stream");
}

#ifdef _GNUC__
void glue::operator>>(long long int& r)
{
  if (output==NULL)
    {
      output = new XDR;
      xdrmem_create(output,(const caddr_t)data,size,XDR_DECODE);
    }
  if (!xdr_int(output,&r)) error("error decoding XDR stream");
  if (sizeof(r)!=2*sizeof(int) || !xdr_int(output,((int*)&r)+1)) 
    error("Error encoding XDR stream");
}
#endif

void glue::operator>>(double& r)
{
  if (output==NULL)
    {
      output = new XDR;
      xdrmem_create(output,(const caddr_t)data,size,XDR_DECODE);
    }
  if (!xdr_double(output,&r)) error("error decoding XDR stream");
}


void glue::operator>>(iarray& r)
{
  int s;
  *this>>s;
  r=iarray(s);
  for (int i=0; i<r.size; i++) *this>>r[i];
}

void glue::operator>>(array& r)
{
  int s;
  *this>>s;
  r=array(s);
  for (int i=0; i<r.size; i++) *this>>r[i];
}

void glue::operator>>(sparse_mat& r)
{
  *this>>r.diag;
  *this>>r.row;
  *this>>r.col;
  *this>>r.val;
}

global::global(const global& x) 
{
  int i;
  ocell=x.ocell; ncells=x.ncells; tstep=x.tstep;
  dims=x.dims; nsp=x.nsp;

  if (x.coords==NULL) coords=NULL;
  else
    {
      coords=new iarray[dims.size+1];
      for (i=0; i<dims.size+1; i++)
	coords[i]=x.coords[i];
    }
  
  
  iarrays=new iarray[niarr=x.niarr]; 
  for (i=0; i<niarr; i++) iarrays[i]=x.iarrays[i];
  
  arrays=new array[narr=x.narr];
  for (i=0; i<narr; i++) arrays[i]=x.arrays[i];
  
  sparse_mats=new sparse_mat[nspars=x.nspars];
  for (i=0; i<nspars; i++) sparse_mats[i]=x.sparse_mats[i];
}

global global::operator=(const global& x)
{
  int i;
  ocell=x.ocell; ncells=x.ncells; tstep=x.tstep;
  dims=x.dims; nsp=x.nsp;
  
  delete [] coords;
  if (x.coords==NULL) coords=NULL;
  else
    {
      coords=new iarray[dims.size+1];
      for (i=0; i<dims.size+1; i++)
	coords[i]=x.coords[i];
    }
  
  if (niarr!=x.niarr)
    {
      delete [] iarrays;
      if (niarr=x.niarr)
	iarrays=new iarray[x.niarr]; 
      else
	iarrays=NULL;
    }
  for (i=0; i<niarr; i++) iarrays[i]=x.iarrays[i];
  
  if (narr!=x.narr)
    {
      delete [] arrays;
      if (narr=x.narr)
	arrays=new array[x.narr]; 
      else
	arrays=NULL;
    }
  for (i=0; i<narr; i++) arrays[i]=x.arrays[i];
  
  if (nspars!=x.nspars)
    {
      delete [] sparse_mats;
      if (nspars=x.nspars)
	sparse_mats=new sparse_mat[x.nspars]; 
      else
	sparse_mats=NULL;
    }
  for (i=0; i<nspars; i++) sparse_mats[i]=x.sparse_mats[i];
  return (*this);
}


/// return just the global variables belonging to a range of cell 
global global::get_cell_vars(int cell, int nc) 
{
  global r;
  iarray offs, slice, mask;
  int i;
  
  r.ocell=cell+ocell;
  r.ncells=nc;
  r.tstep=tstep;

  r.dims=dims;     /* dimensionality of model is constant */
  r.nsp=nsp[cell+pcoord(nc)];

  r.coords=new iarray[dims.size+1];
  slice=pcoord(nc)+cell;
  for (i=0; i<dims.size+1; i++)
    r.coords[i]=coords[i][slice];
      
  offs=enumerate(nsp);
  slice=pcoord(sum(r.nsp)) + offs[cell];

  r.niarr=niarr; r.iarrays = new iarray[niarr]; 
  for (i=0; i<niarr; i++) r.iarrays[i]=iarrays[i][slice];

  r.narr=narr; r.arrays = new array[narr];
  for (i=0; i<narr; i++) r.arrays[i]=arrays[i][slice];

  r.nspars=nspars; r.sparse_mats = new sparse_mat[nspars];
  for (i=0; i<nspars; i++)
    r.sparse_mats[i] = 
      sparse_mats[i].submat(offs[cell],slice.size + offs[cell]);
  return r;
}

///replace cells variables by those contained in vars
void global::put_cell_vars(global vars)
{
  int i, cell;
  iarray mask, slice1, slice2, offs;

  if (dims.size!=vars.dims.size) dims=vars.dims;
  if (coords==NULL) coords=new iarray[dims.size+1];
  if (iarrays==NULL) 
    {
      iarrays=new iarray[vars.niarr];
      niarr=vars.niarr;
    }
  if (arrays==NULL) 
    {
      arrays=new array[vars.narr];
      narr=vars.narr;
    }
  if (sparse_mats==NULL) 
    {
      sparse_mats=new sparse_mat[vars.nspars];
      nspars=vars.nspars;
    }

  /* expand nsp if vars out of current bounds */
  assert(ocell<=vars.ocell);
  if (vars.ocell+vars.ncells > ncells+ocell)
    {
      int s;
      if (ncells>0)
	s=vars.ocell+vars.ncells-ncells-ocell;
      else /* current global is empty!! */
	{
	  s=vars.ncells;
	  ocell=vars.ocell;
	}
      iarray t(s);
      nsp <<= (t=0);
      ncells = nsp.size;
      /* set extra coords to undefined */
      for (i=0; i<dims.size+1; i++) 
	coords[i]<<=(t=-1); 
   }
    
 
  for (i=0; i<dims.size+1; i++) 
    coords[i][pcoord(vars.ncells)+vars.ocell-ocell]=vars.coords[i];

  offs=enumerate(nsp) << sum(nsp);
  
  slice1=pcoord(offs[vars.ocell-ocell]);
  slice2=pcoord( offs[ncells] - offs[vars.ocell+vars.ncells-ocell]) + 
    offs[vars.ocell+vars.ncells-ocell];
      
  assert(niarr==vars.niarr);
  for (i=0; i<niarr; i++) 
    iarrays[i]=iarrays[i][slice1] << vars.iarrays[i] << iarrays[i][slice2];
  
  assert(narr==vars.narr);
  for (i=0; i<narr; i++) 
    arrays[i]=arrays[i][slice1] << vars.arrays[i] << arrays[i][slice2];

  assert(nspars==vars.nspars);
  for (i=0; i<nspars; i++) 
    sparse_mats[i].insert(vars.sparse_mats[i], offs[vars.ocell-ocell],
	  offs[vars.ocell+vars.ncells-ocell]-offs[vars.ocell-ocell]);
  
  assert(sum(vars.nsp)==vars.iarrays[0].size);
  slice1=pcoord(vars.ocell-ocell);
  slice2=pcoord( ncells - vars.ocell - vars.ncells + ocell) + 
    vars.ocell+vars.ncells-ocell;
  nsp = nsp[slice1] << vars.nsp << nsp[slice2];
}

void global::packup(glue& buffer)
{
  int i;
  buffer<<ocell;
  buffer<<ncells;
  buffer<<tstep;
  buffer<<niarr;
  buffer<<narr;
  buffer<<nspars;
  buffer<<dims;
  buffer<<nsp;
  for (i=0; i<dims.size+1; i++) buffer << coords[i];
  for (i=0; i<niarr; i++) buffer<<iarrays[i];
  for (i=0; i<narr; i++)  buffer<<arrays[i];
  for (i=0; i<nspars; i++) buffer<<sparse_mats[i];
}


void global::unpack(glue& buffer)
{
  int i, itmp;
  buffer>>ocell;
  buffer>>ncells;
  buffer>>tstep;

  buffer>>itmp;
  if (niarr>0)
    assert(niarr==itmp);
  else
    {
      niarr=itmp;
      iarrays = new iarray[niarr];
    }

  buffer>>itmp;
  if (narr>0)
    assert(narr==itmp);
  else
    {
      narr=itmp;
      arrays = new array[narr];
    }

  buffer>>itmp;
  if (nspars>0)
    assert(nspars==itmp);
  else
    {
      nspars=itmp;
      sparse_mats = new sparse_mat[niarr];
    }

  buffer>>dims;
  buffer>>nsp;

  delete [] coords; coords=new iarray[dims.size+1];
  for (i=0; i<dims.size+1; i++) buffer>>coords[i];

  for (i=0; i<niarr; i++)
    buffer>>iarrays[i];

  for (i=0; i<narr; i++)
    buffer>>arrays[i];
  
  for (i=0; i<nspars; i++)
    buffer>>sparse_mats[i];
}

/* initialize models vars to have one component, all zero. This is
   meant for client type ecolab implementations, where the model will
   be read in later */

NEWCMD(dummy_model,0)
{
  dims=iarray(0);
  nsp=iarray(ncells=1);
  nsp[0]=1;
  int i;
  for (i=0; i<global_vars.niarr; i++) global_vars.iarrays[i]<<=0;
  for (i=0; i<global_vars.narr; i++) global_vars.arrays[i]<<=0;
  for (i=0; i<global_vars.nspars; i++) global_vars.sparse_mats[i]=sparse_mat(1,0);
}

#if MPI

/* this routine matches distribute_data in init_arrays.cc */

NEWCMD(take_it,1)
{
  PARALLEL
  int msgsz;
  MPI_Status status;

  if (myid==atoi(argv[1]))
    {
      MPI_Probe(0, TAG_PUSH, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_CHAR, &msgsz);
      glue buffer(msgsz);
      MPI_Recv(buffer.data,msgsz,MPI_CHAR,0,TAG_PUSH,MPI_COMM_WORLD,&status);
      buffer.size=msgsz;
      global_vars.unpack(buffer);

      /* receive ngcells and proc_dims */
      MPI_Recv(&ngcells,1,MPI_INT,0,TAG_PUSH,MPI_COMM_WORLD,&status);
      MPI_Probe(0, TAG_PUSH, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_INT, &msgsz);
      proc_dims=iarray(msgsz);
      int *t=proc_dims;
      MPI_Recv(t,msgsz,MPI_INT,0,TAG_PUSH,MPI_COMM_WORLD,&status);
      proc_dims=t;
    }
}

NEWCMD(give_me,1)
{
  PARALLEL;
  glue buffer;
  int msgsz, all=strcmp(argv[1],"all")==0;
  if (all && myid>0 || !all && myid==atoi(argv[1]))
    {
      global_vars.packup(buffer);
      MPI_Send(buffer.data, buffer.size, MPI_CHAR, 0, TAG_PUSH,MPI_COMM_WORLD);
    }
}

void get_all_globals(global& all)
{
  MPI_Status status;
  global t;
  int msgsz;
  tclcmd cmd;
  all=global_vars;   /* get processor 0's variables */
  cmd << "give_me all\n";
  for (int cnt=nprocs-1; cnt; cnt--)
    {
      MPI_Probe(MPI_ANY_SOURCE, TAG_PUSH, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_CHAR, &msgsz);
      glue buffer(msgsz);
      buffer.size=msgsz;
      MPI_Recv(buffer.data, msgsz, MPI_CHAR, MPI_ANY_SOURCE, TAG_PUSH,
	       MPI_COMM_WORLD, &status);
      t.unpack(buffer);
      all.put_cell_vars(t);
    }
}
  
#endif



