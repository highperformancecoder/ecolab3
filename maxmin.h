inline int max(int i, int j)
{
  return (i>j)?i:j;
}

inline int min(int i, int j)
{
  return (i<j)?i:j;
}

inline double max(double i, double j)
{
  return (i>j)?i:j;
}

inline double min(double i, double j)
{
  return (i<j)?i:j;
}

/* the other type of mod! */
inline int mod(int x, int y)
{ 
  int r=x%y;
  return r>=0? r: r+y;
}

