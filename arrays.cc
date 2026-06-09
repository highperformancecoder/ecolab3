#include "arrays.h"

/** submatrix extraction and insertion -- closest thing to full vector
    indexing of sparse matrices -- the latter is too computationally
    expensive, and not needed */

sparse_mat sparse_mat::submat(int min, int max)
{
  sparse_mat r;
  iarray mask;

  /*  
  mask=pcoord(max-min)+min;
  r.diag = diag[mask];
  mask = row >= min && row < max && col >=min && col < max;
  int ntrue = sum(mask);
  r.rowsz=r.colsz=max-min;
  r.val = pack(val, mask, ntrue);
  r.row = pack(row, mask, ntrue)-min;
  r.col = pack(col, mask, ntrue)-min;
  */

  /* tuned version below */
  int i, j, ntrue;
  r.diag=iarray(max-min);
  for (i=min; i<max; i++) r.diag[i-min]=diag[i];
  for (ntrue=i=0; i<row.size; i++)
    if (row[i] >= min && row[i] < max && col[i] >=min && col[i] < max)
      ntrue++;
  r.row=iarray(ntrue); r.col=iarray(ntrue); r.val=array(ntrue);
  r.rowsz=r.colsz=max-min;
  for (j=i=0; i<row.size; i++)
    if (row[i] >= min && row[i] < max && col[i] >=min && col[i] < max)
      {
	r.row[j]=row[i]-min;
	r.col[j]=col[i]-min;
	r.val[j]=val[i];
	j++;
      }
  assert(sum(r.row>=r.diag.size)==0 && sum(r.col>=r.diag.size)==0);
  return r;
}

/** block diagonal submatrix insertion */

void sparse_mat::insert(sparse_mat x, int where, int old_size)
{
  assert(sum(x.row>=x.diag.size)==0 && sum(x.col>=x.diag.size)==0);
  iarray slice1, slice2, mask;

  slice1 = pcoord(where); 
  slice2 = pcoord(diag.size-where-old_size) + where + old_size;

  diag = diag[slice1] << x.diag << diag[slice2];

  /* cut out old submatrix */
  slice1 = row < where &&  col < where;
  slice2 = row >= where + old_size && col >= where + old_size;
  int adjust = x.diag.size - old_size; 
  int ntrue1 = sum(slice1);
  int ntrue2 = sum(slice2);

  val = pack(val,slice1,ntrue1) << pack(val,slice2,ntrue2) << x.val;
  row = pack(row,slice1,ntrue1) << pack(row,slice2,ntrue2)+adjust <<
    x.row+where;
  col = pack(col,slice1,ntrue1) << pack(col,slice2,ntrue2)+adjust <<
    x.col+where;
}

void iarray::print() 
{for (int i=0; i<size; i++) cout << *((double*)list+i) << ' ';}
void array::print() 
{for (int i=0; i<size; i++) cout << *((int*)list+i) << ' ';}

void lgspread( array& a, array s )
{
  array gran(a.size);
  fillgrand(gran);
  //  a *= 1.0 + s*gran;
  a = (array)sign(a)*exp(log(abs(a))+s*gran);
}

void gspread( array& a, array s )
{
  array gran(a.size);
  fillgrand(gran);
  a += s*gran;
}

void put(array x, FILE *f)
{
  fwrite(&x.size,sizeof(int),1,f);
  fwrite((double*) x,sizeof(double),x.size,f);
}

void put(iarray x, FILE *f)
{  
  fwrite(&x.size,sizeof(int),1,f);
  fwrite((int*) x,sizeof(int),x.size,f);
}

void put(sparse_mat x, FILE *f)
{
   fwrite(&x.diag.size,sizeof(int),1,f);
   fwrite(&x.val.size,sizeof(int),1,f);
   fwrite((double*) x.diag,sizeof(double),x.diag.size,f);
   fwrite((double*) x.val,sizeof(double),x.val.size,f);
   fwrite((int*) x.col,sizeof(int),x.col.size,f);
   fwrite((int*) x.col,sizeof(int),x.row.size,f);
} 

void get(array& x, FILE *f)
{
  int n;
  fread(&n,sizeof(int),1,f);
  x=array(n);
  fread((double*)x,sizeof(double),n,f);
}

void get(iarray& x, FILE *f)
{
  int n;
  fread(&n,sizeof(int),1,f);
  x=iarray(n);
  fread((int*)x,sizeof(int),n,f);
}

void get(sparse_mat& x, FILE *f)
{
  int n, noff;
  fread(&n,sizeof(int),1,f);
  fread(&noff,sizeof(int),1,f);
  x.diag=array(n);
  x.val=array(noff);
  x.row=iarray(noff);
  x.col=iarray(noff);
  fread((double*)x.diag,sizeof(double),n,f);
  fread((double*)x.val,sizeof(double),noff,f);
  fread((int*)x.row,sizeof(int),noff,f);
  fread((int*)x.col,sizeof(int),noff,f);
}
