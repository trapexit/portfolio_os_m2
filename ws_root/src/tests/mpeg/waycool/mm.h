#ifndef __MM_H
#define __MM_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <graphics/gp.h>
#include <graphics/graphics.h>
#include <graphics/clt/gstate.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/item.h>
#include <kernel/time.h>
#include <kernel/operror.h> 
#include <kernel/kernelnodes.h>
#include <kernel/mem.h>
#include <misc/event.h>
#include "myputils.h"
#include "plaympeg.h"

typedef struct TexDescrStruct {
	uint32			num_blocks;
	uint32			lerpMode;
	TexLOD			*tl;
	TexBlend		**txb;
	Texture			**tex;
	uint32			*txDataOff;
} TexDescr;

typedef struct ObjDescrStruct {
	uint32			num_blocks;
	uint32			tx_xsize;
	uint32			tx_ysize;
	uint32			vsegments;
	uint32			hsegments;
	uint32			num_verts;
	uint32			num_txwords;
	uint32          hiddenSurf;
	uint32          cullFaces;
	MatProp    		material;
	Point3			*vtxData;
	Vector3			*normData;
	TexCoord		*texData;
	Surface			*surf;
	struct ObjDescrStruct *nextObject;
} ObjDescr;

typedef struct {
	uint32		*mpegBase;
	uint32		*mpegPadded;
	uint32		*mpegActive;
	Item        bitmapItem;
	Bitmap      *bitmapPtr;
} MPEGBuf;

Err     createSphere(ObjDescr *sph, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth);
Err     createTube(ObjDescr *obj, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth);
Err     createPlane(ObjDescr *pln, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth);
Err     createTorus(ObjDescr *tor, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth);
Err		initMorph(ObjDescr *morph, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth);
Err     morphObject(ObjDescr *from, ObjDescr *to, ObjDescr *morph, uint32 nsteps, uint32 step);
Err		objCreateAlloc(ObjDescr *obj);
Err		objCreateAddSurface(ObjDescr *obj);
Err		objCreateAddTexBlend(ObjDescr *obj, uint32 *tmem);
Err		drawObject(GP *gp, ObjDescr *obj, TexDescr *td, MatProp *mat);
void	destroyObject(ObjDescr *obj);
void    destroyTexture(TexDescr *obj);
Err		initMPEG(mpegStream *stream, char *fn, MPEGBuf *mpg);
Err		getMPEGBuf(MPEGBuf *mpg, uint32 tx_depth);
void	destroyMPEG(MPEGBuf *mpg);
void	setLerpMode(TexDescr *obj);
Err     waitvbl( int32 n);
Err     texCreateDescr(TexDescr *tdp, uint32 *tmem, uint32 num_blocks, uint32 tx_xsize, uint32 tx_ysize, uint32 transparent, uint32 tx_depth);
Item    allocbitmap ( int32 wide, int32 high, int32 type);
Err     GfxUtil_ClearScreenToBitmap( GP* gp, BitmapPtr bm, bool clearZ, uint32 x, uint32 y );

#define	APP_OK		(0)
#define	APP_MEM_ERR	(-1)
#define	APP_DEV_ERR	(-2)
#define	APP_IO_ERR	(-3)

#define LERP_TEX	(0)
#define	LERP_PRIM	(1)
#define	LERP_BLEND	(2)

#endif

