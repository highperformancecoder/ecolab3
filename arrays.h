#ifndef ARRAYS_H
#define ARRAYS_H

#include <stdio.h>

//#if (defined(sgi) && !defined(NDEBUG))
//extern "C" void __assert(const char *, const char *, int);
//extern void _assert(const char *, const char *, int);
//#define assert(EX)  ((EX)?((void)0):_assert( # EX , __FILE__, __LINE__))
//
//#else
#include <assert.h>
//#endif

/* include header file for C language kernel */ 
extern "C" {
#include "c_arrays.h"
void error(char *,...);
}

#include <iostream.h>

#include "newarrays.h"
#endif
