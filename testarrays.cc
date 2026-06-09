#include <stdio.h>
#include <assert.h>
#include "arrays.h"
#include <stdarg.h>
#include <math.h>

extern "C" void error(char *fmt,...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt,args);
  va_end(args);
  exit(0);
}

void asg(array& x, double y[])
{
  x=array(x.size);
  x=y;
}

void asg(iarray& x, int y[])
{
  x=iarray(x.size);
  x=y;
}

void print(array x)
{
  for (int i=0; i<x.size; i++) printf("%g ",(double)x[i]);
  printf("\n");
}

void print(iarray x)
{
  for (int i=0; i<x.size; printf("%d ",(int)x[i++]));
  printf("\n");
}

int near(double x,double y)
{return (x==y)? 1: fabs(x-y)/fabs(x+y)<1E-4;}

main()
{
  /* test array creation and indexing */
  int i;
  array a(10);
  iarray ia(10);
  int itest[]={1,2,3,4,5,6,7,8,9,10};
  double test[]={1,2,3,4,5,6,7,8,9,10};

  asg(a,test); asg(ia,itest);

  for (i=0; i<10; i++) 
    {
      assert(a[i]==i+1);
      assert(ia[i]==i+1);
      assert(((double*)a)[i]==i+1);
      assert(((int*)ia)[i]==i+1);
    }

  /* test sum */
  assert(sum(ia)==55);
  assert(sum(a)==55.0);
  assert(prod(ia)==3628800);
  assert(prod(a)==3628800.0);
  int ctest[]={0,0,1,0,0,3,0,2,0,0};
  asg(ia,ctest);
  assert(sum(ia)==6);
  

  /* test array equality */
  array b(10);
  iarray ib(10);
  asg(a,test); asg(b,test); b[0]=2; b[5]=7;
  asg(ia,itest); asg(ib,itest); ib[0]=2; ib[5]=7;
  assert(sum(a==a)==10);
  assert(sum(ia==ia)==10);
  assert(sum(a==b)==8);
  assert(sum(ia==ib)==8);
  assert(b[0]==2&&b[5]==7);
  assert(ib[0]==2&&ib[5]==7);

  /* broadcast and copy test */
  {
    array a(10), b;
    iarray ia(10), ib;
    a=2; ia=2;
    b=a; ib=ia;
    a[2]=2; ia[2]=2;
    for (i=0; i<10; i++) assert((double)(a[i])==2);
    for (i=0; i<10; i++) assert((int)(ia[i])==2);
  }
    
  /* test array assignment */

  int ibtest[]={10,9,8,7,4,6,4,3,2,1};
  double btest[]={10,9,8,7,4,6,4,3,2,1};

  b=a;   ib=ia;
  assert( sum(a==b) == 10 && sum(ia==ib) == 10);
  

  /* binary operators */
  array c; iarray ic;
  asg(a,test); asg(ia,itest);
  asg(b,btest); asg(ib,ibtest);

  c=a+b; ic=ia+ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]+b[i] && ic[i]==ia[i]+ib[i] );

  c=a-b; ic=ia-ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]-b[i] && ic[i]==ia[i]-ib[i] );

  c=a*b; ic=ia*ib;
  for (i=0; i<10; i++) assert( c[i]==a[i]*b[i] && ic[i]==ia[i]*ib[i] );

  c=a/b; ic=ia/ib;
  for (i=0; i<10; i++) 
    assert( fabs(c[i]-a[i]/b[i])<1e-15 && ic[i]==ia[i]/ib[i] );

  ic=ia%ib;
  for (i=0; i<10; i++) 
    assert(  ic[i]==ia[i]%ib[i] );
  

  c=a<<b; ic=ia<<ib;
  for (i=0; i<10; i++) assert( c[i]==a[i] && ic[i]==ia[i] );
  for (i=10; i<20; i++) assert( c[i]==b[i-10] && ic[i]==ib[i-10] );

  /* test logical operators */
  
  assert( sum(a<b)==4 && sum(ia<ib)==4);
  assert( sum(a<=b)==5 && sum(ia<=ib)==5);
  assert( sum(a>b)==5 && sum(ia>ib)==5);
  assert( sum(a>=b)==6 && sum(ia>=ib)==6);
  assert( sum(a!=b)==9 && sum(ia!=ib)==9);

  /* test broadcast ops */
  a=5; ia=1;
  assert(sum(a>b)==5 && sum(ia)==10);
  assert(sum(a==5)==10 && sum(ia==1)==10);
  assert(sum(5==a)==10 && sum(1==ia)==10);
  assert(sum(b+5==b+a)==10 && sum(ib+1==ib+ia)==10);
  assert(sum(b-5==b-a)==10 && sum(ib-1==ib-ia)==10);
  assert(sum(b*5==b*a)==10 && sum(ib*1==ib*ia)==10);
  assert(sum(b/5==b/a)==10 && sum(ib/1==ib/ia)==10);
  assert(sum(b<5==b<a)==10 && sum(ib<1==ib<ia)==10);
  assert(sum(b<=5==b<=a)==10 && sum(ib<=1==ib<=ia)==10);
  assert(sum(b>5==b>a)==10 && sum(ib>1==ib>ia)==10);
  assert(sum(b>=5==b>=a)==10 && sum(ib>=1==ib>=ia)==10);
  assert(sum((b!=5)==(b!=a))==10 && sum((ib!=1)==(ib!=ia))==10);
  assert(sum(5+b==a+b)==10 && sum(1+ib==ia+ib)==10);
  assert(sum(5-b==a-b)==10 && sum(1-ib==ia-ib)==10);
  assert(sum(5*b==a*b)==10 && sum(1*ib==ia*ib)==10);
  assert(sum(5/b==a/b)==10 && sum(1/ib==ia/ib)==10);
  assert(sum(5<b==a<b)==10 && sum(1<ib==ia<ib)==10);
  assert(sum(5<=b==a<=b)==10 && sum(1<=ib==ia<=ib)==10);
  assert(sum(5>b==a>b)==10 && sum(1>ib==ia>ib)==10);
  assert(sum(5>=b==a>=b)==10 && sum(1>=ib==ia>=ib)==10);
  assert(sum((5!=b)==(a!=b))==10 && sum((1!=ib)==(ia!=ib))==10);
  a=b<<1; ia=ib<<1;
  assert(a[10]==1 && ia[10]==1);

  /* compound assignment */
  a=array(10); ia=iarray(10);
  asg(a,test); asg(b,test);
  asg(ia,itest); asg(ib,itest);
  c=a; ic=ib;
  c+=b; ic+=ib;
  assert(sum(c==a+b)==10 && sum(ic==ia+ib)==10 );
  c=a; ic=ib;
  c*=b; ic*=ib;
  assert(sum(c==a*b)==10 && sum(ic==ia*ib)==10 );
  c=a; ic=ib;
  c-=b; ic-=ib;
  assert(sum(c==a-b)==10 && sum(ic==ia-ib)==10 );
  c=a; ic=ib;
  c/=b; ic/=ib;
  assert(sum(c==a/b)==10 && sum(ic==ia/ib)==10 );
  c=a; ic=ib;
  c<<=b; ic<<=ib;
  assert(sum(c==a<<b)==20 && sum(ic==ia<<ib)==20);

  c=a; ic=ia;
  c+=2; ic+=2;
  assert(sum(c==a+2)==10 && sum(ic==ia+2)==10 );
  c=a; ic=ia;
  c*=2; ic*=2;
  assert(sum(c==a*2)==10 && sum(ic==ia*2)==10 );
  c=a; ic=ia;
  c-=2; ic-=2;
  assert(sum(c==a-2)==10 && sum(ic==ia-2)==10 );
  c=a; ic=ia;
  c/=2; ic/=2;
  assert(sum(c==a/2)==10 && sum(ic==ia/2)==10 );
  c=a; ic=ia;
  c<<=2; ic<<=2;
  assert(sum(c==a<<2)==11 && sum(ic==ia<<2)==11);

  {
    int iresult[]={10,9,8,7,6,6,7,8,9,10}; iarray ir(10); 
    int ibtest[]={10,9,8,7,6,5,4,3,2,1};
    double btest[]={10,9,8,7,6,5,4,3,2,1};
    ia=itest; ib=ibtest; a=test; b=btest; 
    ic=merge(ia>ib,ia,ib);

    ir=iresult; assert(sum(ir!=ic)==0);
    c=merge(a>b,a,b);
    assert(sum((array)ir!=c)==0);
    assert(max(a,a<b)==5);
    assert(max(a)==10);
    assert(max(ia,ia<ib)==5);
    assert(max(ia)==10);
    assert(min(a,a>b)==6);
    assert(min(a)==1);
    assert(min(ia,ia>ib)==6);
    assert(min(ia)==1);
    assert(sum(a,a<b)==15);
    assert(sum(ia,ia<ib)==15);
    assert(prod(a,a<b)==120);
    assert(prod(ia,ia<ib)==120);
    assert(sum(sign(a-b)!=merge(a>b,1,-1))==0);
    assert(sum(sign(ia-ib)!=merge(ia>ib,1,-1))==0);
    assert(sum(abs(a-b)*sign(a-b)==a-b));
    assert(sum(abs(ia-ib)*sign(ia-ib)==ia-ib));
  }


  /* test array indexing */
  int iresult[]={10,9,3,7,2,5,6,8,4,1}, index_data[]={9,8,2,6,1,4,5,7,3,0};
  double result[]={10,9,3,7,2,5,6,8,4,1};
  iarray index(10);
  c=array(10); ic=iarray(10);

  asg(index,index_data);
  asg(a,test); asg(b,result); asg(ia,itest); asg(ib,iresult);
  c[index]=b; ic[index]=ib;
  assert(sum(a[index]==b)==10 && sum(ia[index]==b)==10 );
  assert(sum(c==a)==10 && sum(ic==ia)==10 );
  
  /* test logicals */
  int ltest[]={0,1,0,1,0,1,0,1,0,1};
  int lresult[]={1,0,1,0,1,0,1,0,1,0};
  asg(ia,ltest); asg(ib,lresult);
  assert(sum(!ia==ib)==10);

  assert(sum((ia&&ia)==ia)==10 && sum((ia&&ib)==0)==10);
  assert(sum((ia||ia)==ia)==10 && sum((ia||ib)==1)==10);

  /* test type conversion */

  asg(a,test); asg(ib,itest); 
  ia=a;
  assert(sum(ia==ib)==10);
  b=ia;
  assert(sum(a==b)==10);
  ia+=a;
  assert(sum(ia==2*ib)==10);
  assert(sum(a*ib==a*b)==10);
  
  /* test comms routines */

  int ipresult[]={2,4,6,8,10};
  ic=iarray(5); c=array(5);
  ib=iarray(10); asg(ib,ltest);
  asg(ia,itest); asg(a,test);
  ic=pack(ia,ib); c=pack(a,ib);
  ib=iarray(5);
  asg(ib,ipresult);
  assert(sum(c==ib)==5 && sum(ic==ib)==5);

  int ieresult[]={0,0,1,1,2,2,3,3,4,4};
  ic=iarray(10); asg(ic,ieresult);
  ia=iarray(10); asg(ia,ltest);
  ib=iarray(10);
  ib=enumerate(ia);
  assert(sum(ib==ic)==10);

  {
    int testx[]={0,0,1,2,0,1}, testr[]={2,3,3,5};
    iarray a(6), b(4);
    asg(a,testx); asg(b,testr);
    assert(sum(gen_index(a)==b)==b.size);
  }

  {
    sparse_mat a(2,2);
    iarray b(2); array c(2);
    a.diag[0]=1; a.diag[1]=2; a.val[0]=1; a.val[1]=2; 
    a.row[0]=0;a.row[1]=1;a.col[0]=1;a.col[1]=0;
    b[0]=1; b[1]=2;
    c[a.row] = a.val * (iarray)(b[a.col]);
    c += a.diag*b;
    assert(sum(a*b == c )==2);
  }

  /* test sorting */
  {
    int testa[]={4,3,5,2};
    double testb[]={4,3,5,2};
    int resultf[]={3,1,0,2};
    int resultr[]={2,0,1,3};
    iarray a(4),r(4); array b(4);
    asg(a,testa); asg(b,testb); asg(r,resultf);
    assert(sum(rank(a)!=r)==0);
    assert(sum(rank(b)!=r)==0);
    asg(r,resultr);
    assert(sum(rank(a,downwards)!=r)==0);
    assert(sum(rank(b,downwards)!=r)==0);
  }

  /* log, exp etc */
  {
    array a(5), b(5), c(5);
    double atest[]={1,2,3,4,5};
    a=atest; b=log(a); c=exp(b);
    for (int i=0; i<5; i++)
      //      printf("%g %g %g %g\n",b[i],log(a[i]),c[i],exp(b[i]));
      { assert(near(b[i],log(a[i]))); assert(near(c[i],exp(b[i])));}
  }

}

