/* serial emulation of C* support */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#ifndef RAND_MAX
#define RAND_MAX  2147483647  /* This is missing from <stdlib.h>? */
#endif

/* array support */

#define binop(op,opname,name,rettype,type)\
void name##_##opname(int size, rettype* r,type* x,type* y)\
{\
  int i;\
  for (i=0; i<size; i++)\
    r[i] = x[i] op y[i];\
}

#define C_defs(name,type)\
void *new_##name(int s)\
{\
  type *r;\
  r=s? malloc(s*sizeof(type)): NULL;\
  return r;\
}\
void delete_##name(type *x) \
{\
  free(x);\
}\
type *cs_getlist_##name(type *x) {return x;}\
void cs_putlist_##name(type *x, type *y, int size)\
    {memmove(x,y,size*sizeof(type));}\
type get_##type(type *x,int i) {return x[i];}\
void put_##type(type *x,int i,type y) {x[i]=y;}\
void copy_##type(int size,type *x,type *y) \
{ \
  int i;\
  for (i=0; i<size; i++) x[i]=y[i];\
}\
/* vector indexing */\
void get_##type##_array(int size, type* r, type* x, int* i)\
{\
  int j;\
  for (j=0; j<size; j++) \
    r[j] = x[i[j]];\
}\
void put_##type##_array(int size, type* x, int* i,type* y)\
{\
  int j;\
  for (j=0; j<size; j++)\
    x[i[j]] = y[j];\
}\
void broadcast_##type(int size, type* x,type y)\
{\
  int j;\
  for (j=0; j<size; j++)\
    x[j]=y;\
}\
  binop(+,plus,name,type,type)\
  binop(-,minus,name,type,type)\
  binop(*,mul,name,type,type)\
  binop(/,div,name,type,type)\
  binop(<,lt,name,int,type)\
  binop(<=,le,name,int,type)\
  binop(>,gt,name,int,type)\
  binop(>=,ge,name,int,type)\
  binop(==,eq,name,int,type)\
  binop(!=,ne,name,int,type)\
void name##_cat(type* r,int xsize,type* x,int ysize,type* y)\
{\
  int i;\
  for (i=0; i<xsize; i++)\
    r[i] = x[i];\
  for (i=0; i<ysize; i++)\
    r[i+xsize] = y[i];\
}\
void cs_merge_##type(int size, type* r, int* m, type *x, type* y)\
{\
  int i;\
  for (i=0; i<size; i++) r[i] = m[i]?x[i]:y[i];\
}\
type cs_max_##name(int size, type *x)\
{\
  int i;\
  type m=x[0];\
  for (i=0; i<size; i++)\
    m=(m>x[i])? m: x[i];\
  return m;\
}\
type cs_masked_max_##name(int size, type *x, int *mask)\
{\
  int i, first=1;\
  type m;\
  for (i=0; i<size; i++) \
    if (mask[i]) \
       if (first) \
	{\
	  first=0;\
	  m=x[i];\
	}\
      else\
        m=(m>x[i])? m: x[i];\
  return m;\
}\
type cs_min_##name(int size, type *x)\
{\
  int i;\
  type m=x[0];\
  for (i=0; i<size; i++)\
    m=(m<x[i])? m: x[i];\
  return m;\
}\
type cs_masked_min_##name(int size, type *x, int *mask)\
{\
  int i, first=1;\
  type m;\
  for (i=0; i<size; i++) \
    if (mask[i]) \
      if (first) \
	{\
	  first=0;\
	  m=x[i];\
	}\
      else\
        m=(m<x[i])? m: x[i];\
  return m;\
}\
type cs_sum_##name(int size, type *x)\
{\
  int i;\
  type r;\
  for (i=0, r=0; i<size; i++) r+=x[i];\
  return r;\
}\
type cs_masked_sum_##name(int size, type *x, int *mask)\
{\
  int i;\
  type r;\
  for (i=0, r=0; i<size; i++) if (mask[i]) r+=x[i];\
  return r;\
}\
type cs_prod_##name(int size, type *x)\
{\
  int i;\
  type r;\
  for (i=0, r=1; i<size; i++) r*=x[i];\
  return r;\
}\
type cs_masked_prod_##name(int size, type *x, int *mask)\
{\
  int i;\
  type r;\
  for (i=0, r=1; i<size; i++) if (mask[i]) r*=x[i];\
  return r;\
}\
void cs_sign_##name(int size, int *r,type *x)\
{\
  int i;\
  for (i=0; i<size; i++) r[i] = (x[i]<0)? -1:1;\
}\
void cs_pack_##type(int size,type* r, type* x, int* m)\
{\
  int i,j;\
  for (i=0, j=0; i<size; i++)\
    if (m[i]) r[j++] = x[i];\
}\


C_defs(array,double)
C_defs(iarray,int)

void iarray_not(int size,int* r,int* x)
{
  int i;
  for (i=0; i<size; i++) 
    r[i]=!x[i];
}

binop(&&,and,iarray,int,int);
binop(||,or,iarray,int,int);
binop(%,mod,iarray,int,int);

void cs_enumerate( int size, int  *r, int *x )
{
  int i;
  if (size==0) return;
  r[0] = 0;
  for (i=1; i<size; i++) r[i] = r[i-1] + x[i-1];
}

void cs_abs_iarray(int size, int* r,int* x)
{
  int i;
  for (i=0; i<size; i++) r[i]=abs(x[i]);
}

