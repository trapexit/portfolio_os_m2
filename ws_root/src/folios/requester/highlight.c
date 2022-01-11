/* @(#) highlight.c 96/09/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/font.h>
#include <stdlib.h>
#include <string.h>
#include "req.h"
#include "utils.h"
#include "highlight.h"


/*****************************************************************************/


/*
static void StepHighlight(StorageReq *req, Highlight *hl);
static void DrawHighlight(StorageReq *req, Highlight *hl, GState *gs);
*/


static const AnimMethods highlightMethods =
{
    (UnprepFunc)UnprepHighlight,
    (StepFunc)StepHighlight,
    (DrawFunc)DrawHighlight,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


void PrepHighlight(StorageReq *req, Highlight *hl)
{
uint32 i;

    PrepAnimNode(req, (AnimNode *)hl, &highlightMethods, sizeof(Highlight));
    hl->hl.an.n_Priority = 200;

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        PrepTransition(&hl->hl_Corners[i].co_X, TT_NONE, 0, 0, 0, NULL);
        PrepTransition(&hl->hl_Corners[i].co_Y, TT_NONE, 0, 0, 0, NULL);
    }
    PrepTransition(&hl->hl_ColorMotion, TT_LOOPING, 0, 9, 14, &req->sr_CurrentTime);
}


/*****************************************************************************/


void UnprepHighlight(StorageReq *req, Highlight *hl)
{
uint32 i;

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        UnprepTransition(&hl->hl_Corners[i].co_X);
        UnprepTransition(&hl->hl_Corners[i].co_Y);
    }
    UnprepTransition(&hl->hl_ColorMotion);

    UnprepAnimNode(req, (AnimNode *)hl);
}


/*****************************************************************************/


void StepHighlight(StorageReq *req, Highlight *hl)
{
uint32 i;

    StepAnimNode(req, (AnimNode *)hl);

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        StepTransition(&hl->hl_Corners[i].co_X, &req->sr_CurrentTime);
        StepTransition(&hl->hl_Corners[i].co_Y, &req->sr_CurrentTime);
    }
    StepTransition(&hl->hl_ColorMotion, &req->sr_CurrentTime);
}


/*****************************************************************************/


void DrawHighlight(StorageReq *req, Highlight *hl, GState *gs)
{
Color4 primaryColor;
Color4 secondaryColor;
int32  pad;
uint32 x0, x1, x2;
uint32 y0, y1, y2;
Point2 points[21];
Color4 colors[21];
uint32 i;

    TOUCH(req);

    if (hl->hl.an_DisabledLevel.ti_Current == 0xff)
        return;

    primaryColor.r = 188.0 / 256.0;
    primaryColor.g = 209.0 / 256.0;
    primaryColor.b = 182.0 / 256.0;
    primaryColor.a = .7 * (255 - hl->hl.an_DisabledLevel.ti_Current) / 255.;

    secondaryColor.r = 24.0 / 256.0;
    secondaryColor.g = 24.0 / 256.0;
    secondaryColor.b = 24.0 / 256.0;
    secondaryColor.a = .7 * (255 - hl->hl.an_DisabledLevel.ti_Current) / 255.;

    pad = 3;

    x0 = hl->hl_Corners[0].co_X.ti_Current - pad*2;
    x2 = hl->hl_Corners[1].co_X.ti_Current + pad*2 + 1;
    y0 = hl->hl_Corners[0].co_Y.ti_Current - pad;
    y1 = hl->hl_Corners[0].co_Y.ti_Current;

    for (i = 0; i < 20; i += 2)
    {
        x1 = ((x2 - x0 + 1) / 9) * (i / 2) + x0;
        points[i].x   = x1;
        points[i].y   = y0;
        points[i+1].x = x1;
        points[i+1].y = y1;

        colors[i]   = primaryColor;
        colors[i+1] = primaryColor;
    }
    points[18].x = x2;
    points[19].x = x2;

    colors[hl->hl_ColorMotion.ti_Current*2]     = secondaryColor;
    colors[hl->hl_ColorMotion.ti_Current*2 + 1] = secondaryColor;

    ShadedStrip(gs, 20, points, colors);

    x0 = hl->hl_Corners[0].co_X.ti_Current - pad*2;
    x2 = hl->hl_Corners[1].co_X.ti_Current + pad*2 + 1;
    y0 = hl->hl_Corners[2].co_Y.ti_Current + 1;
    y1 = hl->hl_Corners[2].co_Y.ti_Current + pad + 1;

    for (i = 0; i < 20; i += 2)
    {
        x1 = ((x2 - x0 + 1) / 9) * (i / 2) + x0;
        points[i].x   = x1;
        points[i].y   = y0;
        points[i+1].x = x1;
        points[i+1].y = y1;

        colors[i]   = primaryColor;
        colors[i+1] = primaryColor;
    }
    points[18].x = x2;
    points[19].x = x2;

    colors[19 - (hl->hl_ColorMotion.ti_Current*2)]     = secondaryColor;
    colors[19 - (hl->hl_ColorMotion.ti_Current*2 + 1)] = secondaryColor;

    ShadedStrip(gs, 20, points, colors);

    x0 = hl->hl_Corners[0].co_X.ti_Current - pad*2;
    x1 = hl->hl_Corners[0].co_X.ti_Current - 1;
    y0 = hl->hl_Corners[0].co_Y.ti_Current;
    y2 = hl->hl_Corners[3].co_Y.ti_Current + 1;

    for (i = 0; i < 20; i += 2)
    {
        y1 = ((y2 - y0 + 1) / 9) * (i / 2) + y0;
        points[i].x   = x0;
        points[i].y   = y1;
        points[i+1].x = x1;
        points[i+1].y = y1;

        colors[i]   = primaryColor;
        colors[i+1] = primaryColor;
    }
    points[18].y = y2;
    points[19].y = y2;

    colors[19 - (hl->hl_ColorMotion.ti_Current*2)]     = secondaryColor;
    colors[19 - (hl->hl_ColorMotion.ti_Current*2 + 1)] = secondaryColor;

    ShadedStrip(gs, 20, points, colors);

    x0 = hl->hl_Corners[1].co_X.ti_Current + 1;
    x1 = hl->hl_Corners[1].co_X.ti_Current + pad*2 + 1;
    y0 = hl->hl_Corners[0].co_Y.ti_Current;
    y2 = hl->hl_Corners[3].co_Y.ti_Current + 1;

    for (i = 0; i < 20; i += 2)
    {
        y1 = ((y2 - y0 + 1) / 9) * (i / 2) + y0;
        points[i].x   = x0;
        points[i].y   = y1;
        points[i+1].x = x1;
        points[i+1].y = y1;

        colors[i]   = primaryColor;
        colors[i+1] = primaryColor;
    }
    points[18].y = y2;
    points[19].y = y2;

    colors[hl->hl_ColorMotion.ti_Current*2]     = secondaryColor;
    colors[hl->hl_ColorMotion.ti_Current*2 + 1] = secondaryColor;

    ShadedStrip(gs, 20, points, colors);
}


