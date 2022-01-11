#ifndef __GRAPHICS_FRAME2D_LOADTXTR_H
#define __GRAPHICS_FRAME2D_LOADTXTR_H


/******************************************************************************
**
**  @(#) loadtxtr.h 96/02/22 1.2
**
**  Definitions for loading 2D framework Sprite texture files.
**
******************************************************************************/

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

/* --------- Tags appropriate for Spr_LoadTexture() --------- */
/* Move these to spriteobj.h */
#define LOADTEXTURE_TAG_base            (10)
#define LOADTEXTURE_TAG_IFFPARSER       (LOADTEXTURE_TAG_base + 0)
#define LOADTEXTURE_TAG_FILENAME        (LOADTEXTURE_TAG_base + 1)
#define LOADTEXTURE_TAG_IFFPARSETYPE    (LOADTEXTURE_TAG_base + 2)
#define LOADTEXTURE_TAG_CALLBACK        (LOADTEXTURE_TAG_base + 3)
#define LOADTEXTURE_TAG_CALLBACKDATA    (LOADTEXTURE_TAG_base + 4)
#define LOADTEXTURE_TAG_SPRITETYPE      (LOADTEXTURE_TAG_base + 5)

/* --------- Options for LOADTEXTURE_TAG_IFFPARSETYPE --------- */
#define LOADTEXTURE_TYPE_AUTOPARSE      (0)
#define LOADTEXTURE_TYPE_SINGLE         (1)
#define LOADTEXTURE_TYPE_MULTIPLE       (2)

/* --------- Sprite types for LOADTEXTURE_TAG_SPRITETYPE --------- */
#define LOADTEXTURE_SPRITETYPE_NORMAL   (0)
#define LOADTEXTURE_SPRITETYPE_SHORT    (1)
#define LOADTEXTURE_SPRITETYPE_EXTENDED (2)

/* --------- Prototypes --------- */
void Spr_UnloadTexture(List *SpriteList);
Err  Spr_LoadTexture(List *SpriteList, const TagArg *tags);
Err  Spr_LoadTextureVA(List *SpriteList, uint32 tag, ... );

/* --------- Error codes --------- */
#define MakeLTErr(svr,class,err)        MakeErr(ER_FOLI,ER_FRAME2D,svr,ER_E_SSTM,class,err)

#define LT_ERR_BADTAGARG            MakeLTErr(ER_SEVERE, ER_C_STND, ER_BadTagArg)
#define LT_ERR_NOMEM                MakeLTErr(ER_SEVERE, ER_C_STND, ER_NoMem)

#define LT_ERR_INCOMPLETEPARAMETERS MakeLTErr(ER_SEVERE, ER_C_NSTND, 1)
#define LT_ERR_MUTUALLYEXCLUSIVE    MakeLTErr(ER_SEVERE, ER_C_NSTND, 2)
#define LT_ERR_UNKNOWNIFFFILE       MakeLTErr(ER_SEVERE, ER_C_NSTND, 3)
#define LT_ERR_FAULTYTEXTURE        MakeLTErr(ER_SEVERE, ER_C_NSTND, 4)

/* --------- Callback --------- */
typedef uint32 (*LTCallBack)(uint32 TextureNumber, void *UserData);

/* --------- Return values for the above function --------- */
#define LOADTEXTURE_OK                  (0)
#define LOADTEXTURE_SKIP                (1)
#define LOADTEXTURE_STOP                (2)

#endif  /* __GRAPHICS_FRAME2D_LOADTXTR_H */