void cs_abs_array(int size, double* r,double* x)
{
  int i;
  for (i=0; i<size; i++) r[i]=fabs(x[i]);
}

void cs_log_array(int size, double* r,double* x)
{
  int i;
  for (i=0; i<size; i++) r[i]=log(x[i]);
}

void cs_exp_array(int size, double* r,double* x)
{
  int i;
  for (i=0; i<size; i++) r[i]=exp(x[i]);
}



/*
generate a list of species numbers, with each number appearing in the list 
according to the value passed in its position. For example, if x={0,0,1,2,0,1}
then the output from this program will be {2,3,3,5}
*/


void cs_gen_index(int size, int *r, int *x)
{
  int p,j,i;
  for (i=0,p=0; i<size; i++)
    for (j=0; j<x[i]; j++,p++)
      r[p]=i;

}

/* ranking (sort) function */
enum array_dir_t {upwards, downwards};

static int *idata;
static int icmp(const void* ip, const void* jp) 
{
  int i=*(int*)ip, j=*(int*)jp;
  return (idata[i]<idata[j])? -1: (idata[i]>idata[j]);
}
static int ricmp(const void* jp, const void* ip) 
{
  int i=*(int*)ip, j=*(int*)jp;
  return (idata[i]<idata[j])? -1: (idata[i]>idata[j]);
}

void iarray_rank(int size, int *r, int* x, enum array_dir_t dir)
{
  idata=x; 
  //  qsort( r, size, sizeof(int), (dir==upwards)? icmp: ricmp);
  qsort(r, size, sizeof(int), (dir==upwards)? icmp: ricmp);
}

static double *ddata;
static int dcmp(const void* ip, const void* jp) 
{
  int i=*(int*)ip, j=*(int*)jp;
    return (ddata[i]<ddata[j])? -1: (ddata[i]>ddata[j]);
}
static int rdcmp(const void* jp, const void* ip) 
{
  int i=*(int*)ip, j=*(int*)jp;
  return (ddata[i]<ddata[j])? -1: (ddata[i]>ddata[j]);
}

void array_rank(int size, int *r, double* x, enum array_dir_t dir)
{
  ddata=x; 
  qsort( r, size, sizeof(int), (dir==upwards)? dcmp: rdcmp);
}




void array_mul_iarray(int size, double* r, double* x, int* y)
{
  int i; 
  for (i=0; i<size; i++)
    r[i] = x[i] * y[i];
}

void offmul(int size, double* r, double* v, int* rw,  int*c, double* x)
{
  int i;
  for (i=0; i<size; i++) 
    r[ rw[i] ] += v[i] * x[ c[i] ];
}

#if 0
void sparse_mul(int size, double* rv, int *rr, int* rc, double* xv, int* xr, int* xc, double* yv, int *yr, int* yc) 
#endif

/* Rounding function, randomly round up or down, in the range 0..INT_MAX */
int ROUND(double x) 
{
  double dum;
  if (x<0) x=0;
  if (x>INT_MAX-1) x=INT_MAX-1;
  return fabs(modf(x,&dum)) > ((double)rand()/(RAND_MAX+1.0)) ?
	  (int)x+1 : (int)x;
}

void iarray_asg_array_round(int size, int* x, double* y)
{
  int i;
  for (i=0; i<size; i++) 
    x[i] =  ROUND(y[i]); 
}

void iarray_addasg_array_round(int size, int* x, double* y)
{
  int i;
  double r;
  for (i=0; i<size; i++) 
    x[i] =  ROUND(x[i] + y[i]); 
}

void iarray_to_array(int size, double* x, int* y)
{
  int i;
  for (i=0; i<size; i++) x[i]=y[i];
}

void cs_trunc(int size, int *x, double* y)
{
  int i;
  for (i=0; i<size; i++) x[i]=y[i];
}


/* random number support */

/* hook for seed */
void cs_srand(int i) {srand(i);}

/* Uniform random numbers in range 0..1 */

void cs_fillrand(int size, double* x)
{
  int i;
  for (i=0; i<size; x[i++]=rand()/(RAND_MAX+1.0));
}

/*
Fill an array with random numbers distributed according to a Poisson 
distribution
*/

void cs_fillprand(int size, double* x)
{
  int i;
  for (i=0; i<size; x[i++]=-log((rand()+1.0)/(RAND_MAX+1.0)));
}

/*
  generate random numbers that are Gaussian (normally) distributed 
     see Abramowitz and Stegun (1964) sec. 26.8.6.a(2)
*/

float grand()
{
  static float sum=0;
  static unsigned int n=0;
  sum += (float) rand()/RAND_MAX;
  n++;
  return (sum - .5*n) * sqrt( 12./n);
}

void cs_fillgrand(int size, double* x)
{
  int i;
  for (i=0; i<size; x[i++]=grand());
}

#if 0 
void cs_fill_uniq_rand(iarray* x, int n)
{
  n=(n&1)?n-1:n;  /* ensure we have even number */
  {
    int i,j,k, seed[n], tmp;  
  
    
 /* shuffle algorithm */
    for (i=0; i<n; i++) seed[i]=i;
    
    for (i=0; i<n/2; i<<=1)
      for (j=0; j<n; j+=i)
	for (k=0; k<i; k++)
	  if ( (float) rand() / RAND_MAX >= 0.5 )
	  { tmp=seed[j+k]; seed[j+k]=seed[j+k+i]; seed[j+k+i]=tmp; }
    
    for (i=0; i<x->size; i++) x->list[i] = seed[i];
  }
}
#endif

