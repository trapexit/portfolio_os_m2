/*
 *
 *
 */
#ifndef _MISC_H_
#define _MISC_H_

/*
 *
 */
#ifdef MACINTOSH
#include <graphics:graphics.h>
#include <graphics:view.h>
#include <graphics:clt:gstate.h>
#else
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/clt/gstate.h>
#endif

#include <stdio.h>
#include <assert.h>
#include "mercury.h"
#include "lighting.h"
#include "matrix.h"

#include "lights.h"

/*
 *
 */
#define mRandom(num) (((float)rand()/RAND_MAX)*(num))

/*
 * Turn OFF Z-Buffering
 */
#define mCLT_SetZbufferOff(a) \
	gs->gs_SendList((a)); \
	GS_WaitIO((a)); \
	GS_Reserve( (a), 4 ); \
	CLT_Sync(GS_Ptr((a))); \
	CLT_ClearRegister( (a)->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZBUFFEN,1) ); \
	CLT_ClearRegister( (a)->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZOUTEN,1) )

/*
 * Turn ON Z-Buffering
 */
#define mCLT_SetZbufferOn(a) \
	GS_Reserve( (a), 4 ); \
	CLT_Sync(GS_Ptr((a))); \
	CLT_SetRegister( (a)->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZBUFFEN,1) ); \
	CLT_SetRegister( (a)->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZOUTEN,1) )

/*
 *
 */
#define mColor3_Set( s, cr, cg, cb ) \
	(s)->r = (cr); \
	(s)->g = (cg); \
	(s)->b = (cb)

/*
 *
 */
#define mColor4_Set( s, cr, cg, cb, ca ) \
	(s)->r = (cr); \
	(s)->g = (cg); \
	(s)->b = (cb); \
	(s)->a = (ca)

/*
 *
 */
#define mColor4_SetColor( s, cr, cg, cb ) \
	(s)->r = (cr); \
	(s)->g = (cg); \
	(s)->b = (cb)

/*
 *
 */
#define mColor4_SetAlpha( s, ca ) \
	(s)->a = (ca)

/*
 *
 */
#if 0
void BBox_LocalToWorld( BBox* world, Matrix* mtx, BBox* local );
#endif
void BBox_Print( BBox *bbox );


void PodGeometry_Print(PodGeometry *geo);
void Color4_Print(Color4 *color4);
void Color3_Print(Color3 *color3);
void Indices_Print(short *indices, uint32 count);
void Vertices_Print(float *vertices, uint32 count);
void UV_Print(float *uvs, uint32 count);
uint32 Indices_GetCount(short *indices);

void GState_SetZBufferOff( GState *gs );
void GState_SetZBufferOn( GState *gs );

#endif
/* End of File */
