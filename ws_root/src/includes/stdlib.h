#ifndef __STDLIB_H
#define __STDLIB_H


/******************************************************************************
**
**  @(#) stdlib.h 96/02/20 1.16
**
******************************************************************************/


#ifndef __STDDEF_H
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/************** storage functions *******************************************/
void *malloc(size_t);
void free(void *);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *oldBlock, size_t newSize);

#ifdef MEMDEBUG
void *mallocDebug(size_t, const char *sourceFile, int sourceLine);
void freeDebug(void *, const char *sourceFile, int sourceLine);
void *callocDebug(size_t nelem, size_t elsize, const char *sourceFile, int sourceLine);
void *reallocDebug(void *oldBlock, size_t newSize, const char *sourceFile, int sourceLine);
#define malloc(s)    mallocDebug(s,__FILE__,__LINE__)
#define calloc(n,s)  callocDebug(n,s,__FILE__,__LINE__)
#define free(p)      freeDebug(p,__FILE__,__LINE__)
#define realloc(p,s) reallocDebug(p,s,__FILE__,__LINE__)
#endif /* MEMDEBUG */

/************** conversion functions ****************************************/
int  atoi(const char *nptr);
long atol(const char *nptr);
unsigned long strtoul(const char *nsptr, char **endptr, int base);
long strtol(const char *nsptr, char **endptr, int base);
float strtof(const char *str, char **endptr);

#ifdef __DCC__
#pragma no_side_effects atoi, atol
#pragma no_side_effects strtol(2,errno), strtoul(2,errno)
#endif

/************** environmental functions *************************************/

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0
void exit(int status);
int system(const char *cmdString);

#ifdef __DCC__
#pragma no_return exit
#endif

/************** integer math functions *************************************/

int abs(int i);
long labs(long i);

#ifdef __DCC__
#pragma pure_function abs, labs
#pragma inline abs, labs

__asm int abs( int num )
{
%   reg         num;
    mr          r3,num
    cmpwi       r3,0
    bge         1f
    neg         r3,r3
1:
}

__asm long labs( long num )
{
%   reg         num;
    mr          r3,num
    cmpwi       r3,0
    bge         2f
    neg         r3,r3
2:
}

#endif /* __DCC__ */

/************** algorithms *************************************************/

#define RAND_MAX     0x7fffffff
int rand(void);
void srand(unsigned int);

void qsort(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *));

void *bsearch(const void *key, const void *base,
                     size_t nmemb, size_t size,
                     int (*compar)(const void *, const void *));
#ifdef __DCC__
#pragma no_side_effects rand, srand, urand
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __STDLIB_H */
