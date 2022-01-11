/****
 *
 *	@(#) frame2.i 96/07/09 1.13
 *	Copyright 1994, The 3DO Company
 *
 * Public includes for M2 2d Framework and graphics pipeline
 *
 ****/

#ifndef _FRAME2_I
#define _FRAME2_I

#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/mem.h>

#include <graphics/clt/gstate.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>
#include "sprcache.h"
#include <string.h>

extern void GS_SetNotSynced(GState* gs, uint32 notSynced);
extern uint32 GS_GetNotSynced(GState* gs);

#define COPYRESERVE(gs,list) GS_Reserve((gs),(list)->size); \
			     CLT_CopySnippetData (GS_Ptr(gs),(list));


typedef struct _geouv {
  gfloat x,y,u,v,w,r,g,b,a;
} _geouv;


void _set_notexture (GState *gs);
void _enable_alpha (GState *gs);
void _disable_alpha (GState *gs);
void _set_srcpassthrough (GState *gs);
SpriteCache *_clip_fan2d (GState *gs, SpriteCache *spc,
	int32 style, int32 count, _geouv *geo, bool dontClip);


#endif /* FRAME2_I */
