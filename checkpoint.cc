#include "arrays.h"
#include "globals.h"
#include "tcl++.h"
extern global global_vars;
extern int ngcells;  /* total number of cells in system = sum_p(ncells(p)) 
			where p=0..nprocs */

NEWCMD(reload,1)
{
  FILE *f;
  int size;
  f=fopen(argv[1],"r");
  if (f==NULL) perror("Reload error");
  fscanf(f,"%d\n",&size);
  glue buffer(size);
  fread(buffer.data,1,size,f);
  buffer.size=size;
  fclose(f);

#if USE_MPI
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
  ngcells=ncells;
  distribute_data();
#else
  global_vars.unpack(buffer);
  ngcells=ncells;
#endif
}

NEWCMD(checkpoint,1)
{
  FILE *f;
  glue buffer;
#if USE_MPI
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

