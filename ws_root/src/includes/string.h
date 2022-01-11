#ifndef __STRING_H
#define __STRING_H


/******************************************************************************
**
**  @(#) string.h 96/08/01 1.16
**
**  Standard C string and memory management definitions
**
******************************************************************************/


#ifndef __STDDEF_H
#include <stddef.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* string manipulation routines */
extern char *strcpy(char *dest, const char *source);
extern char *strncpy(char *dest, const char *source, size_t maxChars);
extern char *strcat(char *str, const char *appendStr);
extern char *strncat(char *str, const char *appendStr, size_t maxChars);
extern int strcmp(const char *str1, const char *str2);
extern int strncmp(const char *str1, const char *str2, size_t maxChars);
extern size_t strlen(const char *str);
extern char *strchr(const char *str, int searchChar);
extern char *strrchr(const char *str, int searchChar);
extern char *strtok(char *str, const char *seps);
extern char *strpbrk(const char *str, const char *breaks);
extern char *strstr(const char *str, const char *subString);
extern size_t strcspn(const char *str, const char *set);
extern size_t strspn(const char *str, const char *set);
extern char *strerror(int errorCode);

/* memory manipulation routines */
extern void *memmove(void *dest, const void *source, size_t numBytes);
extern void *memset(void *mem, int c, size_t numBytes);
extern void *memchr(const void *mem, int searchChar, size_t numBytes);
extern int memcmp(const void *mem1, const void *mem2, size_t numBytes);

/* non-standard extensions */
extern int strcasecmp(const char *str1, const char *str2);
extern int strncasecmp(const char *str1, const char *str2, size_t maxChars);
extern void memswap(void *m1, void *m2, size_t numBytes);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/* on Portfolio, these are synonyms */
#define memcpy(d,s,n) memmove((d),(s),(n))

/* more non-standard extensions */
#define stricmp(s1,s2)    strcasecmp((s1),(s2))
#define strnicmp(s1,s2,n) strncasecmp((s1),(s2),(n))
#define bzero(p,n)        memset((p),0,(n))
#define bcopy(s,d,n)      memcpy((d),(s),(n))


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects memmove(1), memset(1)
#pragma no_side_effects strcat(1), strncat(1), strcpy(1), strncpy(1)
#pragma no_side_effects strcasecmp, strncasecmp
#pragma pure_function strcmp, strncmp, strlen, strchr, strrchr, strstr
#pragma pure_function memcmp
#endif


#endif /* __STRING_H */





