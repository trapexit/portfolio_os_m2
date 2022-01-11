/* @(#) bg.h 96/09/12 1.4 */

#ifndef __BG_H
#define __BG_H


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


#define BOXES_X                 (20)
#define BOXES_Y                 (50)
#define BOXES_WIDTH             (85)
#define BOXES_HEIGHT            (133)
#define BOXES_SPACING           (4)

#define BUTTON_X                (530)
#define BUTTON_Y                (18)
#define BUTTON_SPACING          (3)

#define LISTVIEW_X              (141)
#define LISTVIEW_Y              (102)
#define LISTVIEW_WIDTH          (332)
#define LISTVIEW_HEIGHT         (95)
#define LISTVIEW_ENTRYHEIGHT    (10)

#define SUITCASE_X              (25)
#define SUITCASE_Y              (192)

#define BG_WIDTH                (640)
#define BG_HEIGHT               (240)

#define BG_DRAWBAR              (0x00001)
#define BG_DRAWPROMPT           (0x00002)

#define BG_NUMSLICES            (10)


/*****************************************************************************/


typedef struct
{
    AnimNode   bg;
    SpriteObj *bg_Slices[BG_NUMSLICES];               /* Pointer to actual background slivers */
    uint32     bg_Flags;
} Background;


/*****************************************************************************/


void PrepBackground(StorageReq *req, Background *bg, SpriteObj **slices,
                    uint32 flags);

#define UnprepBackground(req, bg)       UnprepAnimNode(req, (AnimNode *)bg)
#define EnableBackground(req, bg)       EnableAnimNode(req, (AnimNode *)bg)
#define DisableBackground(req, bg)      DisableAnimNode(req, (AnimNode *)bg)


/*****************************************************************************/


#endif /* __BG_H */
