/* @(#) listviews.c 96/10/30 1.3 */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/gstate.h>
#include <stdio.h>
#include <stdlib.h>
#include "req.h"
#include "utils.h"
#include "listviews.h"


/*****************************************************************************/


#define SCROLL_SPEED 75


/*****************************************************************************/


static void StepListView(StorageReq *req, ListView *lv);
static void DrawListView(StorageReq *req, ListView *lv, GState *gs);


static const AnimMethods listViewMethods =
{
    (UnprepFunc)UnprepListView,
    (StepFunc)StepListView,
    (DrawFunc)DrawListView,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


Err PrepListView(StorageReq *req, ListView *lv, ListViewArgs *lva)
{
    PrepAnimNode(req, (AnimNode *)lv, &listViewMethods, sizeof(ListView));
    PrepTransition(&lv->lv_InsertionLevel, TT_NONE, 0, 0, 0, NULL);
    PrepTransition(&lv->lv_RemovalLevel, TT_NONE, 0, 0, 0, NULL);

    lv->lv_X             = lva->lva_X;
    lv->lv_Y             = lva->lva_Y;
    lv->lv_Width         = lva->lva_Width;
    lv->lv_Height        = lva->lva_Height;
    lv->lv_NumEntries    = 0;
    lv->lv_EntryHeight   = lva->lva_EntryHeight;
    lv->lv_RenderFunc    = lva->lva_RenderFunc;
    lv->lv_UserData      = lva->lva_UserData;
    lv->lv_SelectionY    = (lv->lv_Height - lv->lv_EntryHeight) / 2;

    SetListViewNumEntries(req, lv, 0);

    return 0;
}


/*****************************************************************************/


void UnprepListView(StorageReq *req, ListView *lv)
{
    UnprepTransition(&lv->lv_RemovalLevel);
    UnprepTransition(&lv->lv_InsertionLevel);
    UnprepTransition(&lv->lv_TopLine);
    UnprepAnimNode(req, (AnimNode *)lv);
}


/*****************************************************************************/


static void StepListView(StorageReq *req, ListView *lv)
{
int16 totalHeight;

    StepAnimNode(req, (AnimNode *)lv);
    StepTransition(&lv->lv_RemovalLevel, &req->sr_CurrentTime);
    StepTransition(&lv->lv_InsertionLevel, &req->sr_CurrentTime);
    StepTransition(&lv->lv_TopLine, &req->sr_CurrentTime);

    if (lv->lv_InsertionLevel.ti_Current == lv->lv_EntryHeight)
        lv->lv_EntryBeingAdded = -1;

    if (lv->lv_RemovalLevel.ti_Current == 0)
        lv->lv_EntryAfterOneDeleted = -1;

    totalHeight = (lv->lv_NumEntries + lv->lv_PrePadEntries + lv->lv_PostPadEntries)
                  * lv->lv_EntryHeight;

    if (lv->lv_EntryBeingAdded >= 0)
        totalHeight -= lv->lv_EntryHeight - lv->lv_InsertionLevel.ti_Current;

    if (lv->lv_EntryAfterOneDeleted >= 0)
        totalHeight += lv->lv_RemovalLevel.ti_Current;

    if (lv->lv_TopLine.ti_Current + lv->lv_Height > totalHeight)
    {
        PrepTransition(&lv->lv_TopLine, TT_NONE, totalHeight - lv->lv_Height,
                                                 totalHeight - lv->lv_Height,
                                                 0, NULL);
    }
}


/*****************************************************************************/


static void DrawListView(StorageReq *req, ListView *lv, GState *gs)
{
int16  y;
int16  entry;
Color4 colors[4];
uint32 i;
uint32 row;
int16 y0, y1;
float  height;

    TOUCH(req);

    SetOutClipping(gs, lv->lv_X,
                       lv->lv_Y,
                       lv->lv_X + lv->lv_Width - 1,
                       lv->lv_Y + lv->lv_Height - 1);

    entry = lv->lv_TopLine.ti_Current / lv->lv_EntryHeight;
    y     = lv->lv_Y - (lv->lv_TopLine.ti_Current % lv->lv_EntryHeight);
    while (y < lv->lv_Y + lv->lv_Height)
    {
        if (entry == lv->lv_EntryBeingAdded)
        {
            /* clear with a height of lv->lv_InsertionLevel.ti_Current */
            y += lv->lv_InsertionLevel.ti_Current;
            entry++;
            continue;
        }

        if (entry == lv->lv_EntryAfterOneDeleted)
        {
            /* clear with height lv->lv_RemovalLevel.ti_Current */
            y += lv->lv_RemovalLevel.ti_Current;
        }

        if ((entry < lv->lv_PrePadEntries)
         || (entry >= lv->lv_PrePadEntries + lv->lv_NumEntries))
        {
            /* clear region */
        }
        else
        {
            (* lv->lv_RenderFunc)(lv->lv_X + 4, y, lv->lv_Width - 8, lv->lv_Height,
                                  entry - lv->lv_PrePadEntries,
                                  lv->lv_UserData);
        }

        y += lv->lv_EntryHeight;
        entry++;
    }
    ClearClipping(gs);

    y0 = lv->lv_Y;
    height = (lv->lv_SelectionY  / 20.0) + .2;
    y1 = y0 + height;
    for (row = 1; row < 21; row++)
    {
        for (i = 0; i < 4; i++)
        {
            colors[i].r = (104.0 / 256.0);
            colors[i].g = (12.0 / 256.0);
            colors[i].b = (32.0 / 256.0);
            colors[i].a = ((2.0 - (row / 20.0)) * .5) - .2;
        }

        ShadedRect(gs, lv->lv_X,
                       y0,
                       lv->lv_X + lv->lv_Width,
                       y1,
                       colors);
        y0  = y1;
        y1  = lv->lv_Y + row * height;

        if (y1 > lv->lv_SelectionY + lv->lv_Y)
            y1 = lv->lv_SelectionY + lv->lv_Y;
			
    }

    y0 = lv->lv_Y + lv->lv_SelectionY + lv->lv_EntryHeight;
    height = (lv->lv_SelectionY  / 20.0) + .2;
    y1 = y0 + height;
    for (row = 1; row < 21; row++)
    {
        for (i = 0; i < 4; i++)
        {
            colors[i].r = (104.0 / 256.0);
            colors[i].g = (12.0 / 256.0);
            colors[i].b = (32.0 / 256.0);
            colors[i].a = ((2.0 - ((19 - row) / 20.0)) * .5) - .2;
        }

        ShadedRect(gs, lv->lv_X,
                       y0,
                       lv->lv_X + lv->lv_Width,
                       y1,
                       colors);

        y0  = y1;
        y1  = lv->lv_Y + lv->lv_SelectionY + lv->lv_EntryHeight + row * height;

        if (y1 > lv->lv_Height + lv->lv_Y + 1)
            y1 = lv->lv_Height + lv->lv_Y + 1;
    }
}


/*****************************************************************************/


void SetListViewNumEntries(StorageReq *req, ListView *lv, int16 numEntries)
{
int16 pad;
int16 topLine;

    TOUCH(req);

    lv->lv_NumEntries           = numEntries;
    lv->lv_EntryBeingAdded      = -1;
    lv->lv_EntryAfterOneDeleted = -1;
    lv->lv_SelectedEntry        = 0;

    pad                   = (lv->lv_SelectionY + lv->lv_EntryHeight - 1) / lv->lv_EntryHeight;
    lv->lv_PrePadEntries  = pad;
    lv->lv_PostPadEntries = pad;

    topLine = (lv->lv_PrePadEntries * lv->lv_EntryHeight) - lv->lv_SelectionY;
    PrepTransition(&lv->lv_TopLine, TT_NONE, topLine, topLine, 0, NULL);

    if (lv->lv_NumEntries == 0)
    {
        lv->lv_PostPadEntries++;
        lv->lv_SelectedEntry = -1;
    }
}


/*****************************************************************************/


void AddListViewEntry(StorageReq *req, ListView *lv, int16 afterThisOne)
{
int16 amount;

    if (afterThisOne >= lv->lv_NumEntries)
        afterThisOne = lv->lv_NumEntries - 1;

    afterThisOne += lv->lv_PrePadEntries;

    if (lv->lv_NumEntries == 0)
        lv->lv_PostPadEntries++;

    lv->lv_NumEntries++;

    if (lv->lv_EntryAfterOneDeleted == afterThisOne + 1)
    {
        lv->lv_EntryAfterOneDeleted = -1;
        amount = lv->lv_RemovalLevel.ti_Current;
    }
    else
    {
        if (lv->lv_EntryAfterOneDeleted > afterThisOne)
        {
            lv->lv_EntryAfterOneDeleted++;
        }

        amount = 1;
    }

    if (lv->lv_EntryBeingAdded > 0)
    {
        if (afterThisOne < lv->lv_EntryBeingAdded)
            lv->lv_EntryBeingAdded++;
    }
    else
    {
        lv->lv_EntryBeingAdded = afterThisOne + 1;

        PrepTransition(&lv->lv_InsertionLevel, TT_LINEAR,
                       amount, lv->lv_EntryHeight, SCROLL_SPEED,
                       &req->sr_CurrentTime);
    }
}


/*****************************************************************************/


void RemoveListViewEntry(StorageReq *req, ListView *lv, int16 entryNum)
{
int16 amount;

    if ((entryNum >= lv->lv_NumEntries) || (entryNum < 0))
        return;

    entryNum += lv->lv_PrePadEntries;

    lv->lv_NumEntries--;
    if (lv->lv_NumEntries == 0)
        lv->lv_PostPadEntries++;

    if (lv->lv_SelectedEntry >= lv->lv_NumEntries)
        lv->lv_SelectedEntry = lv->lv_NumEntries - 1;

    if (entryNum == lv->lv_EntryBeingAdded)
    {
        lv->lv_EntryBeingAdded = -1;
        amount = lv->lv_InsertionLevel.ti_Current;
    }
    else
    {
        if (entryNum < lv->lv_EntryBeingAdded)
            lv->lv_EntryBeingAdded--;

        amount = lv->lv_EntryHeight - 1;
    }

    if (lv->lv_EntryAfterOneDeleted >= 0)
    {
        if (entryNum < lv->lv_EntryAfterOneDeleted)
            lv->lv_EntryAfterOneDeleted--;
    }
    else
    {
        lv->lv_EntryAfterOneDeleted = entryNum;

        PrepTransition(&lv->lv_RemovalLevel, TT_LINEAR,
                       amount, 0, SCROLL_SPEED, &req->sr_CurrentTime);

    }
}


/*****************************************************************************/


void ScrollListView(StorageReq *req, ListView *lv, int16 newSelectedEntry)
{
int16 newTopLine;
int16 scrollSpeed;

    if (newSelectedEntry < 0)
    {
        newSelectedEntry = 0;
    }
    else if (lv->lv_NumEntries == 0)
    {
        newSelectedEntry = 0;
    }
    else if (newSelectedEntry >= lv->lv_NumEntries)
    {
        newSelectedEntry = lv->lv_NumEntries - 1;
    }

    lv->lv_SelectedEntry = newSelectedEntry;
    if (lv->lv_SelectedEntry >= lv->lv_NumEntries)
        lv->lv_SelectedEntry = lv->lv_NumEntries - 1;

    newTopLine = ((newSelectedEntry + lv->lv_PrePadEntries) * lv->lv_EntryHeight)
                - lv->lv_SelectionY;

    if (newTopLine != lv->lv_TopLine.ti_Current)
    {
        scrollSpeed = abs(lv->lv_TopLine.ti_Current - newTopLine) * 6;
        if (scrollSpeed < SCROLL_SPEED)
            scrollSpeed = SCROLL_SPEED;

        PrepTransition(&lv->lv_TopLine, TT_LINEAR,
                       lv->lv_TopLine.ti_Current, newTopLine,
                       scrollSpeed, &req->sr_CurrentTime);
    }
}


/*****************************************************************************/


int16 GetSelectedEntry(ListView *lv)
{
    return lv->lv_SelectedEntry;
}


/*****************************************************************************/


void GetSelectedEntryPos(ListView *lv, int16 *x, int16 *y)
{
    *x = lv->lv_X;
    *y = lv->lv_Y + lv->lv_SelectionY;
}


/*****************************************************************************/


void GetListViewCorners(const ListView *lv, Corner *corners)
{
    corners[0].cr_X = lv->lv_X;
    corners[0].cr_Y = lv->lv_Y - 1;

    corners[1].cr_X = lv->lv_X + lv->lv_Width - 1;
    corners[1].cr_Y = lv->lv_Y - 1;

    corners[2].cr_X = lv->lv_X + lv->lv_Width - 1;
    corners[2].cr_Y = lv->lv_Y + lv->lv_Height;

    corners[3].cr_X = lv->lv_X;
    corners[3].cr_Y = lv->lv_Y + lv->lv_Height;
}
