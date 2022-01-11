/*  @(#) loaderty.h 96/04/23 1.13 */

#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include "predefines.h"

typedef char int8;
typedef short int16;
typedef long int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
#ifndef macintosh
// The new gcc header files define true and false - 
// the Unix Makefile is set up to use the older compiler.
// DEFINE _NEW_GCC IF USING NEW COMPILER!!!
#ifdef _NEW_GCC
   typedef bool Boolean;
#else
   enum Boolean { false, true }; 
#endif
typedef char Str255[255];
typedef const char ConstStr255Param[255];
typedef char* StringPtr;
typedef void* Ptr;
typedef int OSErr;
#endif

#if defined(_GCC) || defined(_MSDOS)
#define nil 0
#endif

#ifdef _GCC
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#endif

