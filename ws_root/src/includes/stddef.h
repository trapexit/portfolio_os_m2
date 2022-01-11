#ifndef __STDDEF_H
#define __STDDEF_H


/******************************************************************************
**
**  @(#) stddef.h 96/02/20 1.2
**
**  Standard C definitions
**
******************************************************************************/


typedef long int      ptrdiff_t;
typedef unsigned long size_t;
typedef char          wchar_t;

#define offsetof(type,field)		((size_t)&(((type *)NULL)->field))

#define NULL ((void *)0)


/*****************************************************************************/


#endif /* __STDDEF_H */
