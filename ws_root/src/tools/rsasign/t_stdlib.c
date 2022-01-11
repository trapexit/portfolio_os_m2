/* Copyright (C) RSA Data Security, Inc. created 1992.

   This file is used to demonstrate how to interface to an
   RSA Data Security, Inc. licensed development product.

   You have a royalty-free right to use, modify, reproduce and
   distribute this demonstration file (including any modified
   version), provided that you agree that RSA Data Security,
   Inc. has no warranty, implied or otherwise, or liability
   for this demonstration file or any modified version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "bsafe2.h"

/* If the standard C library comes with a memmove() that correctly
     handles overlapping buffers, MEMMOVE_PRESENT should be defined as
     1, else 0.
   The following defines MEMMOVE_PRESENT as 1 if it has not already been
     defined as 0 with C compiler flags.
 */
#ifndef MEMMOVE_PRESENT
#define MEMMOVE_PRESENT 1
#endif

void T_memset (p, c, count)
POINTER p;
int c;
unsigned int count;
{
  if (count != 0)
    memset (p, c, count);
}

void T_memcpy (d, s, count)
POINTER d, s;
unsigned int count;
{
  if (count != 0)
    memcpy (d, s, count);
}

void T_memmove (d, s, count)
POINTER d, s;
unsigned int count;
{
#if MEMMOVE_PRESENT
  if (count != 0)
    memmove (d, s, count);
#else
  unsigned int i;

  if ((char *)d == (char *)s)
    return;
  else if ((char *)d > (char *)s) {
    for (i = count; i > 0; i--)
      ((char *)d)[i-1] = ((char *)s)[i-1];
  }
  else {
    for (i = 0; i < count; i++)
      ((char *)d)[i] = ((char *)s)[i];
  }
#endif
}

int T_memcmp (s1, s2, count)
POINTER s1, s2;
unsigned int count;
{
  if (count == 0)
    return (0);
  else
    return (memcmp (s1, s2, count));
}

POINTER T_malloc (size)
unsigned int size;
{
  return ((POINTER)malloc (size == 0 ? 1 : size));
}

POINTER T_realloc (p, size)
POINTER p;
unsigned int size;
{
  if (p == NULL_PTR)
    return (T_malloc (size));
  else
    return ((POINTER)realloc (p, size == 0 ? 1 : size));
}

void T_free (p)
POINTER p;
{
  if (p != NULL_PTR)
    free (p);
}

