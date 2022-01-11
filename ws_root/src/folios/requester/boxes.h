/* @(#) boxes.h 96/09/12 1.4 */

#ifndef __BOXES_H
#define __BOXES_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __GRAPHICS_FRAME2D_SPRITEOBJ_H
#include <graphics/frame2d/spriteobj.h>
#endif

#ifndef __GRAPHICS_FONT_H
#include <graphics/font.h>
#endif

#ifndef __TRANSITIONS_H
#include "transitions.h"
#endif



/*****************************************************************************/


typedef struct
{
    AnimNode        bx;

    int16           bx_New;                     /* Is there a new box? */
    int16           bx_X;                       /* The upper left x value */
    int16           bx_Y;                       /* The upper left y value */
    int16           bx_Width;                   /* The width of the box */
    int16           bx_Height;                  /* The height of the box */
    int16           bx_TotalHeight;
    int16           bx_ViewOffset;

    uint32          bx_NumBoxes;
    uint32          bx_StartBox;                /* What box the list starts at */
    TransitionInfo  bx_CurrentTotalHeight;
    TransitionInfo  bx_CurrentViewOffset;

    TransitionInfo  bx_FrameLevel;
    TransitionInfo  bx_BlinkLevel;
} Boxes;


/*****************************************************************************/


void PrepBoxes(struct StorageReq *req, Boxes *boxes, int16 x, int16 y, int16 width, int16 height);
void UnprepBoxes(struct StorageReq *req, Boxes *boxes);
void GetBoxCorners(struct StorageReq *req, Boxes *boxes, uint32 boxNum, Corner *corners);
void MakeBoxVisible(struct StorageReq *req, Boxes *boxes, uint32 boxNum);
void AddBoxes(struct StorageReq *req, Boxes *box, uint32 numBoxes);
void RemoveBoxes(struct StorageReq *req, Boxes *box, uint32 numBoxes);
void BlinkFirstBox(struct StorageReq *req, Boxes *box);


/*****************************************************************************/


#endif /* __BOXES_H */
