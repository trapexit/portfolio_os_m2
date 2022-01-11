#ifndef __HW_H
#define __HW_H


/******************************************************************************
**
**  @(#) hw.h 96/05/10 1.1
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __GRAPHICS_BITMAP_H
#include <graphics/bitmap.h>
#endif


/*****************************************************************************/


void InitTE(void);
void ClearTEStatus(void);
void ClearTEInterrupts(uint32 ints);
void SetTEFrameBuffer(const Bitmap *bitmap);
void SetTEZBuffer(const Bitmap *bitmap);


/*****************************************************************************/


#endif /* __HW_H */
