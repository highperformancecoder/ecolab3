#include <strstream.h>
#include <float.h>
#include <string.h>
#include "arrays.h"
#include "tcl++.h"
#include "globals.h"
#include <hash_map>
#include <new>

char *index_name(int i)
{
  int j,k;
  tclindex idx;
  ostrstream iname;
  static char *iname_str=NULL;

  assert(dims.size>0 || dims.size==0 && i==0);
  if (dims.size==0) 
    iname << 0;
  else
    for (j=1, k=dims[0], iname << i%dims[0]; j<dims.size; k*=dims[j++])
      iname << ',' << (i/k)%dims[j];

  iname << ends;
  iname_str = (char*) realloc(iname_str,strlen(iname.str())+1);
  strcpy(iname_str,iname.str());
  return iname_str;
}

/* Which cell does species i correspond to? */
int which_cell(int i)
{
  int j, cell;
  tclindex idx;
  if (ncells==1) return 0;
  for ( cell=0, j=0; cell<ncells; cell++ )
    {
      j+=nsp[cell];
      if (i<j) return cell;
    }
  return cell;
} 


/* This code is a workaround for a bug conflict between gethostbyname
and libstdc++ on Linux systems. It only consults /etc/hosts. Feel free
to comment out if you have working gethostbyname already */

#if 0 /*__linux__ */
#include <netdb.h>
extern "C" struct hostent *gethostbyname(const char *name)
{
  static struct hostent r;
  static char *aliases[]={0};
  static char addr[]={127,0,0,1};
  static char *addresses[]={addr};
  FILE *hosts=fopen("/etc/hosts","r");
  char buffer[1024];
  while (fgets(buffer,1024,hosts)!=NULL)
    if (strstr(buffer,name)!=NULL)
      {
	for (int i=0; i<4; i++)
	  addr[i]=atoi(strtok( (i==0)?buffer: NULL, "."));
	printf("resolved %s as %d.%d.%d.%d\n",name,(unsigned char)addr[0],(unsigned char)addr[1],(unsigned char)addr[2],(unsigned char)addr[3]);
	break;
      }
  r.h_name=name;
  r.h_aliases=aliases;
  r.h_addrtype=2;
  r.h_length=4;
  r.h_addr_list=addresses;
  return &r;
}
#endif


#ifdef MEMDEBUG  /* use these new/delete replacements for tracking
down memory leaks */

#undef malloc
#undef free
static int memused=0;

class hash<void *>
{
  hash<unsigned> hasher;
public:
  size_t operator()(void *x){return hasher((unsigned)x);}
};

/* warning, warning! do not mix this Realloc with malloc and free! Use
   realloc(NULL,sz) as a replacement for malloc(sz), and
   realloc(ptr,0) as a replacement for free(ptr). */


char *Realloc(char *p, size_t sz)
{
  static hash_map<void *,int> sizes;
  char *r;
  if (sz==0)   /* equivalent to free(p); */
    r=NULL;
  else
    {
      if (sizes.count(p) && sizes[p]>=sz) return p; /* nothing needed */
      //      r = new char[sz];   /* new should not return NULL */
      r=malloc(sz);
      sizes[r]=sz;
      if (p!=NULL) memcpy(r,p,sz);
    }
  memused += sz;
  if (sizes.count(p)) 
    {
      memused -= sizes[p];
      sizes.erase(p);
      //      delete [] p;
      free(p);
    }
  return r;
}

#include <sys/types.h>
static int m=-1, maxm=0;

void *operator new(size_t s)
{
  void *r;
  //  if (m++>maxm) {maxm=m; printf("alloc: %d\n",m);}  
  r=Realloc(NULL,s); 
  if (memused>maxm)
    {
      cout << "memused= "<<memused/1024<<" Kbytes\n";
      maxm=memused;
    }
  //printf("new: %x\n",r=ckalloc(s));
  if (r==NULL) throw bad_alloc();
  assert(r!=NULL);
  return r;
}

void operator delete(void *p)
{
  //  if (p!=NULL) m--; //printf("delete: %d\n",m--);
  //  if (p!=NULL) printf("delete: %x\n",p);
  //free(p);
  Realloc(p,0);
}
#endif

//#ifdef sgi
//extern "C" void __assert(const char *, const char *, int);
//void _assert(const char *a, const char *b, int c)
//{__assert(a,b,c);}
//#endif

