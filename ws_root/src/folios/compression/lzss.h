/* @(#) lzss.h 95/05/12 1.4 */
/* $Id: lzss.h,v 1.1 1994/08/17 16:35:13 vertex Exp $ */

#ifndef __LZSS_H
#define __LZSS_H


/*****************************************************************************/


/* Various constants used to define the compression parameters.
 * INDEX_BIT_COUNT tells how many bits we allocate to indices into the
 * text window. This directly determines the WINDOW_SIZE.  LENGTH_BIT_COUNT
 * tells how many bits we allocate for the length of an encoded phrase. This
 * determines the size of the look ahead buffer. END_OF_STREAM is a special
 * index used to flag the fact that the file has been completely encoded, and
 * there is no more data. MOD_WINDOW() is a macro used to perform arithmetic
 * on tree indices.
 */

#define INDEX_BIT_COUNT  12
#define LENGTH_BIT_COUNT 4
#define WINDOW_SIZE      (1 << INDEX_BIT_COUNT)
#define BREAK_EVEN       2
#define END_OF_STREAM    0
#define MOD_WINDOW(a)    ((a) & (WINDOW_SIZE - 1))


/*****************************************************************************/


#endif /* __LZSS_H */
