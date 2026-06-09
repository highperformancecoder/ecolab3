#ifdef MEMDEBUG
char *Realloc(char *p, size_t sz);
#if 0
#define malloc(x) Realloc(NULL,x)
#define free(x)   Realloc(x,0)
#endif
#else
#include <stdlib.h>
#define Realloc(x,y) (char*)realloc(x,y)
#endif
