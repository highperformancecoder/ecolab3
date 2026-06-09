
#include <float.h>
#include <math.h>
#include <strstream.h>
#include <unistd.h>

#include "arrays.h"
#include "tcl++.h"
#include "globals.h"
#include "analysis.h"
#include "maxmin.h"

#ifdef BLT
extern "C" {
#include <blt.h>
}
#endif

char** palette_class::table=NULL;
int palette_class::size=0;

palette_class::palette_class()
{
  int  elemc;

  if (size==0)
   {
     tclvar palette("palette");
     char **elem;

     if (exists(palette))
       {
	 if (Tcl_SplitList(interp,palette,&elemc,&elem)!=TCL_OK) 
	   longjmp(TclError,1);
	 size=elemc;
	 table = new char *[size];
	 for (int j=0; j<elemc; j++)
	     {
	       table[j] = new char[strlen(elem[j])+1];
	       strcpy(table[j],elem[j]);
	     }
	 Tcl_Free((char*)elem);
       }
     else
       {
	 size=1;
	 table = new char *[size];
	 table[0]="black";
       }
   }
}

/*
display cell

display a line plot of species densities as a function of time
if cell not specified, then cell 0 is assumed.
*/

void display_stub(iarray density, iarray species, iarray nsp, 
		  int ncells, int cell)
{ 
#if BLT
  tclcmd cmd;
  tclvar palette_var("palette");
  palette_class palette;
  static iarray mapped;
  static Blt_Vector time;
  static iarray lasttime;
  iarray offs;
  int minsp, maxsp;
  char display[30], cmdstr[512];

  /*  work out species range */
  offs=enumerate(nsp);
  minsp=offs[cell];
  maxsp=minsp+nsp[cell];

  sprintf(display,"display%-d",cell);

  /* If first time, then create display widget in separate window */
  if (mapped.size==0) mapped=(iarray(ncells)=0);
  if (mapped[cell]==0)
    {
      Tk_Window graph;  
      graph=Tk_CreateWindow(interp,mainWin,display,"");
      Tk_GeometryRequest(graph,300,300);
      Tk_MapWindow(graph);
      cmd << "init_display" << cell << index_name(cell) << "\n";
      mapped[cell]=1;
    }

  if (lasttime.size==0) {lasttime=iarray(ncells); lasttime=0;}
  if (lasttime[cell]==tstep) return; /* don't add unnecessary data */
  lasttime[cell]=tstep;
  
  for (int i=minsp; i<maxsp; i++)
      {
	/* This should be big enough !! */
	char elname[25];
	sprintf(elname,"%s_line%-d",display,(int)species[i]);
	cmd << "array exists" << elname << "\n";
	if (!atoi(cmd.result))  /* new species - create new graph element */
	  {
	    sprintf(cmdstr,"vector %s([%s_time length])\n", elname, display);
	    cmd << cmdstr;
	    sprintf(cmdstr,
   ".%s.graph element create %s -label \"\" -xdata %s_time -ydata %s",
		    display,elname,display,elname); 
	    cmd << cmdstr;
	    if (exists(palette_var)) cmd << "-color" << palette[species[i]];
	    cmd << "-pixels 0\n";
	  }
	cmd << elname << "append" << density[i] << "\n";
      }

  sprintf(cmdstr,"%s_time append ",display);
  cmd << cmdstr << tstep << "\n";
  
#endif
}  


/* 
connect_plot
Display a plot of the species' connectivities (i.e. the sparsity
structure of interaction 
*/

