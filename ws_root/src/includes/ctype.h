#ifndef __CTYPE_H
#define __CTYPE_H


/******************************************************************************
**
**  @(#) ctype.h 96/02/20 1.9
**
**  Standard C character type definitions
**
******************************************************************************/


#define __S 1		 /* whitespace		 */
#define __P 2		 /* punctuation 	 */
#define __B 4		 /* blank		 */
#define __L 8		 /* lower case letter	 */
#define __U 16		 /* upper case letter	 */
#define __N 32		 /* (decimal) digit	 */
#define __C 64		 /* control chars	 */
#define __X 128 	 /* hex chars - A-F, a-f */

extern unsigned const char *__ctype;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int isalnum(int), isalpha(int), iscntrl(int), isdigit(int), isgraph(int);
int islower(int), isprint(int), ispunct(int), isspace(int), isupper(int);
int isxdigit(int), tolower(int), toupper(int);

#define isalnum(c) (__ctype[c] & (__U+__L+__N))
#define isalpha(c) (__ctype[c] & (__U+__L))
#define iscntrl(c) (__ctype[c] & __C)
#define isdigit(c) (__ctype[c] & __N)
#define isgraph(c) (__ctype[c] & (__L+__U+__N+__P))
#define islower(c) (__ctype[c] & __L)
#define isprint(c) (__ctype[c] & (__L+__U+__N+__P+__B))
#define ispunct(c) (__ctype[c] & __P)
#define isspace(c) (__ctype[c] & __S)
#define isupper(c) (__ctype[c] & __U)
#define isxdigit(c) (__ctype[c] & (__N+__X))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CTYPE_H */
