#include "ecolab.h"
#include <signal.h>
#include <unistd.h>

/* definitions of global variables ---
   4 iarrays, 4 arrays and 1 sparse_mat */
global global_vars(3,4,1);

/* iarrays */
iarray &density = global_vars.iarrays[0];
iarray &species = global_vars.iarrays[1];
iarray &create = global_vars.iarrays[2];

/* arrays */
array &repro_rate = global_vars.arrays[0];
array &mutation = global_vars.arrays[1];
array &mig_ns = global_vars.arrays[2];
array &mig_ew = global_vars.arrays[3];

/* sparse_mats */
sparse_mat &interaction = global_vars.sparse_mats[0];

#undef GLOBAL_INIT_HOOK
#define GLOBAL_INIT_HOOK init_global_vars();


void init_ecolab_array(array &a, char *name)
{
  int i;
  a = array(sum(nsp));
  iarray offs=enumerate(nsp);
  array tmp(max(nsp));
  if (initialize( tmp,name))
    for (i=0; i<ncells; i++)
      a[pcoord(nsp[i])+offs[i]] = tmp[pcoord(nsp[i])];
  else
    for (i=0; i<ncells; i++)
      {
	char nm[256];
	tmp=array(nsp[i]);
	sprintf(nm,"%s(%s)",name,index_name(i));
	if (!initialize(tmp,nm))
	  printf("Cannot initialize %s",name);
	a[pcoord(nsp[i])+offs[i]] = tmp;
      }
}

void init_global_vars()
{
  int i;

  /* we can set density.size to zero to reread the values */
  if (myid==0)
    if (density.size==0)  
    {
      init_scalars();

      density = iarray(sum(nsp));
      iarray tmp(max(nsp)), offs=enumerate(nsp);
      if (initialize( tmp,"density"))
	for (i=0; i<ncells; i++)
	  density[pcoord(nsp[i])+offs[i]] = tmp[pcoord(nsp[i])];
      else
	for (i=0; i<ncells; i++)
	  {
	    char nm[256];
	    tmp=iarray(nsp[i]);
	    sprintf(nm,"density(%s)",index_name(i));
	    if (!initialize(tmp,nm))
	      error("Cannot initialize %s",nm);
	    density[pcoord(nsp[i])+offs[i]] = tmp;
	  }

      for (i=0; i<ncells; i++) species <<=pcoord(nsp[i]);
      create=iarray(density.size); create=0;

      init_ecolab_array( repro_rate,"repro_rate");
      init_ecolab_array( mutation,"mutation");
      init_ecolab_array( mig_ns, "migration(north_south)" );
      init_ecolab_array( mig_ew, "migration(east_west)" );
      
      sparse_mat s(max(nsp));
      if (initialize_offdiag( s, "interaction(offdiag)", s.diag.size))
	for (i=0; i<ncells; i++)
	  interaction.insert(s.submat(0,nsp[i]),offs[i],0);
      else
	for (i=0; i<ncells; i++)
	  {
	    char nm[256];
	    sprintf(nm,"interaction(offdiag,%s)",index_name(i));
	    if (!initialize_offdiag( s, nm, nsp[i]))
	      error("Initialisation of %s", nm);
	    interaction.insert(s.submat(0,nsp[i]),offs[i],0);
	  }
      init_ecolab_array( interaction.diag, "interaction(diag)" );

#if USE_MPI
      distribute_data();
#endif
      tclvar model_valid("model_valid");
      model_valid=1;
    }

}

NEWCMD(generate,0)
{ 
  PARALLEL;
  density +=repro_rate * density + (interaction * density) * density;
  tstep++;
}


