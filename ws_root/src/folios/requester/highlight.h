/* @(#) highlight.h 96/09/29 1.4 */

#ifndef __HIGHLIGHT_H
#define __HIGHLIGHT_H


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

#ifndef __ANIMNODES_H
#include "animnodes.h"
#endif


/*****************************************************************************/


#define NUM_HIGHLIGHT_CORNERS           (4)

typedef struct
{
    int16 cr_X;
    int16 cr_Y;
} Corner;

typedef struct
{
    TransitionInfo co_X;
    TransitionInfo co_Y;
} CornerTransition;

typedef struct
{
    AnimNode         hl;
    CornerTransition hl_Corners[NUM_HIGHLIGHT_CORNERS];
    TransitionInfo   hl_ColorMotion;
} Highlight;


/*****************************************************************************/


void PrepHighlight(struct StorageReq *req, Highlight *hl);
void UnprepHighlight(struct StorageReq *req, Highlight *hl);
void MoveHighlight(struct StorageReq *req, Highlight *hl, Corner *corners);
void MoveHighlightFast(struct StorageReq *req, Highlight *hl, Corner *corners);
bool IsMoveHighlightDone(struct StorageReq *req, Highlight *hl);

void DrawHighlight(StorageReq *req, Highlight *hl, GState *gs);
void StepHighlight(StorageReq *req, Highlight *hl);

void MoveHighlightNow(struct StorageReq *req, Highlight *hl, Corner *corners);
void DisableHighlightNow(struct StorageReq *req, Highlight *hl);

#define DisableHighlight(req, hl) DisableAnimNode(req, (AnimNode *)hl)
#define EnableHighlight(req, hl)  EnableAnimNode(req, (AnimNode *)hl)


/*****************************************************************************/


#endif /* __HIGHLIGHT_H */
