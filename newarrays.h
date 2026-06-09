/*
  Class implementation for arrays used in ecolab.
*/

/*
The following classes should be considered as "windows" on the real
data. More than one instance of a variable can refer to the same
data. When a variable is created without initialisation, a new data
item is created. Counters are kept with the data of the number of
variables referring to the data at any one time. When the last
variable is destroyed, then the data is destroyed also. This
arrangement allows copying and assignment to handled by a simple copy
of pointers.
 */


         /***** helper classes for indexing *****/

#ifdef CONTIGUOUS  /* optimized for serial computers */

#define INDEX_OP(type) type& operator[](int);

#define INDEX_OP_DEF(type,name) inline type& name::operator[](int i)\
{\
assert(i<size); \
     if (*cntr>1)   /* make a copy if more than 1 referrant */\
       {\
	 void *ptr_list=list;\
	 *this = name(size);\
	 copy_##type(size,list,ptr_list);\
       }\
return ((type*)list)[i];\
}

#else

#define INDEX_OP(type) par_addr_##type operator[](int); 

#define INDEX_OP_DEF(type,name)  \
inline par_addr_##type name::operator[](int i)\
{\
   par_addr_##type r;\
   assert(i<size);\
   if (*cntr>1)   /* make a copy if more than 1 referrant */\
       {\
	 void *ptr_list=list;\
	 *this = name(size);\
	 copy_##type(size,list,ptr_list);\
       }\
   r.ptr=this; \
   r.offset=i; \
   return r;\
}\
inline par_addr_##type name::operator[](par_addr_int i) \
  {return (*this)[(int)i];}
#endif