NEWCMD(condense,0)   /* remove extinct species */
{
  PARALLEL;
  iarray mask, map, mask_off, extinctions;
//  int mask_true;
//  mask = density != 0;
  int mask_true, i;

  /* Change mask so that it is true if a species is extant in any cell */
  /*  mask = density != 0; */
  iarray extant=iarray(max(species)+1);
  extant=0;
  for (i=0; i<density.size; i++) 
    if (density[i]>0) extant[species[i]]=1;
  mask=extant[species];

  mask_true=sum(mask);
  if (density.size==mask_true) return; /* no change ! */

  map = enumerate( mask );
  //  mask_off = density[ interaction.row ] !=0 && density[ interaction.col ] !=0;
  mask_off = 
    ((iarray)extant[species[interaction.row]])&&((iarray)extant[species[interaction.col]]);

  density = pack( density, mask, mask_true); 
  repro_rate = pack(repro_rate, mask, mask_true); 
  mutation = pack(mutation, mask, mask_true); 
  create = pack(create, mask, mask_true); 
  species = pack(species, mask, mask_true); 
  interaction.diag = pack(interaction.diag, mask, mask_true);
  mig_ns = pack(mig_ns, mask, mask_true);
  mig_ew = pack(mig_ew, mask, mask_true);


  mask_true=sum(mask_off);

  interaction.val = pack(interaction.val, mask_off, mask_true); 
  interaction.row = map[pack(interaction.row, mask_off, mask_true)];
  interaction.col = map[pack(interaction.col, mask_off, mask_true)];

  /* adjust the nsp array */
  if (ncells>1)
    {
      int i;
      iarray tmp_nsp=nsp;
      extinctions = gen_index(!mask);
      for (i=0; i<extinctions.size; i++) 
	tmp_nsp[which_cell(extinctions[i])]--;
      nsp=tmp_nsp;
      assert(sum(nsp)==density.size);
    }
  else
    nsp[0] = density.size;

  assert(sum(interaction.diag>=0)==0);
  assert(sum(mutation<0)==0);
  for (int i=0; i<interaction.row.size; i++)
    assert(which_cell(interaction.row[i])==which_cell(interaction.col[i]));
}


/* 
Vary components according to a Gaussian distribution, with the given
  std deviation 
*/

/* do the offdiagonal mutations */
void do_row_or_col(array& tmp, double range, double minval, double gdist)
{
  double r;
  int j, ntrue, pos;

  /* create or delete some connections */
  r = (2.0*rand())/RAND_MAX - 1;
  if (r!=0) ntrue=(int)(1/fabs(r))-1;
  else ntrue = tmp.size;

  if (ntrue>tmp.size) ntrue=tmp.size;

  if (r>0)
    for (j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size-1) * ((float) rand()/RAND_MAX) +.5);
	if (tmp[pos]==0.0)
	  tmp[pos] = range *((float) rand()/RAND_MAX) + minval;
      }
  else
    for (j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size-1) * ((float) rand()/RAND_MAX) +.5);
	tmp[pos]=0;
      }

  /* mutate values */
  array diff(tmp.size);
  diff=merge(tmp!=0.0,range*gdist,0.0);
  gspread(tmp,diff);
}  

