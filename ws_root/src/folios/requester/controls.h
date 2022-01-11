/* @(#) controls.h 96/10/30 1.4 */

#ifndef __CONTROLS_H
#define __CONTROLS_H


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


typedef enum
{
    CTRL_TYPE_OK,
    CTRL_TYPE_LOAD,
    CTRL_TYPE_SAVE,

    CTRL_TYPE_CANCEL,
    CTRL_TYPE_EXIT,
    CTRL_TYPE_QUIT,

    CTRL_TYPE_DELETE,
    CTRL_TYPE_RENAME,
    CTRL_TYPE_CREATEDIR,
    CTRL_TYPE_FORMAT,
    CTRL_TYPE_COPY,
    CTRL_TYPE_MOVE,
    CTRL_TYPE_END_COPY,
    CTRL_TYPE_END_MOVE,

    CTRL_TYPE_OVL_DELETE,
    CTRL_TYPE_OVL_FORMAT,
    CTRL_TYPE_OVL_CANCEL,
    CTRL_TYPE_OVL_STOP,
    CTRL_TYPE_OVL_OK,

    CTRL_TYPE_OVL_LOCK,
    CTRL_TYPE_OVL_QUESTION,
    CTRL_TYPE_OVL_FOLDER,
    CTRL_TYPE_OVL_WORKING,
    CTRL_TYPE_OVL_HANDINSERT,
    CTRL_TYPE_OVL_ERROR,
	CTRL_TYPE_OVL_COPY,
	CTRL_TYPE_OVL_MOVE
} ControlTypes;


typedef struct
{
    AnimNode        ctrl;

    uint16          ctrl_X;                                                                         /* only the image */
    uint16          ctrl_Y;
    uint16          ctrl_Width;
    uint16          ctrl_Height;

    uint16          ctrl_TotalX;                                                            /* image and label */
    uint16          ctrl_TotalY;
    uint16          ctrl_TotalWidth;
    uint16          ctrl_TotalHeight;

    ControlTypes    ctrl_Type;
    MinNode         ctrl_Link;
    TextState      *ctrl_Label;
    uint32          ctrl_FgColor;                                                           /* The color to write text in */

    SpriteObj     **ctrl_Frames;                                                          /* Pointer to where the frames are */
    uint16          ctrl_NumFrames;                                                         /* The grand total number of frames */
    uint16          ctrl_FramesPerSecond;                                           /* How many frames per second */
    TransitionInfo  ctrl_FrameLevel;
    SpriteObj      *ctrl_DisabledFrame;
    bool            ctrl_AnimateSelection;
    bool            ctrl_Looping;                                                           /* Does this animation loop */
    bool            ctrl_PingPong;

    bool            ctrl_Disabled;                                                          /* Is this control enabled? */
    bool            ctrl_Selected;                                                          /* Is it selected? */
    bool            ctrl_Highlighted;                                                       /* Should it be highlighted */
} Control;


#define Control_Addr(a)		(Control *)((uint32)(a) - offsetof(Control, ctrl_Link))


/*****************************************************************************/


void PrepControl(struct StorageReq *req, Control *ctrl, int16 x, int16 y, ControlTypes type);
void UnprepControl(struct StorageReq *req, Control *ctrl);
void EnableControl(struct StorageReq *req, Control *ctrl);
void DisableControl(struct StorageReq *req, Control *ctrl);
void HighlightControl(struct StorageReq *req, Control *ctrl);
void UnhighlightControl(struct StorageReq *req, Control *ctrl);
void SelectControl(struct StorageReq *req, Control *ctrl);
void UnselectControl(struct StorageReq *req, Control *ctrl);
void GetControlCorners(const Control *ctrl, Corner *corners);
void PositionControl(struct StorageReq *req, Control *ctrl, int16 x, int16 y);


/*****************************************************************************/


#endif /* __CONTROLS_H */
