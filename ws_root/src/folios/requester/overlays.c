/* @(#) overlays.c 96/10/30 1.7 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/frame2d/spriteobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "req.h"
#include "utils.h"
#include "bg.h"
#include "msgstrings.h"
#include "sound.h"
#include "eventloops.h"
#include "overlays.h"

/*****************************************************************************/


#define SEPARATOR " : "


static void StepOverlay(StorageReq *req, Overlay *ov);
static void DrawOverlay(StorageReq *req, Overlay *ov, GState *gs);


static const AnimMethods overlayMethods =
{
    (UnprepFunc)UnprepOverlay,
    (StepFunc)StepOverlay,
    (DrawFunc)DrawOverlay,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


#define TITLE_HEIGHT  21
#define BUTTON_HEIGHT 16
#define TEXT_LEFT_PAD 10
#define TEXT_TOP_PAD  7
#define TEXT_WIDTH    300


Err PrepOverlay(StorageReq *req, Overlay *ov, OverlayTypes type, int16 startingX, int16 startingY, ...)
{
SpriteObj     *spr;
FontTextArray  fta[2];
FontTextArray *ft;
uint16         x, y;
char           text[300];
uint32         numLines;
uint32         i;
uint32         extra;
char          *title;
int16          xSpeed;
int16          ySpeed;
int16          widthSpeed;
int16          heightSpeed;
InfoPair      *ip;
List          *list;
Err            result;
StringExtent   se;
uint32         labelWidth;
uint32         colonWidth;
va_list        args;

	va_start(args, startingY);

    PrepAnimNode(req, (AnimNode *)ov, &overlayMethods, sizeof(Overlay));

    spr               = req->sr_OverlayImage;
    ov->ov_X          = (BG_WIDTH - Spr_GetWidth(spr)) / 2;
    ov->ov_Y          = (BG_HEIGHT - Spr_GetHeight(spr)) / 2 - 24;
    ov->ov_Width      = Spr_GetWidth(spr),
    ov->ov_Height     = Spr_GetHeight(spr),
    ov->ov_Type       = type;
    ov->ov_Label      = NULL;
    ov->ov_Stabilized = FALSE;
    ov->ov_DoText     = TRUE;

    xSpeed      = abs(startingX - ov->ov_X) * 4;
    ySpeed      = abs(startingY - ov->ov_Y) * 4;
    widthSpeed  = ov->ov_Width * 4;
    heightSpeed = ov->ov_Height * 4;
    list        = NULL;

    PrepTransition(&ov->ov_CurrentX,      TT_LINEAR, startingX, ov->ov_X, xSpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentY,      TT_LINEAR, startingY, ov->ov_Y, ySpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentWidth,  TT_LINEAR, 1, ov->ov_Width, widthSpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentHeight, TT_LINEAR, 1, ov->ov_Height, heightSpeed, &req->sr_CurrentTime);

    switch(type)
    {
        case OVL_CONFIRMDELETEFILE : 	vsprintf(text, MSG_OVL_CONFIRMDELETEFILE, args);
                                 		title = MSG_OVL_CONFIRMDELETE_TITLE;
                                 		break;

        case OVL_CONFIRMDELETEDIR : 	vsprintf(text, MSG_OVL_CONFIRMDELETEDIR, args);
                                 		title = MSG_OVL_CONFIRMDELETE_TITLE;
                                 		break;

        case OVL_CONFIRMDELETEFSYS : 	vsprintf(text, MSG_OVL_CONFIRMDELETEFSYS, args);
                                 		title = MSG_OVL_CONFIRMDELETE_TITLE;
                                 		break;

        case OVL_CONFIRMCOPY       : 	vsprintf(text, MSG_OVL_CONFIRMCOPY, args);
                                 		title = MSG_OVL_CONFIRMCOPY_TITLE;
                                 		break;

        case OVL_HIERARCHYCOPY       : 	vsprintf(text, MSG_OVL_HIERARCHY_COPY, args);
                                 		title = MSG_OVL_HIERARCHY_COPY_TITLE;
                                 		break;

        case OVL_ERROR         : 		sprintf(text, "%s", MSG_OVL_ERROR);
                                 		title = MSG_OVL_ERROR_TITLE;
                                 		break;

        case OVL_INFO          :		list  = va_arg(args, List *);
                                 		title = MSG_OVL_INFO_TITLE;
                                 		break;

        case OVL_DUPLICATE     : 		vsprintf(text, MSG_OVL_DUPLICATE, args);
                                 		title = MSG_OVL_DUPLICATE_TITLE;
                                 		break;

        case OVL_MEDIAFULL     : 		vsprintf(text, MSG_OVL_MEDIAFULL, args);
                                 		title = MSG_OVL_MEDIAFULL_TITLE;
                                 		break;

        case OVL_MEDIAPROTECTED: 		vsprintf(text, MSG_OVL_MEDIAPROTECTED, args);
                                 		title = MSG_OVL_MEDIAPROTECTED_TITLE;
                                 		break;

        case OVL_MEDIAREQUEST  : 		vsprintf(text, MSG_OVL_MEDIAREQUEST, args);
                                 		title = MSG_OVL_MEDIAREQUEST_TITLE;
                                		 break;

        case OVL_WORKING       : 		sprintf(text, MSG_OVL_WORKING);
                                 		title = MSG_OVL_WORKING_TITLE;
                                		 break;

#ifdef BUILD_PARANOIA
        default                : 		printf("ERROR: PrepOverlay with bogus overlay type %d\n",type);
                                 		return -1;
#endif
    }
    PlaySound(req, SOUND_HIDEOVERLAY);

    if (list)
    {
        numLines = 0;
        ScanList(list, ip, InfoPair)
        {
            numLines++;
        }

        ft = AllocMem((numLines*3 + 1) * sizeof(FontTextArray), MEMTYPE_NORMAL);
        if (!ft)
            return REQ_ERR_NOMEM;

        ft[0].fta_StructSize       = sizeof(FontTextArray);
        ft[0].fta_Pen.pen_X        = ov->ov_X + TEXT_LEFT_PAD;
        ft[0].fta_Pen.pen_Y        = ov->ov_Y + TEXT_TOP_PAD + req->sr_SampleChar.cd_Ascent;
        ft[0].fta_Pen.pen_FgColor  = TEXT_COLOR_OVERLAY_TITLE;
        ft[0].fta_Pen.pen_BgColor  = 0;
        ft[0].fta_Pen.pen_XScale   = 1.0;
        ft[0].fta_Pen.pen_YScale   = 1.0;
        ft[0].fta_Pen.pen_Flags    = 0;
        ft[0].fta_Pen.pen_reserved = 0;
        ft[0].fta_Clip.min.x       = 0.0;
        ft[0].fta_Clip.min.y       = 0.0;
        ft[0].fta_Clip.max.x       = BG_WIDTH;
        ft[0].fta_Clip.max.y       = BG_HEIGHT;
        ft[0].fta_String           = title;
        ft[0].fta_NumChars         = strlen(title);

        extra = ov->ov_Height - TITLE_HEIGHT - BUTTON_HEIGHT - ((req->sr_SampleChar.cd_CharHeight + req->sr_SampleChar.cd_Leading) * numLines);
        x     = ov->ov_X + TEXT_LEFT_PAD;
        y     = ov->ov_Y + TITLE_HEIGHT + (extra / 2) + req->sr_SampleChar.cd_Ascent;

        labelWidth = 0;
        ScanList(list, ip, InfoPair)
        {
            GetStringExtent(&se, req->sr_Font, &ft[0].fta_Pen, ip->ip_Label, strlen(ip->ip_Label));
            if (TEXTWIDTH(&se) > labelWidth)
                labelWidth = TEXTWIDTH(&se);
        }

        GetStringExtent(&se, req->sr_Font, &ft[0].fta_Pen, SEPARATOR, strlen(SEPARATOR));
        colonWidth = TEXTWIDTH(&se);

        i = 1;
        ScanList(list, ip, InfoPair)
        {
            ft[i]                       = ft[0];
            ft[i].fta_Pen.pen_X         = x;
            ft[i].fta_Pen.pen_Y         = y;
            ft[i].fta_Pen.pen_FgColor   = TEXT_COLOR_INFO_TEMPLATE;
            ft[i].fta_String            = ip->ip_Label;
            ft[i].fta_NumChars          = strlen(ip->ip_Label);

            ft[i+1]                     = ft[0];
            ft[i+1].fta_Pen.pen_X       = x + labelWidth;
            ft[i+1].fta_Pen.pen_Y       = y;
            ft[i+1].fta_Pen.pen_FgColor = TEXT_COLOR_INFO_TEMPLATE;
            ft[i+1].fta_String          = SEPARATOR;
            ft[i+1].fta_NumChars        = strlen(SEPARATOR);

            ft[i+2]                     = ft[0];
            ft[i+2].fta_Pen.pen_X       = x + labelWidth + colonWidth;
            ft[i+2].fta_Pen.pen_Y       = y;
            ft[i+2].fta_Pen.pen_FgColor = TEXT_COLOR_INFO_DATA;
            ft[i+2].fta_String          = ip->ip_Info;
            ft[i+2].fta_NumChars        = strlen(ip->ip_Info);

            y += req->sr_SampleChar.cd_CharHeight + req->sr_SampleChar.cd_Leading;
            i += 3;
        }

        result = CreateTextState(&ov->ov_Label, req->sr_Font, ft, numLines*3 + 1);

        FreeMem(ft, (numLines*3 + 1) * sizeof(FontTextArray));

        return result;
    }

    numLines = 1;
    i        = 0;

    while (text[i])
    {
        if (text[i] == '\n')
            numLines++; 

        i++;
    }

    fta[0].fta_StructSize       = sizeof(FontTextArray);
    fta[0].fta_Pen.pen_X        = ov->ov_X + TEXT_LEFT_PAD;
    fta[0].fta_Pen.pen_Y        = ov->ov_Y + TEXT_TOP_PAD + req->sr_SampleChar.cd_Ascent;
    fta[0].fta_Pen.pen_FgColor  = TEXT_COLOR_OVERLAY_TITLE;
    fta[0].fta_Pen.pen_BgColor  = 0;
    fta[0].fta_Pen.pen_XScale   = 1.0;
    fta[0].fta_Pen.pen_YScale   = 1.0;
    fta[0].fta_Pen.pen_Flags    = 0;
    fta[0].fta_Pen.pen_reserved = 0;
    fta[0].fta_Clip.min.x       = 0.0;
    fta[0].fta_Clip.min.y       = 0.0;
    fta[0].fta_Clip.max.x       = BG_WIDTH;
    fta[0].fta_Clip.max.y       = BG_HEIGHT;
    fta[0].fta_String           = title;
    fta[0].fta_NumChars         = strlen(title);

    extra = ov->ov_Height - TITLE_HEIGHT - BUTTON_HEIGHT - ((req->sr_SampleChar.cd_CharHeight + req->sr_SampleChar.cd_Leading) * numLines);
    x     = ov->ov_X + TEXT_LEFT_PAD;
    y     = ov->ov_Y + TITLE_HEIGHT + (extra / 2) + req->sr_SampleChar.cd_Ascent;

    fta[1]                     = fta[0];
    fta[1].fta_Pen.pen_X       = x;
    fta[1].fta_Pen.pen_Y       = y;
    fta[1].fta_Pen.pen_FgColor = TEXT_COLOR_OVERLAY_TEXT;
    fta[1].fta_String          = text;
    fta[1].fta_NumChars        = i;

	va_end(args);

    return CreateTextState(&ov->ov_Label, req->sr_Font, fta, 2);
}


/*****************************************************************************/


void UnprepOverlay(StorageReq *req, Overlay *ov)
{
    UnprepTransition(&ov->ov_CurrentX);
    UnprepTransition(&ov->ov_CurrentY);
    UnprepTransition(&ov->ov_CurrentWidth);
    UnprepTransition(&ov->ov_CurrentHeight);
    DeleteTextState(ov->ov_Label);
    UnprepAnimNode(req, (AnimNode *)ov);
}


/*****************************************************************************/


static void StepOverlay(StorageReq *req, Overlay *ov)
{
Point2 corners[4];

    StepAnimNode(req, (AnimNode *)ov);

    if (!ov->ov_Stabilized)
    {
        StepTransition(&ov->ov_CurrentX, &req->sr_CurrentTime);
        StepTransition(&ov->ov_CurrentY, &req->sr_CurrentTime);
        StepTransition(&ov->ov_CurrentWidth, &req->sr_CurrentTime);
        StepTransition(&ov->ov_CurrentHeight, &req->sr_CurrentTime);

        corners[0].x = ov->ov_CurrentX.ti_Current;
        corners[0].y = ov->ov_CurrentY.ti_Current;
        corners[1].x = ov->ov_CurrentX.ti_Current + ov->ov_CurrentWidth.ti_Current - 1;
        corners[1].y = ov->ov_CurrentY.ti_Current;
        corners[2].x = ov->ov_CurrentX.ti_Current + ov->ov_CurrentWidth.ti_Current - 1;
        corners[2].y = ov->ov_CurrentY.ti_Current + ov->ov_CurrentHeight.ti_Current - 1;
        corners[3].x = ov->ov_CurrentX.ti_Current;
        corners[3].y = ov->ov_CurrentY.ti_Current + ov->ov_CurrentHeight.ti_Current - 1;
        Spr_SetCorners(req->sr_OverlayImage, corners);

        if ((ov->ov_CurrentX.ti_Current == ov->ov_X) &&
            (ov->ov_CurrentY.ti_Current == ov->ov_Y) &&
            (ov->ov_CurrentWidth.ti_Current == ov->ov_Width) &&
            (ov->ov_CurrentHeight.ti_Current == ov->ov_Height))
        {
            ov->ov_Stabilized = TRUE;
        }
    }
}


/*****************************************************************************/


static void DrawOverlay(StorageReq *req, Overlay *ov, GState *gs)
{
    F2_Draw(gs, req->sr_OverlayImage);

    if (ov->ov_Label && ov->ov_Stabilized && ov->ov_DoText)
        DrawText(gs, ov->ov_Label);
}


/*****************************************************************************/


void MoveOverlay(StorageReq *req, Overlay *ov, int16 x, int16 y)
{
int16 xSpeed;
int16 ySpeed;
int16 widthSpeed;
int16 heightSpeed;

    ov->ov_X          = x;
    ov->ov_Y          = y;
    ov->ov_Width      = 1;
    ov->ov_Height     = 1;
    ov->ov_Stabilized = FALSE;
    ov->ov_DoText     = FALSE;

    xSpeed      = abs(ov->ov_CurrentX.ti_Current - ov->ov_X) * 4;
    ySpeed      = abs(ov->ov_CurrentY.ti_Current - ov->ov_Y) * 4;
    widthSpeed  = ov->ov_CurrentWidth.ti_Current * 4;
    heightSpeed = ov->ov_CurrentHeight.ti_Current * 4;

    PrepTransition(&ov->ov_CurrentX,      TT_LINEAR, ov->ov_CurrentX.ti_Current, ov->ov_X, xSpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentY,      TT_LINEAR, ov->ov_CurrentY.ti_Current, ov->ov_Y, ySpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentWidth,  TT_LINEAR, ov->ov_CurrentWidth.ti_Current, ov->ov_Width, widthSpeed, &req->sr_CurrentTime);
    PrepTransition(&ov->ov_CurrentHeight, TT_LINEAR, ov->ov_CurrentHeight.ti_Current, ov->ov_Height, heightSpeed, &req->sr_CurrentTime);

    PlaySound(req, SOUND_SHOWOVERLAY);
}
