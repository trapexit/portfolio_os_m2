#ifndef __SOUND3D_PATCH_H
#define __SOUND3D_PATCH_H

/******************************************************************************
**
**  @(#) sound3d_patch.h 96/02/19 1.3
**
**  3D Sound Patch Builder
**
**-----------------------------------------------------------------------------
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**  RNM: Robert Marsanyi (rnm)
**
******************************************************************************/

#ifndef __KERNEL_TYPES_H
#include <kernel/item.h>
#endif

Item MakeSound3DPatchTemplate (uint32 s3dFlags);
#define DeleteSound3DPatchTemplate DeleteItem

#endif /* __SOUND3D_PATCH_H */
