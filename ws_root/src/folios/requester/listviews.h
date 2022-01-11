/* @(#) listviews.h 96/09/07 1.2 */

#ifndef __LISTVIEWS_H
#define __LISTVIEWS_H


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


typedef void (* RenderEntryFunc)(int16 x, int16 y, int16 width, int16 height,
                                 int16 entryNum, void *userData);


typedef struct
{
    AnimNode         lv;
    int16            lv_X;
    int16            lv_Y;
    int16            lv_Width;
    int16            lv_Height;
    int16            lv_SelectionY;
    int16            lv_NumEntries;
    int16            lv_PrePadEntries;
    int16            lv_PostPadEntries;
    int16            lv_EntryHeight;
    int16            lv_EntryBeingAdded;
    int16            lv_EntryAfterOneDeleted;
    int16            lv_SelectedEntry;
    TransitionInfo   lv_RemovalLevel;
    TransitionInfo   lv_InsertionLevel;
    TransitionInfo   lv_TopLine;
    RenderEntryFunc  lv_RenderFunc;
    void            *lv_UserData;
} ListView;

typedef struct
{
    int16            lva_X;
    int16            lva_Y;
    int16            lva_Width;
    int16            lva_Height;
    int16            lva_EntryHeight;
    RenderEntryFunc  lva_RenderFunc;
    void            *lva_UserData;
} ListViewArgs;


/*****************************************************************************/


Err PrepListView(struct StorageReq *req, ListView *lv, ListViewArgs *lva);
void UnprepListView(struct StorageReq *req, ListView *lv);
void SetListViewNumEntries(struct StorageReq *req, ListView *lv, int16 numEntries);
void AddListViewEntry(struct StorageReq *req, ListView *lv, int16 afterThisOne);
void RemoveListViewEntry(struct StorageReq *req, ListView *lv, int16 entryNum);
void ScrollListView(struct StorageReq *req, ListView *lv, int16 newSelectedEntry);
int16 GetSelectedEntry(ListView *lv);
void GetSelectedEntryPos(ListView *lv, int16 *x, int16 *y);
void GetListViewCorners(const ListView *lv, Corner *corners);

#define DisableListView(req, lv) DisableAnimNode(req, (AnimNode *)lv)
#define EnableListView(req, lv)  EnableAnimNode(req, (AnimNode *)lv)


/*****************************************************************************/


#endif /* __LISTVIEWS_H */
