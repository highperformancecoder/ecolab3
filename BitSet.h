/* limited implementation of libg++ BitSet class for ecolab purposes */
#include <stdlib.h>
#include "Realloc.h"

class BitSet
{
  char *data;
  int size;
public:
  BitSet() {data=NULL; size=0;}
  ~BitSet() {Realloc(data,0);}
  void set(int i);
  int test(int i);
};
 
inline void BitSet::set(int i)
{
  div_t w=div(i,8);  /* w contains quotient and remainder of i/8 */

  if (w.quot >= size)
    {
      data=(char*)Realloc(data,w.quot+1);
      for (int i=size; i<=w.quot; i++) data[i]='\0';
      size=w.quot+1;
    }
  data[w.quot] |= 1<<w.rem;
}

inline int BitSet::test(int i)
{
  div_t w=div(i,8);  /* w contains quotient and remainder of i/8 */

  if (w.quot >= size) return 0;
  return (data[w.quot]>>w.rem & 1);
}
