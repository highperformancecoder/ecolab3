/* tcl++.h

Contains 4 concepts - a NEWCMD macro for declaring Tcl procedures, a
tclvar class for accessing TCL variables as though they were C
variables, a tclcmd class that turns the TCL interpreter stream
into a simple I/O stream and tclindex, a simple iterator through a TCL array */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include <strstream>
#include <tcl.h>
#include <tk.h>
#include <setjmp.h>
#include <time.h>
#include "Realloc.h"

/// default interpreter 
extern Tcl_Interp *interp;   
/// main window of application
extern Tk_Window mainWin;    
extern jmp_buf TclError;
extern "C" void error(char *,...);
extern "C" char *strchr_(char*,char);  /* bug in g++ 2.7.0 */

/* these are defined to default values, even if MPI is false */
extern int myid, nprocs;

#if USE_MPI
#undef Status
#include <mpi.h>
/// Run a TCL command on all processors
/** tclvars should all be declared before PARALLEL to propagate to slaves */
#define PARALLEL if (myid==0) parsend(argc,argv);

/// maximum size of TCL command passed to slave processors 
#define MAXPMSG 1024
#define TAG_PUSH 1 
void parsend(int,const char**);
void parsend(char*,...);
#else
#define PARALLEL
#endif


class init_cmd
{ 
  ClientData clientdata;
 public:
  init_cmd(char *cmd_name, Tcl_CmdProc *cmd_fn) 
    { 
      if (interp==NULL) interp=Tcl_CreateInterp();
      Tcl_CreateCommand(interp,cmd_name,cmd_fn,clientdata,NULL);
    }
};


/** a hook for any code that needs to be executed before every command:
Usage:
\begin{verbatim}
    #define _TMP GLOBAL_INIT_HOOK \
       <user code goes here>
    #undef GLOBAL_INIT_HOOK
    #define GLOBAL_INIT_HOOK _TMP
    #undef _TMP
\end{verbatim}
*/
#ifndef GLOBAL_INIT_HOOK
#define GLOBAL_INIT_HOOK
#endif

