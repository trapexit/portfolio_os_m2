/*
 *      Copyright 1995, The 3DO Company
 *      File: 	  @(#) gfx_debug.h   95/12/04  1.4
 *	Contents: Contains the definitions for render buffer Items.
 *	Author:	David Somayajulu		Feb. 1, 1995
 */


#ifndef __GFX_DEBUG_H
#define __GFX_DEBUG_H

#include <stdio.h>
#include <kernel/super.h>

#define GFX_FOLIO_DEBUG		0
#if GFX_FOLIO_DEBUG
#define	GfxFolioDebug(x)	printf x
#else
#define GfxFolioDebug(x)
#endif

#define GFX_CNTXT_DEBUG		0
#if GFX_CNTXT_DEBUG
#define	GfxCntxtDebug(x)	printf x
#else
#define GfxCntxtDebug(x)
#endif

#define GFX_RBUFFER_DEBUG	0	
#if GFX_RBUFFER_DEBUG
#define	GfxRbfrDebug(x)		printf x
#else
#define GfxRbfrDebug(x)
#endif


#define	GFX_TEHARDWARE_DEBUG	0

#if GFX_TEHARDWARE_DEBUG

extern void DumpTEGCRegister(void);
extern void DumpSetUpEngine(void);
extern void DumpEdgeWalker(void);
extern void DumpPipRam(void);
extern void DumpTextureBlender(void);
extern void DumpDestinationBlender(void);
extern void DumpTriangleEngine(void);

#define DUMPTEGCRegister()		DumpTEGCRegister()
#define DUMPSetUpEngine()		DumpSetUpEngine()
#define DUMPEdgeWalker()		DumpEdgeWalker()
#define DUMPPipRam()			DumpPipRam()
#define DUMPTextureBlender()		DumpTextureBlender()
#define DUMPDestinationBlender()	DumpDestinationBlender()
#define DUMPTriangleEngine()		DumpTriangleEngine()

#else

#define DUMPTEGCRegister()		
#define DUMPSetUpEngine()	
#define DUMPEdgeWalker()		
#define DUMPPipRam()		
#define DUMPTextureBlender()		
#define DUMPDestinationBlender()	
#define DUMPTriangleEngine()		

#endif

#endif /* __GFX_DEBUG_H */
