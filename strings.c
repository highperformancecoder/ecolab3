#include <string.h>

char *strchr_(char* cs,char c)
{return strchr(cs,c);}

char *strstr_(char* cs, char* ct)
{return strstr(cs,ct);}

#include <netinet/in.h>
unsigned short htons_(unsigned short x)
{return htons(x);}

#include <rpc/types.h>
#include <rpc/xdr.h>
int xdr_setpos_(XDR* x,int p) {return xdr_setpos(x,p);}
int xdr_getpos_(XDR* x) {return xdr_getpos(x);}
void xdr_destroy_(XDR* x){xdr_destroy(x);}