NEWCMD(mutate,0)
{
  /* force load of these variables */
  tclvar sp_sep("sp_sep");
  tclvar mut_max("mutation(random,maxval)");
  tclvar repro_bnds("repro_rate(random)");
  tclvar odiagbnds("interaction(offdiag,random)");
  tclvar minval("repro_rate(random,minval)"); 
  tclvar ominval("interaction(offdiag,random,minval)");
  tclvar omaxval("interaction(offdiag,random,maxval)");
  PARALLEL;

  iarray new_sp;
  array new_repro_rate, new_mutation, new_idiag, new_mig_ns, new_mig_ew;
  int i, cell, j, ntrue;
  static int sp_cntr=1;    /* used to label newly created species */

  /* ensure sp_cntr operates in a different domain on each
     processor. This is done by initialising it with
     max(species)+myid+1, then incrementing it by nprocs each time a
     new species is generated */

  if (sp_cntr==1)
    {
#ifdef USE_MPI
	int sp_cntr_i=max(species);
	MPI_Allreduce(&sp_cntr_i, &sp_cntr, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
	sp_cntr+=myid+1;
#else
      sp_cntr=max(species)+myid+1;
#endif
    }

  for (cell=0; cell<ncells; cell++)
    {
      global l=global_vars.get_cell_vars(cell);

     /* calculate the number of mutants each species produces */
      new_sp = (double) sp_sep * l[repro_rate] * l[mutation] * l[density];

      /* adjust density by mutant values i.e. consider that some organisms
	 born in previous step are now discovered to be mutants */
      l[density]-=new_sp;

      /* generate index list of old species that mutate to the new */
      iarray t=new_sp;
      new_sp = gen_index(new_sp); 

      if (new_sp.size==0) continue;

      /* adjust row and col to "within cell" coordinates */

      /* collect the old phenotypes */
      new_repro_rate = l[repro_rate][new_sp];
      new_idiag = l[interaction].diag[new_sp];
      new_mutation = l[mutation][new_sp];
      new_mig_ns = l[mig_ns][new_sp];
      new_mig_ew = l[mig_ew][new_sp];

      /* calculate the genetic distances for the mutants from the parents*/
      array gdist(new_sp.size);
      fillprand(gdist);
      gdist *= new_mutation;

      /* 
	 change phenotypes randomly, according a normal distribution 
	 with std dev gdist 
	 */
      double range = exists(repro_bnds["maxval"])? 
	(double) repro_bnds["maxval"]: 1.0 -
	exists(repro_bnds["minval"])? (double) repro_bnds["minval"]: 0.0;

      gspread( new_repro_rate, range*gdist );
      array a=new_idiag;
      lgspread( new_idiag, gdist );
      lgspread( new_mutation, gdist );
      lgspread( new_mig_ns, gdist );
      lgspread( new_mig_ew, gdist );

      /* limit idiag to avoid it vanishing (causes system instability) - a
	 reasonable is to chose it so that the equilibrium value of
	 density is always less than half of INT_MAX */
      array max_idiag = -abs(new_repro_rate)/(0.1*INT_MAX);
      new_idiag = merge( new_idiag < max_idiag, new_idiag, max_idiag);

      /* limit mutation rate to mutation(random,maxval) */
      new_mutation = merge( new_mutation < mut_max, new_mutation, mut_max);

      l[repro_rate] <<=  new_repro_rate;
      l[interaction].diag <<=  new_idiag;
      l[mutation] <<= new_mutation;
      l[mig_ns] <<= new_mig_ns;
      l[mig_ew] <<= new_mig_ew;

      /* adjust nsp */
      l.nsp[0] += new_sp.size;

      /* vary offdiagonal elements */
      double maxval=exists(odiagbnds["maxval"])? 
	(double) odiagbnds["maxval"]: 1.0;
      double minval=exists(odiagbnds["minval"])? 
	(double) odiagbnds["minval"]: 0.0;
      range =  maxval-minval;

      /* collect interaction data */
      for (i=0; i<new_sp.size; i++)
	{
	  array tmp1(nsp[cell]+i), tmp2(nsp[cell]+i);
	  double s;
	  tmp1 = 0; tmp2=0;
	  iarray pcd = pcoord(tmp1.size);
	      
	  /* project out connections for row[new_sp[i]] */
	  iarray mask = l[interaction].row==new_sp[i];
	  int ntrue = sum(mask);
	  tmp1[ pack(l[interaction].col,mask,ntrue) ] = 
	    pack(l[interaction].val,mask,ntrue);
	  tmp1[ new_sp[i] ] = l[interaction].diag[new_sp[i]];

	  do_row_or_col(tmp1,range,minval,gdist[i]);
      
	  /* project out connections for col[new_sp[i]] */
	  mask = l[interaction].col==new_sp[i];
	  ntrue = sum(mask);
	  tmp2[ pack( l[interaction].row, mask, ntrue) ] = 
	    pack( l[interaction].val, mask, ntrue);
	  tmp2[ new_sp[i] ] = l[interaction].diag[new_sp[i]];

	  do_row_or_col(tmp2,range,minval,gdist[i]);

	  /* adjust offdiag vals so that n.\beta n<0 for all positive n */
	  /* construct list of offdiag vals that sum positively */
	  mask = tmp1+tmp2>0;
	  s=sum(tmp1+tmp2,mask)+new_idiag[i];
	  if (s>0)
	    {
	      int nadj;
	      iarray m1=mask && tmp1!=0.0;
	      iarray m2=mask && tmp2!=0.0;
	      nadj = sum(m1 || m2);
	      tmp1=merge(m1,tmp1-s/nadj,tmp1);
	      tmp2=merge(m2,tmp2-s/nadj,tmp2);
	    }

	  /* pack up to add to interaction */
	  mask=tmp1!=0.0;
	  ntrue = sum(mask);
	  l[interaction].val <<= pack(tmp1,mask,ntrue);
	  l[interaction].col <<= pack( pcd, mask, ntrue);
	  l[interaction].row <<= pack( nsp[cell]+i, mask, ntrue);

	  mask=tmp2!=0.0;
	  ntrue = sum(mask);
	  l[interaction].val <<= pack( tmp2, mask, ntrue);
	  l[interaction].row <<= pack( pcd, mask, ntrue);
	  l[interaction].col <<= pack( nsp[cell]+i, mask, ntrue);

	}

      /* concatonate new species */
      new_sp = 1;
      l[density]  <<= new_sp;
      new_sp = 0;
      l[create] <<=  new_sp;
      l[species] <<= pcoord(new_sp.size)*nprocs + sp_cntr;
      sp_cntr+=new_sp.size*nprocs;

      global_vars.put_cell_vars(l);
    }

  assert(sum(interaction.diag>=0)==0);
  assert(sum(mutation<0)==0);
#ifndef NDEBUG
  for (i=0; i<interaction.row.size; i++)
    assert(which_cell(interaction.row[i])==which_cell(interaction.col[i]));
#endif
}


#if USE_MPI
static global north, east, west, south;
#endif

global get_neighbour(int cell, int dx, int dy, int torus)
{
  iarray *coords=global_vars.coords;
  int minx=coords[0][0], miny=coords[1][0];
  int maxx=coords[0][ncells-1], maxy=coords[1][ncells-1];
  int nx=coords[0][cell]+dx, ny=coords[1][cell]+dy;

  /* do boundary conditions */
#if USE_MPI
  if (torus)
    {
      if (nx<minx)
	if (torus || myid%proc_dims[0]!=0) 
	  {
	    global r=west.get_cell_vars(ny-miny);
	    /* coords[dims.size] stores the global cell coordinate */
	    r.ocell=west.coords[2][ny-miny]; 
	    return r;
	  }
	else nx=minx;
      else if (nx>maxx) 
	if (torus || myid%proc_dims[0]!=proc_dims[0]-1) 
	  {
	    global r=east.get_cell_vars(ny-miny);
	    /* dims stores the global cell coordinate */
	    r.ocell=east.coords[2][ny-miny];
	    return r;
	  }
	else nx=maxx;
      else if (ny<miny) 
	if (torus || myid/proc_dims[0]!=0) 
	  {
	    global r=north.get_cell_vars(nx-minx);
	    /* dims stores the global cell coordinate */
	    r.ocell=north.coords[2][nx-minx];
	    return r;
	  }
	else ny=miny;
      else if (ny>maxy) 
	if (torus || myid/proc_dims[0]!=proc_dims[1]-1) 
	  {
	    global r=south.get_cell_vars(nx-minx);
	    /* dims stores the global cell coordinate */
	    r.ocell=south.coords[2][nx-minx];
	    return r;
	  }
	else ny=maxy;
    }
#else
  if (torus)
    {
      if (nx<minx)       nx=maxx;
      else if (nx>maxx) nx=minx;
      else if (ny<miny)  ny=maxy;
      else if (ny>maxy) ny=miny;
    }
  else
    {
      if (nx<minx)       nx=minx;
      else if (nx>maxx) nx=maxx;
      else if (ny<miny)  ny=miny;
      else if (ny>maxy) ny=maxy;
    }
#endif

  return global_vars.get_cell_vars((nx-minx)+(ny-miny)*(maxx-minx+1));
}

/* embed all the following cells in their superspace */
void embed(int ncells, iarray& species, global* cell0, ...)
{
  va_list ap;
  va_start(ap,cell0);
  int i, j, k, maxsp=max((*cell0)[species]);
  typedef global *globalp;
  global **cell=new globalp[ncells];
  cell[0]=cell0;

  /* compute a superspace that contains all species - living and extinct 
     first compute the dimension */
  for (i=1; i<ncells; i++)
    {
      cell[i]=va_arg(ap,global*);
      maxsp=max(maxsp,max((*cell[i])[species]));
    }

  /* now set up a mask of living species contained within the list of cells */
  maxsp++;
  iarray gmask(maxsp);
  gmask=0;
  for (i=0; i<ncells; i++)
    gmask[ (*cell[i])[species] ] = 1;
  int ntrue=sum(gmask);
  iarray map=enumerate(gmask);

  /* do the arrays */
  for (j=0; j<cell0->narr; j++)
    {
      array arr(ntrue);
      arr=-1.0;
      for (i=0; i<ncells; i++)
	{
	  assert( 
		 sum( 
		     arr[map[(*cell[i])[species]]] != (*cell[i]).arrays[j], 
		     arr[map[(*cell[i])[species]]]!=-1.0
		     )==0
		 );
	  arr[map[(*cell[i])[species]]] = (*cell[i]).arrays[j];
	}
      for (i=0; i<ncells; i++)
	(*cell[i]).arrays[j] = arr;
    }

  /* now the sparse_mats */
  
  BitSet b;
  for (j=0; j<cell0->nspars; j++)
    {
      int maxoff=0;
      for (i=0; i<ncells; i++) maxoff+=(*cell[i]).sparse_mats[j].row.size;

      sparse_mat s(ntrue,maxoff);
      int r, c;
      for (maxoff=i=0; i<ncells; i++)
	{
	  s.diag[map[(*cell[i])[species]]] = (*cell[i]).sparse_mats[j].diag;

	  /* loop over offdiagonal elements, and check to see if
             species pair already processed in a previous cell */
	  for (k=0; k<(*cell[i]).sparse_mats[j].row.size; k++)
	    {
	      r=(*cell[i])[species][(*cell[i]).sparse_mats[j].row[k]];
	      c=(*cell[i])[species][(*cell[i]).sparse_mats[j].col[k]];
	      if (!b.test(r) || !b.test(c)) 
		{     /* at least one such species is new */
		  s.row[maxoff] = map[r];
		  s.col[maxoff] = map[c];
		  s.val[maxoff] = (*cell[i]).sparse_mats[j].val[k];
		  maxoff++;
		}
	    }
	  /* load up this cell's species into the bitset b */
	  for (k=0; k<(*cell[i])[species].size; k++) 
	    b.set((*cell[i])[species][k]);
	}
      s.row.size=s.col.size=s.val.size=maxoff;

      for (i=0; i<ncells; i++)
	(*cell[i]).sparse_mats[j] = s;
    }
 
  /* finally the iarrays */
  for (i=0; i<ncells; i++)
    {
      iarray osp = (*cell[i])[species];
      /* all iarrays are extended by zeros, except ...*/
      for (j=0; j<cell0->niarr; j++)
	{
	  iarray iarr(maxsp);
	  iarr=0;
	  iarr[osp] = (*cell[i]).iarrays[j];
	  (*cell[i]).iarrays[j]=pack(iarr,gmask,ntrue);
	}
      /* the species array, which is extended by the actual species index */
      (*cell[i])[species]=pack(pcoord(maxsp),gmask,ntrue);
      (*cell[i]).nsp[0]=ntrue;
    }

  delete [] cell;
}



#if USE_MPI
/* circular shift the edges around the processors */
void shift(global &edge, int dest, int src)
{

  if (dest==src && dest==myid) return;/* nothing to be done */
  glue sendbuf;
  edge.packup(sendbuf); 
  
  /* send buffer sizes first */
  int sz=sendbuf.size, nsz;
  MPI_Status status;
  MPI_Sendrecv(&sz,1,MPI_INT,dest,1,
	       &nsz,1,MPI_INT,src,1,
	       MPI_COMM_WORLD,&status);

  /* now send & receive data */
  glue recvbuf(nsz); recvbuf.size=nsz;
  MPI_Sendrecv(sendbuf.data,sz,MPI_CHAR,dest,1,
	       recvbuf.data,nsz,MPI_CHAR,src,1,
	       MPI_COMM_WORLD,&status);
  edge.unpack(recvbuf);
}

/* get processor number located (dx,dy) relative to current */
int proc(int dx, int dy)
{
  /* first get current location */
  int x=myid%proc_dims[0];
  int y=myid/proc_dims[0];

  /* then adjust by displacement */
  x=mod(x+dx,proc_dims[0]);
  y=mod(y+dy,proc_dims[1]);
  return x+y*proc_dims[0];
}

#endif

static array mig_r;

iarray update(iarray od, iarray& ld, array mig, int edge)
{
  /* in order to ensure conservation of individuals, we have a random
     number mig_r[edge] in [0..1] that depends on the edge, instead of
     using the usual random array to iarray conversion */

  array m=mig * (od - ld);
  return (iarray)(m + (array)((m!=0.0)*(2*(m>0.0)-1)) * mig_r[edge]);
}

NEWCMD(migrate,0)
{
  int i, torus;
  global cell_vars, nvars, evars, wvars, svars, ngvars=global_vars;
  iarray *coords=global_vars.coords;
  tclvar tcl_torus("torus");
  PARALLEL;
  assert(dims.size==2);
  if (!exists(tcl_torus)) torus=1;  /* default is to have toroidal bdy cond */
  else torus=tcl_torus;
  mig_r=array(ngcells);

#if USE_MPI
  /* broadcast an array of random numbers to nodes */
  if (myid==0) fillrand(mig_r); 
  double *buf=(double*)mig_r;
  MPI_Bcast(buf,ngcells,MPI_DOUBLE,0,MPI_COMM_WORLD);
  mig_r=buf;   /* actually a no operation with contiguous memory */

  int minx=coords[0][0], miny=coords[1][0];
  int maxx=coords[0][ncells-1], maxy=coords[1][ncells-1];

  /* clear edge data */
  east=west=north=south=global();

  /* do east-west shift */
  for (i=0; i<=(maxy-miny)*(maxx-minx+1); i+=maxx-minx+1)
    {
      east.append(global_vars.get_cell_vars(i));
      west.append(global_vars.get_cell_vars(i+maxx-minx));
    }

  /* do north-south shift */
  for (i=0; i<=maxx-minx; i++)
    {
      south.append(global_vars.get_cell_vars(i));
      north.append(
	    global_vars.get_cell_vars(i+(maxy-miny)*(maxx-minx+1))
	    );
    }

  shift(west, proc(1,0), proc(-1,0));
  shift(east, proc(-1,0), proc(1,0));
  shift(north, proc(0,1), proc(0,-1));
  shift(south, proc(0,-1), proc(0,1));

#else
  fillrand(mig_r); 
#endif

  for (int cell=0; cell<ncells; cell++)
    {
      iarray nd, lsp, od, osp;

      /* get centre cell's variables */
      cell_vars = global_vars.get_cell_vars(cell);

      /* get neighbouring variables */
      wvars = get_neighbour(cell,-1,0,torus);
      evars = get_neighbour(cell,1,0,torus);
      nvars = get_neighbour(cell,0,-1,torus);
      svars = get_neighbour(cell,0,1,torus);

      /* now embed neigbouring cells into superspace */
      embed(5,species,&cell_vars, &wvars, &evars, &nvars, &svars);
      
      iarray& ld = cell_vars[density];
      nd = ld;
      nd+=update(wvars[density],ld,cell_vars[mig_ew],cell_vars.ocell);
      nd+=update(evars[density],ld,evars[mig_ew],evars.ocell);
      nd+=update(nvars[density],ld,cell_vars[mig_ns],cell_vars.ocell);
      nd+=update(svars[density],ld,svars[mig_ns],svars.ocell);
      ld=nd;
      ngvars.put_cell_vars(cell_vars);
    }
#ifndef NDEBUG
  int nmigs=sum(density)-sum(ngvars.iarrays[0]);
#if USE_MPI
  int s;
  MPI_Reduce(&nmigs,&s,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
  if (myid==0)
    assert(s==0);
#else
  assert(nmigs==0);
#endif
#endif
  global_vars=ngvars;
}
     
/* return the value of a specific variable as a TCL list */
NEWCMD(get,1)
{
  global g;
  get_all_globals(g);
  tclreturn result;
  if (strcmp(argv[1],"nsp")==0) result << g.nsp;
  if (strcmp(argv[1],"tstep")==0) result << g.tstep;
  if (strcmp(argv[1],"density")==0) result << g[density];
  if (strcmp(argv[1],"species")==0) result << g[species];
  if (strcmp(argv[1],"create")==0) result << g[create];
  if (strcmp(argv[1],"repro_rate")==0) result << g[repro_rate];
  if (strcmp(argv[1],"mutation")==0) result << g[mutation];
  if (strcmp(argv[1],"interaction(diag)")==0) 
    result << g[interaction].diag;
  if (strcmp(argv[1],"interaction(val)")==0) 
    result << g[interaction].val;
  if (strcmp(argv[1],"interaction(row)")==0) 
    result << g[interaction].row;
  if (strcmp(argv[1],"interaction(col)")==0) 
    result << g[interaction].col;
  if (strcmp(argv[1],"migration(north_south")==0) 
    result << g[mig_ns];
  if (strcmp(argv[1],"migration(east_west)")==0) 
    result << g[mig_ew];
}

NEWCMD(lifetimes,0) 
{ 
  int cell; 
  tclreturn lifetimes; 
  tclcmd cmd;

  for (int i=0; i<density.size; i++) 
    {
      if (create[i]==0 && density[i]>10) 
	create[gen_index(species==species[i])] = tstep;
      else if (create[i]>0 && sum(density, species==species[i])==0) 
	/* extinction */
	{
	  lifetimes << tstep - create[i] << " ";
	  create[i]=0;
	}
    }
}


extern "C" void dgees_();

NEWCMD(maxeig,0)
{
  int size=interaction.diag.size;
  double *beta=new double[size*size], *eigr=new double[size], 
    *eigi=new double[size], *work=new double[3*size], maxe;
  int i,j,lwork=3*size,info,idum,idum1=1;
  tclreturn result;

  for (i=0; i<size; i++)
    for (j=0; j<size; j++)
      beta[i*size + j]=0;

  for (i=0; i<size; i++) 
    beta[i*size+i]=interaction.diag[i];
  for (i=0; i<interaction.val.size; i++)
    beta[interaction.row[i]*size + interaction.col[i]]=interaction.val[i];

#ifdef LAPACK
  dgees_("N","N",NULL,&size,beta,&size,&idum,eigr,eigi,
	NULL,&idum1,work,&lwork,NULL,&info);
#endif

  if (info!=0)
    error("Eigenvalues not converged");

  maxe=-FLT_MAX;
  for (i=0; i<size; i++)
    maxe = max(maxe, eigr[i]);
  result << maxe;

  delete [] beta; delete [] eigr; delete [] eigi; delete [] work;
}

NEWCMD(display,-1)
{ 
  int cell;

  if (argc>1)
    cell=atoi(argv[1]);
  else
    cell=0;

  global g;
  get_all_globals(g);
  display_stub( g[density], g[species], g.nsp, g.ncells,cell);
}

NEWCMD(connect_plot,0)
{
  global g;
  get_all_globals(g);
  connect_stub(g[density],g[interaction],g.nsp,g.ncells);
}

NEWCMD(getconn,0)
{
  global g,l;
  get_all_globals(g);
  tclreturn result;
  for (int i=0; i<g.ncells; i++)
    {
      l=g.get_cell_vars(i);
      result << l[interaction].val.size / pow(l.nsp[0],2) << ' ';
    }
}

NEWCMD(strsq,0)
{
  global g,l;
  get_all_globals(g);
  tclreturn result;
  for (int i=0; i<g.ncells; i++)
    {
      l=g.get_cell_vars(i);
      array v=l[interaction].val, d=l[interaction].diag;
      int n=d.size*d.size;
      result << (sum(v*v)/n - pow(sum(v)/n,2)) / 
	pow(sum(d)/d.size,2) << ' ';
    }
}

NEWCMD(totdiversity,0)
{
  tclreturn r;
  global g;
  get_all_globals(g);
  iarray count(max(g[species])+1);
  count=0;
  count[species]=1;
  r<<sum(count);
}

/* Miscellaneous stuff for testing purposes */
NEWCMD(ivals,0)
{
   tclreturn result;
   sparse_mat temps=interaction;
   temps.diag=0; 
   result << temps * density;
}   

NEWCMD(ngt10,0)
{
  tclreturn result;
  result << sum(density>=10);
}

NEWCMD(act_wave,-1)
{ 
  int cell;
  static iarray intd;

  if (argc>1)
    cell=atoi(argv[1]);
  else
    cell=0;

  int maxsp=max(species);
  if (maxsp>=intd.size) 
    {
      iarray zero(maxsp-intd.size+1);
      zero=0;
      intd <<= zero;
    }
  intd[species] += density;
  iarray intds=intd[species];
  display_stub( intds, species, nsp, ncells,cell);
}



NEWCMD(getrand,0)
{
  tclreturn() << rand();
}

NEWCMD(scalemig,1)
{
  PARALLEL;
  double s=atof(argv[1]);
  mig_ew*=s;
  mig_ns*=s;
}