#define Helper_Classes(name,type)\
class par_addr_##type\
{\
  friend class name;\
  name *ptr;\
  int offset;\
 public:\
  operator type();\
  type operator= (type x); \
  type operator= (par_addr_##type x); \
};\
class par_addr_##name \
{\
  friend class name;\
  name *ptr;\
  iarray ilist;\
 public:\
  operator name();\
  name operator= (name x); \
  name operator= (par_addr_##name x); \
  name operator= (type x); \
};\

#define Private_Declarations(name,type)\
  friend par_addr_##type;\
  friend par_addr_##name;\
  int *cntr;\

#define binop(op, opname, rettype, name)\
  rettype operator op (name x) \
   {\
      assert(size==x.size);\
      rettype r(size); \
      name##_##opname(size,r.list,list,x.list); \
      return r;\
   }\

#define broadop(op, rettype, name, type)\
  rettype operator op (type x) \
   {\
      name r(size);\
      r=x;\
      return (*this) op r;\
   } \

#define compop(op,  name, type)\
  name operator op##= (name x) \
   {return operator = ((*this) op x);} \
  name operator op##= (type x) \
   {return (*this) = (*this) op x;} \

#define Public_Declarations(name,type)\
  int size;\
  void *list;\
  /*name() {size=0; list=new_##name(0); cntr=new int; *cntr=1;} */\
  name(int s=0) {size=s; list=new_##name(s); cntr=new int; *cntr=1;}\
  name(const name& x) {size=x.size; list=x.list; cntr=x.cntr; ++(*cntr);}\
  ~name() {if (--(*cntr)==0) {delete_##name(list); delete cntr;}}\
  name& operator=(name x)\
    {\
      if (--(*cntr)==0) {delete_##name(list); delete cntr;}\
      size=x.size; list=x.list; cntr=x.cntr;\
      ++(*cntr); return (*this);\
    }\
             /***** binary operators - << means concatenation ******/\
  operator type*() {return cs_getlist_##name(list);}\
  name& operator=(type * x) {cs_putlist_##name(list,x,size); return *this;}\
  binop(+,plus,name,name)\
  binop(-,minus,name,name)\
  binop(*,mul,name,name)\
  binop(/,div,name,name)\
  binop(<,lt,iarray,name)\
  binop(<=,le,iarray,name)\
  binop(>,gt,iarray,name)\
  binop(>=,ge,iarray,name)\
  binop(==,eq,iarray,name)\
  binop(!=,ne,iarray,name)\
  name operator << (name x) \
   {name r(size+x.size); name##_cat(r.list,size,list,x.size,x.list); return r;} \
             /***** broadcast operators *****/ \
  name& operator=(type x) \
    {if (size>0) {name r(size); broadcast_##type(size,r.list,x); (*this)=r;} \
     return *this;}\
  broadop(+,name,name,type)\
  broadop(-,name,name,type)\
  broadop(*,name,name,type)\
  broadop(/,name,name,type)\
  broadop(<,iarray,name,type)\
  broadop(<=,iarray,name,type)\
  broadop(>,iarray,name,type)\
  broadop(>=,iarray,name,type)\
  broadop(==,iarray,name,type)\
  broadop(!=,iarray,name,type)\
  name operator << (type x) \
   {name t(1); t=x; return (*this) << t;} \
           /***** compound assignment operators *****/ \
  compop(+,name,type)\
  compop(-,name,type)\
  compop(*,name,type)\
  compop(/,name,type)\
  compop(<<,name,type)\
           /***** index operators *****/\
  INDEX_OP(type)\
  par_addr_##type operator[](par_addr_int);\
  par_addr_##name operator[](iarray);\
  par_addr_##name operator[](par_addr_iarray);\
  name operator-() {return this->operator*((type)-1);}\
  void print();\

/* Implementations part must follow definition of iarray */
#define Implementations(name,type)\
   /* extra broadcast ops to satisfy reverse order of operands */\
inline name operator+(type x,name y) {return y+x;} \
inline name operator-(type x,name y) {name z(y.size);z=x;return z-y;} \
inline name operator*(type x,name y) {return y*x;} \
inline name operator/(type x,name y) {name z(y.size);z=x;return z/y;} \
inline name operator<(type x,name y) {return y>x;} \
inline name operator<=(type x,name y) {return y>=x;} \
inline name operator>(type x,name y) {return y<x;} \
inline name operator>=(type x,name y) {return y<=x;} \
inline name operator==(type x,name y) {return y==x;} \
inline name operator!=(type x,name y) {return y!=x;} \
inline par_addr_##type::operator type() {return get_##type(ptr->list,offset);}\
inline type par_addr_##type::operator= (type x) \
   { put_##type(ptr->list,offset,x); return x; }\
inline type par_addr_##type::operator= (par_addr_##type x) \
{return (*this)=(type)x;} \
inline par_addr_##name::operator name()\
{\
   name r(ilist.size);\
   get_##type##_array(ilist.size,r.list,ptr->list,ilist.list);\
   return r;\
}\
inline name par_addr_##name::operator=(name x)\
{\
   assert(ilist.size==x.size);\
   name t;\
   if (ptr->list==x.list) /* ensure pure permutations do not clobber */\
     {t=name(x.size); copy_##type(x.size,t.list,x.list);}\
   else\
     {t=x;}\
   put_##type##_array(ilist.size,ptr->list,ilist.list,x.list); \
   return t;\
}\
inline name par_addr_##name::operator=(par_addr_##name x)\
{ return (*this) = (name)x;}\
INDEX_OP_DEF(type,name)\
inline par_addr_##name name::operator[](iarray i)\
{\
   assert(sum(i>=size)==0);\
   par_addr_##name r;\
   if (*cntr>1)   /* make a copy if more than 1 referrant */\
       {\
	 void *ptr_list=list;\
	 *this = name(size);\
	 copy_##type(size,list,ptr_list);\
       }\
   r.ptr=this; \
   r.ilist = i;\
   return r;\
}\
inline name par_addr_##name::operator=(type x)\
{\
   name xtmp(ilist.size);\
   broadcast_##type(ilist.size,xtmp.list,x);   \
   put_##type##_array(ilist.size,ptr->list,ilist.list,xtmp.list); \
   return xtmp;\
}\
inline par_addr_##name name::operator[](par_addr_iarray i) \
  {return (*this)[(iarray)i];}\


class iarray;
class array;
class par_addr_int;
class par_addr_double;
class par_addr_iarray;
class par_addr_array;

inline int sum(iarray x);


class iarray
{
  Private_Declarations(iarray,int)
 public:
  Public_Declarations(iarray,int)
  iarray operator ! () {iarray r(size); iarray_not(size,r.list,list); return r;}
  binop(%,mod,iarray,iarray);
  broadop(%,iarray,iarray,int);
  compop(%,iarray,int);
  binop(&&,and,iarray,iarray);
  binop(||,or,iarray,iarray);
  iarray operator&=(iarray x) {return *this = *this && x;}
  iarray operator|=(iarray x) {return *this = *this || x;}
  void operator=(array x);     /* probabilistic assignment */
  void operator+=(array x);    /* probabilistic assignment */
  inline operator array();
};

class array
{

  Private_Declarations(array,double)
 public:

  Public_Declarations(array,double)
  inline array operator*(iarray);
  array operator=(int x){operator=((double)x); return *this;}
  operator iarray() 
    {iarray r(size); cs_trunc(size,r.list,list); return r;}
};  

Helper_Classes(iarray,int)
Helper_Classes(array,double)

class sparse_mat
{
 public:
  /* row and column dimension of matrix (used if diag.size==0) */
  int rowsz, colsz;
  /*diagonal and values of offdiagonal*/
  array diag, val;
  /*row and columns of offdiagonal elements*/
  iarray row, col;
  sparse_mat(int s=0, int o=0) 
    {rowsz=colsz=s; diag=array(s); val=array(o); row=iarray(o); col=iarray(o);}
  sparse_mat(const sparse_mat& x) 
    {
      rowsz=x.rowsz; colsz=x.colsz; 
      diag=x.diag; val=x.val; 
      row=x.row; col=x.col;
    }
  /*matrix multiplication*/
  array operator*(array x)  
    {
      array r(colsz);
      assert(row.size==col.size && row.size==val.size);      
      if (diag.size>0)
	{
	  assert(diag.size==x.size);
	  r = diag*x; 
	}
      offmul(val.size,r.list,val.list,row.list,col.list,x.list);
      return r;
    }
  /* not quite sure how to do this yet!
  sparse_mat operator*(sparse_mat x)
    {
      */
  /*assignment*/
  sparse_mat& operator=(const sparse_mat& x) 
    {
      rowsz=x.rowsz; colsz=x.colsz; 
      diag=x.diag; val=x.val; 
      row=x.row; col=x.col; 
      return *this;
    }
  /*return the submatrix bounded by #min# and #max#*/
  sparse_mat submat(int min, int max);
  /* submatrix extraction and insertion */
  void insert(sparse_mat x, int where, int old_size);
};

/****** Implementations Section *******/

Implementations(iarray,int)
Implementations(array,double)

 /* probabilistic assignment */
inline  void iarray::operator=(array x)    
{
  *this = iarray(x.size); 
  assert(x.size==size);
  iarray_asg_array_round(size,list,x.list);
}

inline  void iarray::operator+=(array x)   
{
  assert(x.size==size);
  iarray_addasg_array_round(size,list,x.list);
}

inline iarray::operator array() 
{
  array r(size); 
  iarray_to_array(size,r.list,list); 
  return r;
}
 
inline array array::operator*(iarray x) 
{ 
  array result(size);
  assert(x.size==size);
  array_mul_iarray(size,result.list, list, x.list);
  return result;
}

inline sparse_mat tr(sparse_mat x)
{
  sparse_mat r=x;
  r.row=x.col; r.rowsz=x.colsz;
  r.col=x.row; r.colsz=x.rowsz;
}

/* extra methods to help indexed expressions */
#define par_addr_extra_binop(type,op) \
  inline type operator op(par_addr_##type x, type y){return (type)x op y;} \
  inline type operator op(type y, par_addr_##type x){return y op (type)x;} \
  inline type operator op(par_addr_##type y, par_addr_##type x) \
            {return (type) y op (type)x;} 

#define par_addr_extra_compop(type,op) \
  inline type operator op(par_addr_##type x, type y) \
   {type t; t = (type)x; t op y; x=t; return t;}\
  inline type operator op(type& x, par_addr_##type y) {return x op(type)y;}\
  inline type operator op(par_addr_##type x, par_addr_##type y) \
   {return  x op (type)y;}

#define par_addr_extra_logop(type,op) \
  inline int operator op(par_addr_##type x, type y){return (type)x op y;} \
  inline int operator op(type y, par_addr_##type x){return y op (type)x;} \
  inline int operator op(par_addr_##type y, par_addr_##type x)\
   {return (type) y op (type)x;} 

#define par_addr_extra_binop_array(name,type,op) \
  inline name operator op(par_addr_##name x, type y){return (name)x op y;}\
  inline name operator op(type y, par_addr_##name x){return y op (name)x;}\
  inline name operator op(par_addr_##name x, par_addr_##type y)\
    {return (name)x op (type)y;}\
  inline name operator op(par_addr_##type y, par_addr_##name x)\
    {return (type)y op (name)x;}\
  inline name operator op(par_addr_##name x, name y){return (name)x op y;}\
  inline name operator op(par_addr_##name y, par_addr_##name x) \
            {return y op (name)x;} \

#define par_addr_extra_catop_array(name,type,op) \
  inline name operator op(par_addr_##name x, type y){return (name)x op y;}\
  inline name operator op(par_addr_##name x, par_addr_##type y)\
    {return (name)x op (type)y;}\
  inline name operator op(par_addr_##name x, name y){return (name)x op y;}\
  inline name operator op(par_addr_##name y, par_addr_##name x) \
            {return y op (name)x;} \

#define par_addr_extra_compop_array(name,type,op) \
  inline name operator op(par_addr_##name x, type y) \
   {name t; t = (name)x; t op y; x=t; return t;}\
  inline name operator op(par_addr_##name x, par_addr_##type y) \
   {name t; t = (name)x; t op (type)y; x=t; return t;}\
  inline name operator op(par_addr_##name x, name y) \
   {name t; t = (name)x; t op y; x=t; return t;}\
  inline name operator op(name& x, par_addr_##name y) {return x op(name)y;}\
  inline name operator op(par_addr_##name& x, par_addr_##name y) \
   {return  x op (name)y;}\

#define par_addr_extra_logop_array(name,type,op) \
  inline iarray operator op(par_addr_##name x, name y){return (name)x op y;} \
  inline iarray operator op(name y, par_addr_##name x){return y op (name)x;} \
  inline iarray operator op(par_addr_##name x, type y){return (name)x op y;} \
  inline iarray operator op(type y, par_addr_##name x){return y op (name)x;} \
  inline iarray operator op(par_addr_##name x, par_addr_##type y)\
       {return (name)x op (type)y;} \
  inline iarray operator op(par_addr_##type y, par_addr_##name x)\
       {return (type) y op (name)x;} \
  inline iarray operator op(par_addr_##name y, par_addr_##name x) \
   {return (name) y op (name)x;} \

#define par_addr_extra_ops(type)\
  par_addr_extra_binop(type,+) \
  par_addr_extra_binop(type,-) \
  par_addr_extra_binop(type,*) \
  par_addr_extra_binop(type,/) \
  par_addr_extra_compop(type,+=) \
  par_addr_extra_compop(type,-=) \
  par_addr_extra_compop(type,*=) \
  par_addr_extra_compop(type,/=) \
  inline type operator++(par_addr_##type x,int) {type r; r=x; x+=1; return r;}\
  inline type operator++(par_addr_##type x) {return x+=1;}\
  inline type operator--(par_addr_##type x,int) {type r; r=x; x-=1; return r;}\
  inline type operator--(par_addr_##type x) {return x-=1;}\
  par_addr_extra_logop(type,<) \
  par_addr_extra_logop(type,<=) \
  par_addr_extra_logop(type,>) \
  par_addr_extra_logop(type,>=) \
  par_addr_extra_logop(type,==) \
  par_addr_extra_logop(type,!=) \

#define par_addr_extra_ops_array(name,type)\
  inline name operator-(par_addr_##name x) {return -(name)x;}\
  par_addr_extra_binop_array(name,type,+) \
  par_addr_extra_binop_array(name,type,-) \
  par_addr_extra_binop_array(name,type,*) \
  par_addr_extra_binop_array(name,type,/) \
  par_addr_extra_catop_array(name,type,<<) \
  par_addr_extra_compop_array(name,type,+=) \
  par_addr_extra_compop_array(name,type,-=) \
  par_addr_extra_compop_array(name,type,*=) \
  par_addr_extra_compop_array(name,type,/=) \
  par_addr_extra_compop_array(name,type,<<=) \
  inline name operator++(par_addr_##name x,int) {name r; r=x; x+=1; return r;}\
  inline name operator++(par_addr_##name x) {return x+=1;}\
  inline name operator--(par_addr_##name x,int) {name r; r=x; x-=1; return r;}\
  inline name operator--(par_addr_##name x) {return x-=1;}\
  par_addr_extra_logop_array(name,type,<) \
  par_addr_extra_logop_array(name,type,<=) \
  par_addr_extra_logop_array(name,type,>) \
  par_addr_extra_logop_array(name,type,>=) \
  par_addr_extra_logop_array(name,type,==) \
  par_addr_extra_logop_array(name,type,!=) \

par_addr_extra_ops(int)
par_addr_extra_ops(double)

par_addr_extra_ops_array(iarray,int)
par_addr_extra_ops_array(array,double)

/* assume that r has been declared already with enough space */
inline iarray pack( iarray x, iarray mask, int ntrue=-1) 
{
  assert(x.size==mask.size);
  iarray r( (ntrue==-1)? sum(mask): ntrue );
  cs_pack_int(x.size,r.list,x.list,mask.list);
  return r;
}
inline iarray pack( int x, iarray mask, int ntrue=-1) 
{
  iarray r( (ntrue==-1)? sum(mask): ntrue );
  iarray xx(mask.size); xx = x;
  cs_pack_int(mask.size,r.list,xx.list,mask.list);
  return r;
}
inline array pack( array x, iarray mask, int ntrue=-1) 
{
  assert(x.size==mask.size); assert(ntrue==-1||sum(mask)==ntrue);
  array r( (ntrue==-1)? sum(mask): ntrue );
  cs_pack_double(mask.size,r.list,x.list,mask.list);
  return r;
}
inline iarray enumerate(iarray mask) 
{iarray r(mask.size); cs_enumerate(mask.size,r.list,mask.list); return r;}


inline iarray pcoord(int size)
{
  iarray r(size);
  r=1;
  return enumerate(r);
}

#define MERGE(name,type)\
inline name merge(iarray mask, name x, name y)\
{\
  assert(x.size==y.size && x.size==mask.size);\
  name r(mask.size);\
  cs_merge_##type(mask.size,r.list,mask.list,x.list,y.list);\
  return r;\
}\
inline name merge(iarray mask, type x, name y)\
{\
  assert(y.size==mask.size);\
  name r(mask.size), xx(mask.size);\
  xx=x;\
  cs_merge_##type(mask.size,r.list,mask.list,xx.list,y.list);\
  return r;\
}\
inline name merge(iarray mask, name x, type y)\
{\
  assert(x.size==mask.size);\
  name r(mask.size), yy(mask.size);\
  yy=y;\
  cs_merge_##type(mask.size,r.list,mask.list,x.list,yy.list);\
  return r;\
}\
inline name merge(iarray mask, type x, type y)\
{\
  name r(mask.size), xx(mask.size), yy(mask.size);\
  xx=x; yy=y;\
  cs_merge_##type(mask.size,r.list,mask.list,xx.list,yy.list);\
  return r;\
}

#define MAX(name,type) inline type max(name x) \
  {assert(x.size>0); return cs_max_##name(x.size,x.list);}\
inline type max(name x,iarray mask) \
  {assert(x.size>0 && mask.size==x.size); \
   return cs_masked_max_##name(x.size,x.list,mask.list);}
#define MIN(name,type) inline type min(name x) \
  {assert(x.size>0); return cs_min_##name(x.size,x.list);}\
inline type min(name x,iarray mask) \
  {assert(x.size>0 && mask.size==x.size); \
   return cs_masked_min_##name(x.size,x.list,mask.list);}
#define SUM(name,type) inline type sum(name x) \
  {if (x.size==0) return 0; else return cs_sum_##name(x.size,x.list);}\
inline type sum(name x,iarray mask) \
  {assert(mask.size==x.size); \
   if (x.size==0) return 0; else return cs_masked_sum_##name(x.size,x.list,mask.list);}
#define PROD(name,type) inline type prod(name x) \
  {if (x.size==0) return 0; else return cs_prod_##name(x.size,x.list);}\
inline type prod(name x,iarray mask) \
  {assert(mask.size==x.size); \
   if (x.size==0) return 0; else return cs_masked_prod_##name(x.size,x.list,mask.list);}
#define ABS(name,type) inline name abs(name x) \
  {array r(x.size); cs_abs_##name(x.size,r.list,x.list); return r;}
#define SIGN(name,type) inline iarray sign(name x) \
  {iarray r(x.size); cs_sign_##name(x.size,r.list,x.list); return r;}

#define MATH_FUNCS(name,type)\
MERGE(name,type)\
MAX(name,type)\
MIN(name,type)\
SUM(name,type)\
PROD(name,type)\
ABS(name,type)\
SIGN(name,type)

MATH_FUNCS(iarray,int)
MATH_FUNCS(array,double)

inline array log(iarray x) 
{array r(x.size); array xx; xx=x; cs_log_array(x.size,r.list,xx.list); return r;}

inline array exp(iarray x) 
{array r(x.size); array xx; xx=x; cs_exp_array(x.size,r.list,xx.list); return r;}

inline array log(array x) 
{array r(x.size); cs_log_array(x.size,r.list,x.list); return r;}

inline array exp(array x) 
{array r(x.size); cs_exp_array(x.size,r.list,x.list); return r;}

/*
generate a list of species numbers, with each number appearing in the list 
according to the value passed in its position. For example, if x={0,0,1,2,0,1}
then the output from this program will be {3,4,4,6}
*/

inline iarray gen_index(iarray x) 
{
  iarray result(sum(x)); 
  cs_gen_index(x.size,result.list,x.list);
  return result;
};

inline iarray rank(iarray x, enum array_dir_t dir=upwards)
{
  iarray result=pcoord(x.size);
  iarray_rank(x.size,result.list,x.list,dir);
  return result;
}

inline iarray rank(array x, enum array_dir_t dir=upwards)
{
  iarray result=pcoord(x.size);
  array_rank(x.size,result.list,x.list,dir);
  return result;
}

inline void fillrand(array& x) {cs_fillrand(x.size,x.list);}  
inline void fillprand(array& x) {cs_fillprand(x.size,x.list);}  
inline void fillgrand(array& x) {cs_fillgrand(x.size,x.list);}
/*inline void fill_uniq_rand(iarray& x,int n) {cs_fill_uniq_rand(x.list,n);}*/

/* 
Vary components according to a Gaussian distribution, with the given
  std deviation 
*/


/* stream support */

inline ostream& operator<<(ostream& s, array x)
{
  for (int i=0; i<x.size; i++) s << (double) x[i] <<' ';
  return s;
}

inline ostream& operator<<(ostream& s, iarray x)
{
  for (int i=0; i<x.size; i++) s << (int) x[i]<<' ';
  return s;
}

inline ostream& operator<<(ostream& s, par_addr_double x)
{  return s << (double) x; }

inline ostream& operator<<(ostream& s, par_addr_int x)
{return s << (int) x;}

void put(iarray,FILE*);
void put(array,FILE*);
void put(sparse_mat,FILE*);
void get(iarray&,FILE*);
void get(array&,FILE*);
void get(sparse_mat&,FILE*);
int initialize(iarray&, char *);
int initialize(array&, char *);
int initialize_offdiag(sparse_mat&, char *, int);
void lgspread( array& a, array s );
void gspread( array& a, array s );
