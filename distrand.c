#include <math.h>
#include <stdlib.h>
#include <assert.h>

static double *P=NULL;
static int *PP=NULL;
static double *a=NULL;
static int Pwidth;

static int delta(double x, int i)
{
  /*  return (int)(x*pow(10,i)) % 10;*/
  return (int)(x*pow(16,i)) % 16;
}

void init_distrand( double (*f)(double), int nsamp, int width, 
		    double min, double max, unsigned int seed)
{
  int i,j,k,l;
  double binsz=(max-min)/nsamp, sump;
  double *p;

  /* initialize point probability array */
  p=(double*)malloc(sizeof(double)*nsamp);  
  for (sump=i=0; i<nsamp; i++)
    sump += p[i] = f( i*binsz + min);
  for (i=0; i<nsamp; i++) p[i]/=sump;   /* renormalize the p array */


  /* set up the threshold arrays */
  free(P); free(PP);
  P=(double*)malloc(sizeof(double)*(width+1));
  PP=(int*)malloc(sizeof(int)*(width+1));
  P[0]=0; PP[0]=0; Pwidth=width;
  for (i=1; i<=width; i++)
    {
      P[i]=0; PP[i]=0;
      for (j=0; j<nsamp; j++)
	{
	  PP[i]+=delta(p[j],i);
	}	  
      PP[i]+=PP[i-1];
      /*      P[i]=P[i-1] + pow(10,-i)*PP[i];*/
      P[i]=P[i-1] + pow(16,-i)*PP[i];
    }

  /* set up the lookup table */
  free(a);
  a=(double*)malloc(sizeof(double)*PP[width]);

  for (i=0; i<width; i++)
    for (j=0, l=PP[i]; j<nsamp; j++)
      for (k=0; k<delta(p[j],i+1); l++,k++)
	a[l]=j*(max-min)/nsamp + min;

  /* set seed for uniform random generator -1 means randomize */
  if (seed==0) srand((unsigned int) clock());
  else srand(seed);

  free(p);
}

double distrand()
{
  double u=(double)rand()/RAND_MAX;
  int i;

  if (P==NULL)
    {
      printf("distrand not initialized\n");
      abort();
    }

  for (i=0; i<Pwidth; i++)
    if (u>=P[i] && u<P[i+1]) break;

  return a[(int)((1<<(4*(i+1)))*(u-P[i]))+PP[i]];
}