#ifdef TIMECMDS
#define TIME(cmd) \
  tclvar cmd_times("cmd_times");\
  clock_t t0=clock();\
  cmd(argc,argv);\
  cmd_times[#cmd] = (exists(cmd_times[#cmd])? (double)cmd_times[#cmd]: 0.0) +\
     (double)(clock()-t0)/CLOCKS_PER_SEC;
#else
#define TIME(cmd)   cmd(argc,argv);
#endif

/** Macro for declaring TCL commands: takes a name and the number of
  arguments expected. The command will return and error if the number
  of arguments is not what is expected. A declaration of -1 for the
  expected number of arguments disables this checking */

#define NEWCMD(name,nargs) \
  static void name(int argc, const char* argv[]);\
  static int\
     name##_wrap(ClientData cd,Tcl_Interp* interp,int argc,const char* argv[])\
  {     /* set up error return and check number of arguments */\
    if (setjmp(TclError)!=0) return TCL_ERROR;\
    if (nargs>=0 && nargs+1>argc) error("Incorrect number of arguments");\
    while (Tcl_DoOneEvent(TCL_DONT_WAIT));  /* process any pending events */\
    GLOBAL_INIT_HOOK \
    TIME(name)\
    return TCL_OK;\
  }\
  static init_cmd name##_init(#name,name##_wrap);  /*register cmd in interp*/\
  static void name(int argc,const char* argv[])

class tclindex;

// TCL variable class 

     /** A class implementing the concept of a TCL variable. Here, if the
programmer declares 

tclvar hello="hello"; 

then the variable hello can be used just like a normal C variable in
expression such as 

floatvar=hello*3.4.  

However the difference is that hello is bound to a TCL variable called
hello, which can be accessed form TCL scripts.

*/

class tclvar
{ 
  char *name;  
  inline double dget(void);  
  inline double dput(double x);

 public:

/* constructors */
  tclvar();
  tclvar(char *nm, char *val);
  tclvar(const tclvar&);
  ~tclvar() {delete[] name;}

  ///assignment
  tclvar operator=(tclvar x); 
  ///tclvars may be freely mixed with arithmetic  expressions 
  double operator=(double x) {return dput(x);}

  const char* operator=(char* x) {return Tcl_SetVar(interp,name,x,0);}
  ///
  operator double () {return dget();}
  ///
  operator const char* () {return Tcl_GetVar(interp,name,TCL_GLOBAL_ONLY);}
  ///
  operator int() {return (int)dget();}
  operator unsigned () {return (unsigned)dget();}

  ///
  double operator++()   {return dput(dget()+1);}
  ///
  double operator++(int){double tmp; tmp=dget(); dput(tmp+1); return tmp;}
  ///
  double operator--()   {return dput(dget()-1);}
  ///
  double operator--(int){double tmp; tmp=dget(); dput(tmp-1); return tmp;} 
  ///
  double operator+=(double x) {return dput(dget()+x);}
  ///
  double operator-=(double x) {return dput(dget()-x);}
  ///
  double operator*=(double x) {return dput(dget()*x);}
  ///
  double operator/=(double x) {return dput(dget()/x);}

/// arrays can be indexed either by integers, or by strings 
  tclvar operator[](int index);
/// arrays can be indexed either by integers, or by strings 
  tclvar operator[](char* index);

///size  of arrays 
  int size();  

  friend int exists(const tclvar& x);
  friend tclindex;
};

/* space for a char[] variable to hold a double value in text form */
#define BUFSIZE 20 

inline
  double tclvar::dget(void)  
  {
    double val;
    int ival;
    const char* tclval;
    tclval=Tcl_GetVar(interp,name,TCL_GLOBAL_ONLY);
    if (tclval!=NULL)
      {if (Tcl_GetDouble(interp,tclval,&val)!=TCL_OK)
	 if (Tcl_GetBoolean(interp,tclval,&ival)!=TCL_OK)
	   error("%s as the value of %s\n",Tcl_GetStringResult(interp),name);
	 else
	   return ival;
     }
    else
      error("TCL Variable %s is undefined\n",name);
    return val;
  }

inline
  double tclvar::dput(double x)
  { 
    std::ostrstream value;
    value << x << '\0';
    Tcl_SetVar(interp,name,value.str(),TCL_GLOBAL_ONLY);
    return x;
  }

inline tclvar::tclvar()
{ 
  if (interp==NULL) interp=Tcl_CreateInterp();
  name=NULL;
};

inline 
 tclvar::tclvar(const tclvar& x)
{
  name = new char[strlen(x.name)+1];
  strcpy(name,x.name);
}

///Check if a TCL variable exists
inline
int exists(const tclvar& x)
{return Tcl_GetVar(interp,x.name,TCL_GLOBAL_ONLY)!=NULL;}

inline
  tclvar::tclvar(char *nm, char *val=NULL) 
    { 
      if (interp==NULL) interp=Tcl_CreateInterp();
      name=new char[strlen(nm)+1];  /* 1st argument gives binding */
      strcpy(name,nm);
      if (val!=NULL) Tcl_SetVar(interp,name,(char*)val,0);
#if USE_MPI       /* broadcast master value to slaves */
      if (myid==0 && exists(*this))
	parsend("set %s {%s}\n",name,(const char*)*this);
#endif
    }

inline
  tclvar tclvar::operator=(tclvar x) 
       {
	 delete[] name;
	 name = new char[strlen(x.name)+1]; 
	 strcpy(name,x.name);
	 return x;
       }

/* note the return statement is a g++ extension that names the return
value.  In this case, it is used to ensure that delete is called for
tmp.name at the correct time. The alternative is to create a helper
class to tclvar that does not delete its name when its destructor is called.
*/

inline
  tclvar tclvar::operator[](int index)
    {
      tclvar tmp;
      char *namestr = new char[strlen(name)+BUFSIZE];;
      strcpy(namestr,name);
      if (strchr_(name,'(')!=NULL)  /* already an indexed element */
	sprintf(strchr_(namestr,')'),"%d)",index);
      else
	sprintf(namestr+strlen(namestr),"(%d)",index);
      tmp.name = new char[strlen(namestr)+1];
      strcpy(tmp.name, namestr);
      delete []namestr;
      return tmp;
    }

inline
  tclvar tclvar::operator[](char* index)
    {
      tclvar tmp;
      tmp.name = new char[strlen(name)+strlen(index)+3];
      strcpy(tmp.name,name);
      if (strchr_(name,'(')!=NULL)  /* already an indexed element */
	sprintf(strchr_(tmp.name,')'),",%s)",index);
      else
	sprintf(tmp.name+strlen(tmp.name),"(%s)",index);
      return tmp;
    }

// enable printing of tclvars through the stream process 
inline
std::ostream& operator<<(std::ostream& stream, tclvar x)
{ return stream << (const char*) x;}

#define DBLSIZ 16 /* enough characters to hold a string rep of a double */
#define INTSIZ 12 /* enough characters to hold a string rep of an int */

/** a tclcmd allows Tcl commands to be executed by a simple x << "Tcl
   command" syntax. The commands can be stacked, or accumulated. Each
   time a linefeed is obtained, the command is evaluated. Normally
   spaces are inserted between components: 
\begin{verbatim} 
cmd << "hello" << "arg1"; 
\end{verbatim} 

corresponds to the TCL command "hello arg1". However you can use the
abutting operator as follows: 

\begin{verbatim} 
cmd << "hello" << "arg"|1; 
\end{verbatim} 

to achieve the same result */

class tclcmd
{
  char *buffer;
 public:
  char *result;   /* The result of the previous command is placed here */
  tclcmd() 
  {
    buffer=(char*)Realloc(NULL,1); 
    buffer[0]='\0'; 
    result=NULL;
  }
  ~tclcmd() {Realloc(buffer,0); Realloc(result,0);}
  tclcmd& operator<<(const char* cmd) 
    {
      buffer = (char*)Realloc(buffer, strlen(buffer)+strlen(cmd)+2);
      strcat(buffer, cmd);
      strcat(buffer," ");
      if (buffer[strlen(buffer)-2] == '\n') 
	{
	  if (Tcl_Eval(interp,buffer)!=TCL_OK) error("");
	  buffer[0] = '\0';
	  result = (char*)Realloc(result, strlen(Tcl_GetStringResult(interp))+1);
	  strcpy(result, Tcl_GetStringResult(interp));
	}
      return *this;
    }
  tclcmd& operator<<(double x) 
    {
      buffer = (char*) Realloc(buffer, strlen(buffer)+DBLSIZ+1);
      sprintf(buffer+strlen(buffer),"%g ",x);
      return *this;
    }
  tclcmd& operator<<(int x) 
    {
      buffer = (char*) Realloc(buffer, strlen(buffer)+INTSIZ+1);
      sprintf(buffer+strlen(buffer),"%d ",x);
      return *this;
    }
#if 0     
#ifdef _GNUC__
  tclcmd& operator<<(long long int x) 
    {
      buffer = (char*) Realloc(buffer, strlen(buffer)+2*INTSIZ+1);
      sprintf(buffer+strlen(buffer),"%d ",x);
      return *this;
    }
#endif
#endif
  tclcmd& operator<<(unsigned x) {return (*this) << (int) x;}
  tclcmd& operator<<(tclvar x) {return (*this) << (double) x;}
  /* remove trailing space */
  tclcmd& operator|(char *x) {buffer[strlen(buffer)-1]='\0'; return *this<<x;}
  tclcmd& operator|(double x) {buffer[strlen(buffer)-1]='\0'; return *this<<x;}
  tclcmd& operator|(int x) {buffer[strlen(buffer)-1]='\0'; return *this<<x;}
  tclcmd& operator|(unsigned x) 
    {buffer[strlen(buffer)-1]='\0'; return *this<<x;}
  tclcmd& operator|(tclvar x) {buffer[strlen(buffer)-1]='\0'; return *this<<x;}
};

class tclreturn: public std::ostrstream 
{
 public:
  ~tclreturn()
    {
      (*this) << '\0'; 
      //Tcl_AppendResult(interp,str(),NULL); 
      Tcl_SetResult(interp,str(),TCL_VOLATILE); 
#ifdef __GNUC__
      freeze(0); /* it is OK to free memory when destroyed */
#endif
    }
};

#if 0
template<class T>
operator<<(tclreturn& r,class T x) {r.data << x;}
#endif

/** index through TCL arrays: example usage - obtaining product of elements in 
 array 
\begin{verbatim}
for (ncells *= (int)idx.start(dims); !idx.last(); ncells *= (int)idx.incr() );
\end{verbatim}
 */
class tclindex  
{
  char *searchid;
  char *arrayname;
public:
  tclindex() {searchid=NULL; arrayname=NULL;}
  tclindex(const tclindex&);
  ~tclindex() {done();}
//start the indexing
  tclvar start(const tclvar&);
//finish up
  inline void done();
//get next element in array
  inline tclvar incr();
  tclvar incr(const tclvar& x) {return incr();}  /* ignores argument */
//return true if this is the last element in array
  int last();
};

inline 
tclindex::tclindex(const tclindex& x)
{
  searchid=x.searchid;
  arrayname=x.arrayname;
  if (searchid!=NULL)
    {
      searchid = new char[strlen(x.searchid)+1];
      strcpy(searchid,x.searchid);
      arrayname = new char[strlen(x.arrayname)+1];
      strcpy(arrayname,x.arrayname);
    }  
}

inline
void tclindex::done()
{
  tclcmd cmd;
  if (searchid!=NULL)
    {
      cmd << "array donesearch " << arrayname << searchid << "\n";
      delete [] arrayname;
      delete [] searchid;
      searchid=arrayname=NULL;
    }
}

inline 
tclvar tclindex::start(const tclvar& x)
{
  tclcmd cmd;
  tclvar r;
  /* check if x is actually an array variable */
  cmd << "array exists " << x.name << "\n";
  if (!atoi(cmd.result)) return x;

  done();  /* ensure any previous invocation is cleaned up */
  cmd << "array startsearch " << x.name << "\n";
  arrayname = new char [strlen(x.name)+1];
  searchid = new char [strlen(cmd.result)+1];
  strcpy(arrayname,x.name);
  strcpy(searchid,cmd.result);
  r=incr();    /* work around a compiler bug in gcc 2.8.1??? */
  return r;
}
  
inline
tclvar tclindex::incr()
{
  tclcmd cmd;
  tclvar r;
  
  if (searchid==NULL) error("tclindex not initialized");
  cmd << "array nextelement " << arrayname << searchid << "\n";
  r.name = new char[strlen(arrayname)+strlen(cmd.result)+3];
  sprintf(r.name,"%s(%s)",arrayname,cmd.result);
  return r;
}

inline 
int tclvar::size()
{
  
  tclcmd cmd;
  cmd << "array size " << name << "\n";
  return atoi(cmd.result);
}


inline 
int tclindex::last()
{
  tclcmd cmd;
  if (searchid!=NULL)
    {
      cmd << "array anymore " << arrayname << searchid << "\n";
      return !atoi(cmd.result);
    }
  else
    return 1;
}
