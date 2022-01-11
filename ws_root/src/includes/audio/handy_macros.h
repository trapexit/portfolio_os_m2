#ifndef __AUDIO_HANDY_MACROS_H
#define __AUDIO_HANDY_MACROS_H


/******************************************************************************
**
**  @(#) handy_macros.h 96/02/13 1.4
**
**  Private handy macros used a lot in audio stuff.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error This file may not be used in externally released source code.
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __STRING_H
#include <string.h>
#endif


/* -------------------- List */

    /* process nodes in list (including removing them)
       notes: only evaluates NextNode() when IsNode() is true */
#define PROCESSLIST(l,n,s,t)                                \
    for ( n = (t *)FirstNode(l);                            \
          IsNode(l,n) && ((s = (t *)NextNode(n)), TRUE);    \
          n = s )


/* -------------------- Packed String */
/*
    Macros to operate on packed strings (series of C strings concatenated
    together w/ '\0' in between).
*/

#define PackedStringSize(s)     (strlen(s) + 1)
#define NextPackedString(s)     ((s) + PackedStringSize(s))                 /* @@@ multiple evaluations of s */
#define AppendPackedString(d,s) (strcpy ((d),(s)) + PackedStringSize(s))    /* @@@ multiple evaluations of s */
#define IsValidPackedStringArray(array,arraySize) ((array)[(arraySize)-1] == '\0')  /* assumes array and arraySize represent a valid memory range (e.g., not 0-length) */


/* -------------------- Math */

#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))
#define ABS(x)      ((x<0)?(-(x)):(x))
#define CLIPRANGE(n,a,b) ((n)<(a) ? (a) : (n)>(b) ? (b) : (n))      /* range clipping */


/*****************************************************************************/


#endif /* __AUDIO_HANDY_MACROS_H */
