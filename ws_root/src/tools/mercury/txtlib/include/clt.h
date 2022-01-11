/*
 *    @(#) clt.h 95/12/21 1.28
 *  Copyright 1994, The 3DO Company
 */
#ifndef CLT_H
#define CLT_H

/*
  #include <graphics/clt/gstate.h>
  */

/* All register definitions, and shortcut macros, are defined in cltmacros.h */
#include "cltmacros.h"

/* Data structures */

/* A snippet is used whereever a command list is useful.
   It carries around the size of the command list that is pointed to by
   the pointer. */

typedef struct {
	uint32	*data;
	uint16	size;		/* Number of words used */
	uint16  allocated; /* Number of words allocated */
} CltSnippet;

/* Function prototypes */

#ifdef _cplusplus
extern "C" {
#endif

extern void	CLT_InitSnippet(CltSnippet *s);
extern void	CLT_CopySnippet(CltSnippet *dest, CltSnippet *src);
extern void	CLT_CopySnippetData(uint32 **dest, CltSnippet *src);
extern int CLT_AllocSnippet(CltSnippet *s, uint32 nWords);
extern void	CLT_FreeSnippet(CltSnippet *s);

/* Utility routines */
/*
  void CLT_SetSrcToCurrentDest(GState *g);
  void CLT_ClearFrameBuffer(GState *gs, float red, float green, float blue,
  float alpha, bool clearScreen, bool clearZ);
  
  */

#ifdef _cplusplus
}
#endif

#define CLT_GetSize(snippet)	((snippet)->size)
#define CLT_GetData(snippet)	((snippet)->data)

extern CltSnippet CltNoTextureSnippet;
extern CltSnippet CltEnableTextureSnippet;

#endif /*CLT_H*/
