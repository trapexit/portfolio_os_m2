#ifndef __GRAPHICS_CLT_CLT_H
#define __GRAPHICS_CLT_CLT_H


/******************************************************************************
**
**  @(#) clt.h 96/02/16 1.30
**
**  Definitions for the command-list toolkit.
**
******************************************************************************/


#include <graphics/clt/gstate.h>

/* All register definitions, and shortcut macros, are defined in cltmacros.h */
#include <graphics/clt/cltmacros.h>

/* Data structures */

/* A snippet is used whereever a command list is useful.
   It carries around the size of the command list that is pointed to by
   the pointer. */

typedef struct {
	uint32	*data;
	uint16	size;		/* Number of words used */
	uint16  allocated; /* Number of words allocated */
} CltSnippet;


/*****************************************************************************/


/* CLT error codes */

#define MakeCLTErr(svr,class,err) MakeErr(ER_LINKLIB,ER_CLT,svr,ER_E_SSTM,class,err)

#define CLT_ERR_BADPTR     MakeCLTErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define CLT_ERR_NOTFOUND   MakeCLTErr(ER_SEVERE,ER_C_STND,ER_NotFound)
#define CLT_ERR_NOSUPPORT  MakeCLTErr(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define CLT_ERR_NOMEM      MakeCLTErr(ER_SEVERE,ER_C_STND,ER_NoMem)


/*****************************************************************************/


/* Function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

extern void  CLT_InitSnippet(CltSnippet *s);
extern void  CLT_CopySnippet(CltSnippet *dest, const CltSnippet *src);
extern void  CLT_CopySnippetData(uint32 **dest, const CltSnippet *src);
extern Err   CLT_AllocSnippet(CltSnippet *s, uint32 nWords);
extern void  CLT_FreeSnippet(CltSnippet *s);

/* Utility routines */

void CLT_SetSrcToCurrentDest(GState *g);
void CLT_ClearFrameBuffer(GState *gs, float red, float green, float blue,
						  float alpha, bool clearScreen, bool clearZ);

#ifdef __cplusplus
}
#endif

#define CLT_GetSize(snippet)	((snippet)->size)
#define CLT_GetData(snippet)	((snippet)->data)

extern const CltSnippet CltNoTextureSnippet;
extern const CltSnippet CltEnableTextureSnippet;


#endif /* __GRAPHICS_CLT_CLT_H */