/*****************************************************************************/


void MoveHighlight(StorageReq *req, Highlight *hl, Corner *corners)
{
uint32 i;
int16  xSpeed;
int16  ySpeed;

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        xSpeed = abs(hl->hl_Corners[i].co_X.ti_Current - corners[i].cr_X) * 4;
        ySpeed = abs(hl->hl_Corners[i].co_Y.ti_Current - corners[i].cr_Y) * 4;

        PrepTransition(&hl->hl_Corners[i].co_X, TT_LINEAR, hl->hl_Corners[i].co_X.ti_Current, corners[i].cr_X, xSpeed, &req->sr_CurrentTime);
        PrepTransition(&hl->hl_Corners[i].co_Y, TT_LINEAR, hl->hl_Corners[i].co_Y.ti_Current, corners[i].cr_Y, ySpeed, &req->sr_CurrentTime);
    }
}

/*****************************************************************************/


void MoveHighlightFast(StorageReq *req, Highlight *hl, Corner *corners)
{
uint32 i;
int16  xSpeed;
int16  ySpeed;

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        xSpeed = abs(hl->hl_Corners[i].co_X.ti_Current - corners[i].cr_X) * 8;
        ySpeed = abs(hl->hl_Corners[i].co_Y.ti_Current - corners[i].cr_Y) * 8;

        PrepTransition(&hl->hl_Corners[i].co_X, TT_LINEAR, hl->hl_Corners[i].co_X.ti_Current, corners[i].cr_X, xSpeed, &req->sr_CurrentTime);
        PrepTransition(&hl->hl_Corners[i].co_Y, TT_LINEAR, hl->hl_Corners[i].co_Y.ti_Current, corners[i].cr_Y, ySpeed, &req->sr_CurrentTime);
    }
}

/*****************************************************************************/


bool IsMoveHighlightDone(StorageReq *req, Highlight *hl)
{
uint32	i;

	TOUCH(req);

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
#if 0 /* FIXME: this is how it should work, but transistions.c isn't coded to ever change a TT_LINEAR to TT_NONE for some reason */
		if (hl->hl_Corners[i].co_X.ti_Type != TT_NONE || hl->hl_Corners[i].co_Y.ti_Type != TT_NONE)
			return FALSE;
#else
		if (hl->hl_Corners[i].co_X.ti_Current != hl->hl_Corners[i].co_X.ti_End || hl->hl_Corners[i].co_Y.ti_Current != hl->hl_Corners[i].co_Y.ti_End)
			return FALSE;
#endif
    }
	
	return TRUE;
}

/*****************************************************************************/


void MoveHighlightNow(StorageReq *req, Highlight *hl, Corner *corners)
{
uint32 i;

    TOUCH(req);

    for (i = 0; i < NUM_HIGHLIGHT_CORNERS; i++)
    {
        hl->hl_Corners[i].co_X.ti_Current = corners[i].cr_X;
        hl->hl_Corners[i].co_Y.ti_Current = corners[i].cr_Y;
    }
}


/*****************************************************************************/


void DisableHighlightNow(struct StorageReq *req, Highlight *hl)
{
    TOUCH(req);

    hl->hl.an_DisabledLevel.ti_Current = 0xff;
    hl->hl.an_Disabled                 = TRUE;
}
