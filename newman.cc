/*
#include "tcl++.h"
#include "arrays.h"
#include "globals.h"
#include "maxmin.h"
#include "math.h"
#include <blt.h>
#include <float.h>
*/
#include "ecolab.h"

void init_newman();
extern "C" {float grand();}

#undef GLOBAL_INIT_HOOK
#define GLOBAL_INIT_HOOK init_newman();

/* definitions of global variables  */
global global_vars(3,1,0);

iarray &density = global_vars.iarrays[0];
iarray &species = global_vars.iarrays[1];
iarray &create = global_vars.iarrays[2];
array  &threshold = global_vars.arrays[0];

extern "C" {
#include "distrand.h"
}

/* functional form of pstress */
double fp(double x)
{
  return 1/(.1+pow(x,1.5));
}

double pstress()
{
  tclvar sigma("sigma");
  //  return fabs(distrand()*(double)sigma);
  return fabs(grand()*(double)sigma);
}

void gspread( array& a, double s )
{
  array gran(a.size);
  fillgrand(gran);
  a += s*gran;
}

NEWCMD(step,0)
{
  tclvar mutation("mutation"), mut1("mut1");
  tclvar imax("interact(random,maxval)"), imin("interact(random,minval)");
  iarray new_sp, odensity;
  array new_thresh, new_intr;
  static int sp_cntr=1;    /* used to label newly created species */
  double stress;

  /* ensure that sp_cntr > species already allocated */
  sp_cntr = max(sp_cntr,max(species)+1);

  odensity=density;

  /* perform extinctions */
  stress = (double)nsp[0]*pstress();
  density = (stress < threshold) * odensity;
  tstep++;
  nsp[0]=sum(density);

  /* get species that mutate */
  new_sp = mutation * (array)odensity;

  /* generate index list of old species that mutate to the new */
  new_sp = gen_index(new_sp); 
  if (new_sp.size==0) return;

  /*
  new_thresh = threshold[new_sp];
  gspread(new_thresh,(double)mut1);
  */
  new_thresh=array(new_sp.size);
  fillrand(new_thresh);

  new_sp = 1;
  density <<= new_sp;
  species <<= sp_cntr + pcoord(new_sp.size);
  new_sp=tstep;
  create <<= new_sp;
  threshold <<= new_thresh;
}

NEWCMD(condense,0)
{
  iarray mask;
  mask = density != 0;
  int mask_true=sum(mask);
  if (density.size==mask_true) return; /* no change ! */
  density = pack(density, mask,mask_true);
  species = pack(species, mask,mask_true);
  create = pack(create, mask,mask_true);
  threshold = pack(threshold, mask,mask_true);
  nsp[0]=density.size;
}

void init_newman()
{
  if (density.size==0)  
    {
      init_scalars();
      density = iarray(sum(nsp));
      density = 1;
      species = pcoord(density.size);
      create=iarray(density.size); create=0;
      threshold = array(density.size);
      initialize(threshold,"threshold");
      nsp=iarray(1);
      nsp[0]=density.size;
      ncells=1;

      //      init_distrand(fp,100,3,0,12,0);
    }
}


/* return the value of a specific variable as a TCL list */
NEWCMD(get,1)
{
  tclreturn result;
  if (strcmp(argv[1],"nsp")==0) result << nsp;
  if (strcmp(argv[1],"tstep")==0) result << tstep;
  if (strcmp(argv[1],"density")==0) result << density;
  if (strcmp(argv[1],"species")==0) result << species;
  if (strcmp(argv[1],"create")==0) result << create;
  if (strcmp(argv[1],"threshold")==0) result << threshold;
}
  
NEWCMD(lifetimes,0) 
{ 
  tclreturn lifetimes; 

  /* assume condense is called immediately after this */
  lifetimes << tstep - pack(create,density==0);

}

NEWCMD(reload,1)
{
  FILE *f;
  int size;
  f=fopen(argv[1],"r");
  fscanf(f,"%d\n",&size);
  glue buffer(size);
  fread(buffer.data,1,size,f);
  buffer.size=size;
  fclose(f);

#if MPI
  /* we must remap to a possibly new processor layout */
  get_all_globals(global_vars);
  iarray cell_map(ncells), idx(ncells);
  int i, j, pd;

  for (i=0, pd=1, idx=0; i<dims.size; pd*=dims[i++])
    idx+=global_vars.coords[i]*pd;
  cell_map[idx]=pcoord(ncells);

  global tmp, tmp_cell;
  tmp.unpack(buffer);

  if (sum(dims)!=sum(tmp.dims)) 
    error("geometry configuration inconsistent with checkpoint");

  /* ensure all extra fields (eg tstep) are correct */
  global_vars=tmp;

  for (int cell=0; cell<ncells; cell++)
    {
      for (i=0, pd=1, j=0; i<dims.size; pd*=dims[i++]) 
	j+=tmp.coords[i][cell]*pd;
      tmp_cell = tmp.get_cell_vars(cell);
      tmp_cell.ocell=cell_map[j];
      global_vars.put_cell_vars( tmp_cell ); 
    }
  /* reset cell layout */
  global_vars.coords[dims.size]=pcoord(ncells);
  distribute_data();
#else
  global_vars.unpack(buffer);
#endif
}

NEWCMD(checkpoint,1)
{
  FILE *f;
  glue buffer;
#if MPI
  global full_global=global_vars;
  get_all_globals(full_global);
  full_global.packup(buffer);
#else
  global_vars.packup(buffer);
#endif
  f=fopen(argv[1],"w");
  fprintf(f,"%d\n",buffer.size);
  fwrite(buffer.data,1,buffer.size,f);
  fclose(f);
}  

