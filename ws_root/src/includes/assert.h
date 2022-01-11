#ifndef __ASSERT_H
#define __ASSERT_H


/******************************************************************************
**
**  @(#) assert.h 96/02/20 1.12
**
**  Standard C assert definitions
**
******************************************************************************/


/* assert is disabled unless NDEBUG is undefined and BUILD_PARANOIA */
/* is defined */

#undef assert

#ifdef BUILD_PARANOIA
#ifndef NDEBUG

void __assert(const char *, const char *, int);
#define assert(expr)    ((void)(!(expr) ? __assert(#expr,__FILE__,__LINE__):(void)0))

#else
#define assert(x)
#endif /* NDEBUG */
#else
#define assert(x)
#endif /* BUILD_PARANOIA */
#endif /* __ASSERT_H */
