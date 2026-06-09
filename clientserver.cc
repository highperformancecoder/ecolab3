#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include "tcl++.h"
#include "arrays.h"
#include "globals.h"
#include "maxmin.h"
#ifdef ZLIB
#include <zlib.h>
#else
typedef long uLongf; 
#endif
#include "Realloc.h"

/* Field of the label specifying amount of data to be sent, including
   trailing NUL */
#define BUFSIZE_LABEL 32

/* A pipe used to communicate between master and child process */
static int dpipe[2]={0,0}, cpipe[2]; /* data pipe and control pipe */
static Tcl_Channel sock;
static void close_socket(int d) {Tcl_Close(interp,sock); exit(0);}

/* Utility routines for managing the pipe */
/* nonblock=O_NONBLOCK or 0 */
static int read_pipe(void *buffer,int size,int nonblock=0)
{
  char ack;
  int s;
  fcntl(dpipe[0],F_SETFL,nonblock);
  
  for (s=0; s<size;)
    {
      s+=read(dpipe[0],((char*)buffer)+s,size-s);
      if (s<0) return s;
      write(cpipe[1],&ack,1);
    }
  return s;
}

static void write_pipe(void *buffer,int size)
{
  char ack;
  for (int s=0; s<size; s+=PIPE_BUF)
    {
      write(dpipe[1],((char*)buffer)+s,min(size-s,PIPE_BUF));
      read(cpipe[0],&ack,1);
    }
}

/* get global variables from server at argv[1] at port argv[2] */

NEWCMD(get_global_vars,2)
{
  static int childpid=0;

  if (childpid)   /* handler already running */
    {
      int sz;
      if (read_pipe(&sz,sizeof(int),O_NONBLOCK)==-1) return; /* no data yet */
      glue buffer(sz);
      buffer.size=read_pipe(buffer.data,sz);
      global_vars.unpack(buffer);
      tclvar model_valid("model_valid"); model_valid=1;
      return;
    }

  if (dpipe[0]==0 && (pipe(dpipe)||pipe(cpipe))) error("Error creating pipe");

  if (!(childpid=fork()))
    {
      int sz, bufsz, zret;
      uLongf ubufsz;
      char *buffer=NULL, *ubuffer=NULL;
      char bufsize[BUFSIZE_LABEL];
      close(dpipe[0]); 
      close(cpipe[1]); 

      signal(SIGTERM,close_socket);

      for (;;)
	{
	  sock=Tcl_OpenTcpClient(interp,atoi(argv[2]),argv[1],0,NULL,0);
	  Tcl_SetChannelOption(interp,sock,"-translation","binary");
	  Tcl_Read(sock,bufsize,BUFSIZE_LABEL);

	  sscanf(bufsize,"%ld %d",&ubufsz,&sz);
	  buffer=Realloc(buffer,sz);	  
	  ubuffer=Realloc(ubuffer,ubufsz);	  
	  for (bufsz=0; bufsz<sz; 
	       bufsz += Tcl_Read(sock,buffer+bufsz,sz-bufsz));  

	  if (sz<ubufsz)   /* data is compressed */
#ifdef ZLIB
	    if (Z_OK!=(zret=uncompress(ubuffer,&ubufsz,buffer,bufsz)))
	      {
		printf("Compression failure: %d\n",zret);
		goto abort;
	      }
	    else
	      {char *tmp; tmp=buffer; buffer=ubuffer; ubuffer=tmp; sz=ubufsz;}
#else
	    /* we're sunk */
	    printf("zlib not available\n");
#endif

	    write_pipe(&sz,sizeof(sz));
	    write_pipe(buffer,sz);
	abort:
	    Tcl_Close(interp,sock);
	}
    }
    else
      {close(dpipe[1]); close(cpipe[0]);}
}

static void dumpdata(ClientData cd, Tcl_Channel channel, char* host, int port)
{
  glue buffer;
  global allg;
  char bufsize[BUFSIZE_LABEL], *sendbuf;
  uLongf compsz; 
  static char *compbuf=NULL;
#ifdef MPI
  get_all_globals(allg);
#else
  allg=global_vars;
#endif
  allg.packup(buffer); 

#ifdef ZLIB
  compsz=13+1.1*buffer.size;
  compbuf=Realloc(compbuf,compsz);
  if (Z_OK!=compress(compbuf,&compsz,buffer.data,buffer.size))
    compsz=buffer.size;
#else
  compsz=buffer.size;
#endif

  if (compsz<buffer.size)
    sendbuf=compbuf;
  else
    {sendbuf=buffer.data; compsz=buffer.size;}

  sprintf(bufsize,"%*d %*d\n",BUFSIZE_LABEL/2-2,buffer.size,BUFSIZE_LABEL/2-2,
	  compsz);
  Tcl_SetChannelOption(interp,channel,"-translation","binary");
  Tcl_Write(channel,bufsize,BUFSIZE_LABEL);
  Tcl_Write(channel,sendbuf,compsz);
  Tcl_Close(interp,channel);
}

/* set up a data server on port argv[1] to send global variables to an
   ecolab frontend */
NEWCMD(data_server,1)
{
  if (!Tcl_OpenTcpServer(interp,atoi(argv[1]),NULL,dumpdata,0))
    perror("Error in data_server");
}

