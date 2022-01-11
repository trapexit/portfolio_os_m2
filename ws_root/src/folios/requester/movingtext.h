/* @(#) movingtext.h 96/09/07 1.2 */

#ifndef __MOVINGTEXT_H
#define __MOVINGTEXT_H


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


typedef enum
{
    TEXT_DELETED,
    TEXT_COPIED
} TextTypes;

typedef struct
{
    AnimNode       mt;
    TextState     *mt_Label;
    TextTypes      mt_Type;
    bool           mt_Stabilized;
    TransitionInfo mt_CurrentX;
    TransitionInfo mt_CurrentY;
    TransitionInfo mt_CurrentAngle;

    int16          mt_DestX;
    int16          mt_DestY;

    int16          mt_Width;
} MovingText;


/*****************************************************************************/


Err PrepMovingText(struct StorageReq *req, MovingText *mt, enum TextTypes type, const char *label);
void UnprepMovingText(struct StorageReq *req, MovingText *mt);
void ReturnMovingText(StorageReq *req, MovingText *mt);

#define DisableMovingText(req, mt)     DisableAnimNode(req, (AnimNode *)mt)
#define EnableMovingText(req, mt)      EnableAnimNode(req, (AnimNode *)mt)


/*****************************************************************************/


#endif /* __MOVINGTEXT_H */
