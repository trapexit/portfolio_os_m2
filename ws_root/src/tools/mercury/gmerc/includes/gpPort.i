/****
 *
 * Private includes for graphics pipeline
 *
 ****/
#ifndef _IGPPORT
#define _IGPPORT

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef OUTPUT_STIMULUS
#include <stdarg.h>
#endif

#ifdef NEW_MAC_MEMORY
#include "allocator.h"
#endif

#define UNUSED(x)		&x

extern void exit(int);
int strcasecmp (const char *s1, const char *s2) ;

/*
 * Memory allocation wrappers
 *
 *	Mem_Alloc(N)		lowest-level allocation, allocate N bytes
 *	Mem_Create(T, N)	allocate N items of type T
 *	Mem_New(T)			allocate an object of type T
 *	Mem_Extend(T, P, N)	extend an array of type T by N elements
 *	Mem_Free(P)			free something
 */
 
#ifdef NEW_MAC_MEMORY
	#define MALLOC	AllocateBlock
	#define REALLOC	ReallocateBlock
	#define FREE	FreeBlock
#else
	#define MALLOC	malloc
	#define REALLOC	realloc
	#define FREE	free
#endif

#define	Mem_Alloc(size) \
		MALLOC((unsigned int) (size))

#define	Mem_Realloc(ptr, size) \
		REALLOC(ptr, (unsigned int) size)

#define	Mem_Create(type, size) \
	(type*) MALLOC((unsigned int) (size) * sizeof(type))

#define	Mem_Extend(type, ptr, size) \
	(type*) REALLOC(ptr, (unsigned int) (size) * sizeof(type))

#define	Mem_Free(ptr)	FREE((void*) (ptr))

#define	Mem_New(type)	((type*) MALLOC(sizeof(type)))

#ifdef __cplusplus

#define	New_Array(type, size)	new type[size]
#define	New_Object(type)		new type()
#define	Del_Object(obj)			delete obj

#else
#define	New_Array(type, size) \
	(type*) MALLOC((unsigned int) (size) * sizeof(type))

#define	New_Object	Mem_New
#define	Del_Object(obj)	 {if (obj) Mem_Free(obj); }

#endif /* __cplusplus */

#endif /* IGPPORT */
