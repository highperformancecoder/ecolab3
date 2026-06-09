extern "C" {
#include <tcl.h>
#include <tk.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#if BLT
#include <blt.h>
int Blt_Init(Tcl_Interp*);
#endif
}

#include "tcl++.h"

Tcl_Interp *interp=NULL;
jmp_buf TclError;
Tk_Window mainWin;

extern "C" void error(char *fmt,...)
{
  char errstring[200];  /* I hope this will always be large enough */
  va_list args;
  va_start(args, fmt);
  vsprintf(errstring,fmt,args);
  va_end(args);
#if MPI
  if (myid>0)
    {
      printf("Error on %d: %s\n",myid,errstring);
      exit(0);
    }
#endif  
  Tcl_AppendResult(interp,errstring,NULL);
  /* fix up any cursor */
  if (mainWin)
    XUndefineCursor(Tk_Display(mainWin),Tk_WindowId(mainWin)); 
  else
    puts(errstring);
  longjmp(TclError,1);
}

int myid=0, nprocs=1;   /* MPI task ID and no. of threads*/
#if MPI
#include <mpi.h>

/* MPI slave command loop */
void do_slave_loop()
{
  /* we have to assume a maximum size of message passed through :( */
  char buffer[MAXPMSG];
  tclcmd cmd;
  for (;;)  /* the only way out of this loop is to send the quit command */
    {
      MPI_Bcast(buffer,MAXPMSG,MPI_CHAR,0,MPI_COMM_WORLD);
      cmd << buffer << "\n";
    }
}
#endif

main(int argc, char* argv[])
{

#ifdef MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  if (myid>0)
    { 
      do_slave_loop();
      exit(0);
    }
#endif

  if (argc!=2) 
    {
      printf("Usage: %s <script>\n",argv[0]);
      exit(1);
    }

  /* set the TCL variables argc and argv to contain the
     arguments. argv[0] will contain the script name, not the
     interpreter name */
  tclvar tcl_argc("argc"), tcl_argv("argv");
  tcl_argc=argc-1;
  for (int i=1; i<argc; i++) tcl_argv[i-1]=argv[i];

if (Tcl_EvalFile(interp,argv[1])!=TCL_OK)
    {
      printf("%s\n",interp->result);
      printf("%s\n",Tcl_GetVar(interp,"errorInfo",0)); /* print out trace */
    }
  /* Clean up */
#ifdef MPI
  parsend("finalize");
  MPI_Finalize();
#endif
#ifdef __unix__
  signal(SIGTERM,SIG_IGN);
  kill(-getpid(),SIGTERM);
#endif
};

NEWCMD(Tkinit,0)
{
#ifdef TK3
  mainWin = Tk_CreateMainWindow(interp,NULL,argv[1],argv[1]);
#else
  Tk_Init(interp);
  mainWin = Tk_MainWindow(interp);
#endif
#if BLT
  if (Blt_Init(interp)!=TCL_OK)
    error("Error initialising BLT\n");
#endif  
}

NEWCMD(map_mainwin,0)
{
  Tk_GeometryRequest(mainWin,10,10);  /* default, in case window geom not 
					 specified */
  Tk_MapWindow(mainWin);
  Tk_MainLoop();
}

#if MPI
void parsend(int argc, char* argv[])
{
  char buffer[MAXPMSG];
  ostrstream cmd(buffer,MAXPMSG);
  for (int i=0; i<argc; i++) cmd << argv[i] << ' ';
  cmd << '\0';
  MPI_Bcast(cmd.str(),cmd.pcount(),MPI_CHAR,0,MPI_COMM_WORLD);
}

void parsend(char* fmt, ...)
{
  char buffer[MAXPMSG];
  va_list args;
  va_start(args, fmt);
  vsprintf(buffer,fmt,args);
  va_end(args);
  MPI_Bcast(buffer,strlen(buffer)+1,MPI_CHAR,0,MPI_COMM_WORLD);
}

NEWCMD(finalize,0)
{
  PARALLEL;
  MPI_Finalize();
  Tcl_Exit(0);
}
#endif

NEWCMD(exit_ecolab,0)  /* replace inbuilt exit with parallel version */
{
  kill(-getpid(),SIGTERM);
}

static char* sigcmd;
void sighand(int s)
{
  tclcmd c;
  c << sigcmd << "\n";
}

NEWCMD(trap,1)     /* trap SIGTERM to excute argv[1] */
{
  sigcmd=new char[strlen(argv[1])+1];
  strcpy(sigcmd,argv[1]);
  signal(SIGTERM,sighand);
  signal(SIGXCPU,sighand);
}

#include <sys/times.h>
#include <time.h>

/* Return total cputime used so far on all threads */
NEWCMD(cputime,0)
{
  PARALLEL;
  struct tms t;
  times(&t);
  int r=t.tms_utime+t.tms_stime;
#ifdef MPI
  int r1=r;
  MPI_Reduce(&r1,&r,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  tclreturn rr;
  rr<<(float)r/CLK_TCK;
}
