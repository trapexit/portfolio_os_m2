#ifndef __GPI
#define __GPI

/*
 * Internal Pipeline headers
 */

#include "kerneltypes.h"

/*
 * Designates a routine private to the implementation but made
 * public for subclassing in the C binding
 */
/* #define	Private */

/*
#ifdef Protected
#undef Protected
#endif
#define	Protected	public
*/

#include "gp.h"

/*
 * Commonly used macros
 */
#define	SQ(a)		((a)*(a))
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#define ABS(A)      ( (A) < 0 ? -(A) : (A) )
#define RadiansToDegrees(a) (((a) * 180.0) /PI)
#define DegreesToRadians(a) (((a) * PI) / 180.0)

/* Arguments to exit(2). */
/* #define EXIT_SUCCESS    0 */
#define EXIT_FAIL    	(GFX_MiscError)

#ifndef GLIB_WARNING
#ifdef BUILD_STRINGS
#define GLIB_WARNING(s) { printf("%s, line %d: ", __FILE__, __LINE__); printf s;}
#else
#define GLIB_WARNING(s) { ; }
#endif
#endif

#ifndef GLIB_ERROR
#ifdef BUILD_STRINGS
#define GLIB_ERROR(s) { printf("%s, line %d: ", __FILE__, __LINE__); printf s; exit(EXIT_FAIL); }
#else
#define GLIB_ERROR(s) { printf("%s, line %d: ", __FILE__, __LINE__); printf("GLIB_ERROR: called\n"); exit(EXIT_FAIL); }
#endif
#endif

#include "gpPort.i"

#endif /* __GPI */

