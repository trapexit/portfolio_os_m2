/* @(#) overlays.h 96/10/30 1.6 */

#ifndef __OVERLAYS_H
#define __OVERLAYS_H


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
    OVL_WORKING,
    OVL_COPYING,
    OVL_MOVING,
    OVL_DELETING,
    OVL_CONFIRMDELETEFILE,
    OVL_CONFIRMDELETEDIR,
    OVL_CONFIRMDELETEFSYS,
    OVL_CONFIRMFORMAT,
    OVL_MEDIAFULL,
    OVL_MEDIAPROTECTED,
    OVL_MEDIAREQUEST,
    OVL_DUPLICATE,
    OVL_ERROR,
    OVL_INFO,
	OVL_CONFIRMCOPY,
	OVL_HIERARCHYCOPY,
	
    NUM_OVERLAYS    /* always last in enum */
} OverlayTypes;

typedef struct
{
    AnimNode         ov;

    TransitionInfo   ov_CurrentX;
    TransitionInfo   ov_CurrentY;
    TransitionInfo   ov_CurrentWidth;
    TransitionInfo   ov_CurrentHeight;

    uint16           ov_X;
    uint16           ov_Y;
    uint16           ov_Width;
    uint16           ov_Height;
    OverlayTypes     ov_Type;
    TextState       *ov_Label;
    bool             ov_Stabilized;
    bool             ov_DoText;
} Overlay;


/*****************************************************************************/


Err PrepOverlay(struct StorageReq *req, Overlay *ov, enum OverlayTypes type, int16 startingX, int16 startingY, ...);
void UnprepOverlay(struct StorageReq *req, Overlay *ov);
void MoveOverlay(StorageReq *req, Overlay *ov, int16 x, int16 y);

#define DisableOverlay(req, ov)     DisableAnimNode(req, (AnimNode *)ov)
#define EnableOverlay(req, ov)      EnableAnimNode(req, (AnimNode *)ov)


/*****************************************************************************/


#endif /* __OVERLAYS_H */