void connect_stub(iarray density, sparse_mat interaction, iarray nsp, 
		  int ncells)
{
  tclcmd cmd;
  int err, i, low_colour, high_colour, offx, offy, maxnsp, 
    cell, offset;
  tclvar display_scale(".connections.scale");
  double scale;
  palette_class palette;
  tclindex idx;
  static int first_time=1;


  if (exists(display_scale))
    scale = display_scale;
  else 
    scale=1;


  /* If first time, then create display widget in separate window */
  if (first_time)
    {
      cmd << "init_connect\n";
      first_time=0;
    }
      /*
    {
      graph=Tk_CreateWindow(interp,mainWin,"connections","");
      Tk_GeometryRequest(graph,300,300);
      Tk_MapWindow(graph);
      
      graph=Tk_NameToWindow(interp,".connections",mainWin);
    }
      */

  /* handle any resize requests and clear of old data */
  else cmd << "update_connect\n";

  /* calculate maximum nsp in a cell */
  maxnsp=max(nsp);

  for (cell=0, offset=0; cell<ncells; offset += nsp[cell++])
    {
     /* calculate offset values */
      switch (dims.size)
	{
	case 0:
	  offx=offy=0;
	  break;
	case 1:
	  {
	    int sqdim=(int)(sqrt((float)dims[0])+.5);
	    offx = (cell % sqdim) * maxnsp;
	    offy = (cell / sqdim) * maxnsp;
	    break;
	  }
	default:
	  {
	    int sqdim=dims[0];
	    offx = (cell % sqdim) * maxnsp;
	    offy = (cell / sqdim) * maxnsp;
	    break;
	  }
	}

      /* draw red box around cell */
      cmd << ".connections.graph create rectangle" << 
	offx * scale << offy * scale << 
	(offx + maxnsp) * scale << (offy + maxnsp) * scale << 
	"-outline red\n";

      /* extract row and column slices */
      iarray slice = interaction.row >= offset && 
	interaction.row < nsp[cell] + offset;
      int ntrue = sum(slice);
      iarray row=pack(interaction.row,slice,ntrue)-offset;
      iarray col=pack(interaction.col,slice,ntrue)-offset;
      assert(sum(col<0)==0 && sum(col>=nsp[cell])==0);

      iarray enum_clusters(nsp[cell]);
      enum_clusters=1; 
      enum_clusters=enumerate(enum_clusters);
      for (i=0; i<row.size; i++)
	{ 
	  if (enum_clusters[row[i]] > enum_clusters[col[i]])
	    {
	      high_colour = enum_clusters[row[i]];
	      low_colour =  enum_clusters[col[i]];
	    }
	  else
	    {
	      high_colour = enum_clusters[col[i]];
	      low_colour =  enum_clusters[row[i]];
	    }
	  enum_clusters = merge( enum_clusters==high_colour, 
				 low_colour, enum_clusters);
	}
      
      /* for grouping species into their ecologies */
 
      iarray map(nsp[cell]), mask;
      map=-1;
      
      for (i=0; i<=max(enum_clusters); i++)
	{
	  mask = enum_clusters==i;
	  map = merge(  mask, enumerate(mask)+max(map)+1, map);
	}
      
      /* create coloured rectangles displaying the plot */
      for (i=0; i<row.size; i++)
	{
	  cmd <<  ".connections.graph create rectangle " << 
	    (map[col[i]]+offx) * scale << 
	    (map[row[i]]+offy) * scale << 
	    (map[col[i]]+1+offx) * scale << 
	    (map[row[i]]+1+offy) * scale;
	  //(int) shouldn't be necessary!
	  if ((int)density[row[i]+offset] == 0 ||   
	      (int) density[col[i]+offset] ==0) 
	    /* a species is extinct, connection is dead ! */
	    cmd << " -fill wheat\n";
	  else 
	    cmd << " -fill " << palette[enum_clusters[row[i]]] << "\n";
	}
     
      /* do diagonals */
      for (i=0; i<nsp[cell]; i++)
	cmd <<  ".connections.graph create rectangle " << 
	  (map[i]+offx)*scale << (map[i]+offy)*scale << 
	  (map[i]+1+offx)*scale << (map[i]+1+offy)*scale <<  " -fill " << 
	  (density[i+offset]? palette[enum_clusters[i]]: "wheat") << "\n";
    }
}





/* setmaxmin name */
NEWCMD(setmaxmin,1)
{
  FILE *f;
  char *scratch=new char[20+strlen(argv[1])];
  char buf[100];

  sprintf(scratch,"%s.dat",argv[1]);
  f=fopen(scratch,"r");

  sprintf(scratch,".%s.max",argv[1]);
  tclvar max(scratch);

  sprintf(scratch,".%s.min",argv[1]);
  tclvar min(scratch);

  double lmax=max, lmin=min;

  while (fgets(buf,100,f)>0)
    {
      double v=atof(buf);
      if (v>lmax) lmax=v;
      if (v<lmin) lmin=v;
    }

  max=lmax; min=lmin;
  fclose(f);
  delete [] scratch;
}

/* fillyarray name min delta */
#if BLT
NEWCMD(fillyarray,3)
{
  FILE *f;
  char *scratch=new char[20+strlen(argv[1])];
  char buf[100];
  double min=atof(argv[2]), delta=atof(argv[3]);
  Blt_Vector *y;

  sprintf(scratch,"%s.dat",argv[1]);
  f=fopen(scratch,"r");
  
  sprintf(scratch,".%s.xlogison",argv[1]);
  int xlogison=tclvar(scratch);  /* x logscale flag */

  sprintf(scratch,"%s_y",argv[1]);
#if TK3
  Blt_Vector oy;
  if (Blt_GetVector(interp,scratch,&oy)!=TCL_OK) 
    error(interp->result);
  y=&oy;
#else
   if (Blt_GetVector(interp,scratch,&y)!=TCL_OK) 
    error(interp->result);
#endif 
  tclvar yarr(scratch);  /* The TCL array variable */

  for (int i=0; i<y->numValues; i++) y->valueArr[i]=0;
  while (fgets(buf,100,f)>0)
    {
      double v=atof(buf);
      if (xlogison)
	y->valueArr[(int)((log(v)-log(min))/delta)]++;
      else
	y->valueArr[(int)((v-min)/delta)]++;
    }

#ifdef TK3
  if (Blt_ResetVector(interp,scratch,y,TCL_STATIC)!=TCL_OK) 
    error(interp->result);
#else
  if (Blt_ResizeVector(y,y->numValues)!=TCL_OK) 
    error(interp->result);
#endif
  fclose(f);
  delete [] scratch;
}
#endif

NEWCMD(newwin,1)
{
  Tk_Window window;
  if ((window=Tk_NameToWindow(interp,argv[1],mainWin))==NULL)
    {
      /* Create Window */
      window=Tk_CreateWindowFromPath(interp,mainWin,argv[1],"");
      Tk_GeometryRequest(window,300,300);
      Tk_MapWindow(window);
    }
}

/* Bunch of reduction functions */

NEWCMD(max,1)
{
  int i,n;
  char **v;
  double m=DBL_MIN;
  tclreturn result;
  if (Tcl_SplitList(interp,argv[1],&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m=max(m,atof(v[i]));
  result << m;
} 


NEWCMD(min,1)
{
  int i,n;
  char **v;
  double m=DBL_MAX;
  tclreturn result;
  if (Tcl_SplitList(interp,argv[1],&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m=min(m,atof(v[i]));
  result << m;
} 


NEWCMD(av,1)
{
  int i,n;
  char **v;
  double m=0;
  tclreturn result;
  if (Tcl_SplitList(interp,argv[1],&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m+=atof(v[i]);
  result << m/n;
} 


