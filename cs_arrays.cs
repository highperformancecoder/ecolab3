/* serial emulation of C* support */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cscomm.h>
typedef struct{ int size; shape *sh; double:void *list} array;
typedef struct{ int size; shape *sh; int:void *list} iarray;
#ifndef RAND_MAX
#define RAND_MAX  2147483647  /* This is missing from <stdlib.h>? */
#endif

/* array support */

#include "cs_arrays_defs.exh"

void iarray_not(iarray*r,iarray*x)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list=!*x->list;
}

void iarray_and(iarray*r,iarray*x,iarray*y)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list= *x->list && *y->list;
}

void iarray_or(iarray*r,iarray*x,iarray*y)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list= *x->list || *y->list;
}

int cs_count(iarray *x)
{
  if (x->size==0) return 0;
  with (*x->sh)
    return += *x->list;
}

void cs_pack_int(iarray* r, iarray* x, iarray* m)
{
  if (r->size==0) return;
  with (*r->sh)
    where (*m->list)
      [enumerate(0,CMC_upward,CMC_exclusive,CMC_none,CMC_no_field)]
	*r->list = *x->list;
}

void cs_pack_double(array* r, array* x, iarray* m)
{
  if (r->size==0) return;
  with (*r->sh)
    where (*m->list)
      [enumerate(0,CMC_upward,CMC_exclusive,CMC_none,CMC_no_field)]
	*r->list = *x->list;
}

void cs_enumerate( iarray  *r, iarray *x )
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list = scan(*x->list,0,CMC_combiner_add,CMC_upward,
		    CMC_none,CMC_no_field,CMC_exclusive);
}

void cs_merge_int(iarray* r, iarray* m, iarray *x, iarray* y)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list = *m->list * *x->list + !*m->list * *y->list; 
}

void cs_abs_array(array*r,array*x)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list = abs(*x->list); 
}

/*
generate a list of species numbers, with each number appearing in the list 
according to the value passed in its position. For example, if x={0,0,1,2,0,1}
then the output from this program will be {2,3,3,5}
*/


void cs_gen_index(iarray *r, iarray *x)
{
  int p,j,i;
  for (i=0,p=0; i<x->size; i++)
    for (j=0; j<[i]*x->list; j++,p++)
      [p]*r->list=i;

}
void array_mul_iarray(array* r, array* x, iarray*y)
{
  if (r->size==0) return;
  with (*r->sh)
    *r->list = *x->list * *y->list;
}

void offmul(array*r, array* v, iarray* rw,  iarray*c, iarray* x)
{
  if (v->size==0) return;
  with (*v->sh)
    [ *rw->list ]*r->list += *v->list * [ *c->list ]*x->list;
}

/* Rounding function, randomly round up or down */
int:current ROUND(double:current x) 
{
  double:current dum;
 int:current mask;
  mask = fabs(modf(x,&dum)) > ((double:current)prand()/RAND_MAX);
  return  (int:current)x + mask * (2*(x>0)-1);
}

void iarray_asg_array_round(iarray* x,array* y)
{
  int i;
  if (x->size==0) return;
  with (*x->sh)
    {
      *x->list =  ROUND(*y->list); 
      *x->list *= (*x->list<0);
    }
}

void iarray_addasg_array_round(iarray* x,array* y)
{
  if (x->size==0) return;
  with (*x->sh)
    {
      *x->list +=  ROUND(*y->list); 
      *x->list *= (*x->list<0);
    }
}

void iarray_to_array(array *x, iarray *y)
{
  if (x->size==0) return;
  with (*x->sh) *x->list = *y->list;
}

/* random number support */

/* hook for seed */
void cs_srand(int i) {psrand(i);}

/* Uniform random numbers in range 0..1 */

void cs_fillrand(array* x)
{
  if (x->size==0) return;
  with (*x->sh)
    *x->list=(double:current)prand()/RAND_MAX;
}

/*
Fill an array with random numbers distributed according to a Poisson 
distribution
*/

void cs_fillprand(array* x)
{
  if (x->size==0) return;
  with (*x->sh)
  *x->list = -log((double:current)prand()/RAND_MAX);
}

/*
  generate random numbers that are Gaussian (normally) distributed 
     see Abramowitz and Stegun (1964) sec. 26.8.6.a(2)
*/

double:current grand()
{
  static double sum=0;
  static unsigned int n=0;
  double:current result;
  int:current nn;
  nn = n + pcoord(0) + 1;
  result =  scan( (double:current) prand()/RAND_MAX, 0, CMC_combiner_add, 
       CMC_upward,CMC_none, CMC_no_field, CMC_inclusive) + sum;   
  sum = [positionsof(current)-1] result;
  n+=positionsof(current);
  return (result - .5*nn) * sqrt(12./nn);
}

void cs_fillgrand(array* x)
{
  if (x->size==0) return;
  with (*x->sh) *x->list=grand();
}

