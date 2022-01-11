/* @(#) utils.c 96/09/29 1.3 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <graphics/clt/gstate.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clt.h>
#include <graphics/pipe/gfxtypes.h>
#include <graphics/pipe/vec.h>
#include <graphics/pipe/col.h>
#include <string.h>


/*****************************************************************************/


bool IsFilesystemName(const char *name)
{
	/* if the right-most slash is the start of the name, it's a filesystem */
	
	return (strrchr(name, '/') == name);
}


/*****************************************************************************/


void stccpy(char *to, const char *from, uint32 maxChars)
{
uint32 i;

    if (maxChars == 0)
        return;

    i = 0;
    while (from[i] && (i < (maxChars - 1)))
        to[i] = from[i++];

    to[i] = 0;
}


/*****************************************************************************/


Err GetFullPath(char *path, uint32 numBytes)
{
Err    result;
Item   file;
Item   ioreq;
IOInfo ioInfo;

    file = result = OpenFile(path);
    if (file >= 0)
    {
        ioreq = result = CreateIOReq(NULL,0,file,0);
        if (ioreq >= 0)
        {
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = FILECMD_GETPATH;
            ioInfo.ioi_Recv.iob_Buffer = path;
            ioInfo.ioi_Recv.iob_Len    = numBytes;
            result = DoIO(ioreq,&ioInfo);

            DeleteIOReq(ioreq);
        }
        CloseFile(file);
    }

    return result;
}


/*****************************************************************************/


void SetOutClipping(GState *gs, uint32 x0, uint32 y0, uint32 x1, uint32 y1)
{
    GS_Reserve(gs, 8);
    CLT_Sync(GS_Ptr(gs));
    CLT_DBXYWINCLIP(GS_Ptr(gs), x0, x1 + 1, y0, y1 + 1);
    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, FV_DBUSERCONTROL_WINCLIPOUTEN_MASK);
}


/*****************************************************************************/


void SetInClipping(GState *gs, uint32 x0, uint32 y0, uint32 x1, uint32 y1)
{
    GS_Reserve(gs, 8);
    CLT_Sync(GS_Ptr(gs));
    CLT_DBXYWINCLIP(GS_Ptr(gs), x0, x1 + 1, y0, y1 + 1);
    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, FV_DBUSERCONTROL_WINCLIPINEN_MASK);
}


/*****************************************************************************/


void ClearClipping(GState *gs)
{
    GS_Reserve(gs, 4);
    CLT_Sync(GS_Ptr(gs));
    CLT_ClearRegister(gs->gs_ListPtr, DBUSERCONTROL, FV_DBUSERCONTROL_WINCLIPOUTEN_MASK |
                                                     FV_DBUSERCONTROL_WINCLIPINEN_MASK);
}


/*****************************************************************************/


#define COPYRESERVE(gs,list) GS_Reserve((gs),(list)->size); CLT_CopySnippetData (GS_Ptr(gs),(list));

static const uint32 _prllist[] =
{
    CLT_SetRegistersHeader(DBUSERCONTROL,1),
    CLA_DBUSERCONTROL (0, 0, 0, 0, 0, 0, 0,
		     (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) |
		      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) |
		      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) |
		      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA))),

    CLT_WriteRegistersHeader(DBAMULTCNTL,1),
    CLA_DBAMULTCNTL(CLT_Const(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR),
		  CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, TEXALPHA),
		  CLT_Const(DBAMULTCNTL, AMULTCONSTCONTROL, TEXSSB),
		  0),

    CLT_WriteRegistersHeader(DBDESTALPHACNTL,1),
    CLA_DBDESTALPHACNTL(CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,TEXALPHA)),
};

static const CltSnippet _prlsnippet =
{
    &_prllist[0],
    sizeof(_prllist)/sizeof(uint32),
};


void ShadedStrip(GState *gs, uint32 numPoints, struct Point2 *p, Color4 *c)
{
Bitmap *bm;

    bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));

    /* turn on alpha blending */
    GS_Reserve(gs, 14);
    CLT_Sync(GS_Ptr(gs));
    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, FV_DBUSERCONTROL_BLENDEN_MASK | FV_DBUSERCONTROL_SRCEN_MASK);
    CLT_WriteRegister(gs->gs_ListPtr, DBSRCBASEADDR, bm->bm_Buffer);
    CLT_WriteRegister(gs->gs_ListPtr, DBSRCXSTRIDE,  bm->bm_Width);
    CLT_WriteRegister(gs->gs_ListPtr, DBAMULTCNTL,   CLT_SetConst(DBAMULTCNTL,AINPUTSELECT,TEXCOLOR) | CLT_SetConst(DBAMULTCNTL,AMULTCOEFSELECT,TEXALPHA));
    CLT_WriteRegister(gs->gs_ListPtr, DBBMULTCNTL,   CLT_SetConst(DBBMULTCNTL,BINPUTSELECT,SRCCOLOR) | CLT_SetConst(DBBMULTCNTL,BMULTCOEFSELECT,TEXALPHACOMPLEMENT));
    CLT_WriteRegister(gs->gs_ListPtr, DBALUCNTL,     CLT_SetConst(DBALUCNTL,ALUOPERATION,A_PLUS_BCLAMP));

    COPYRESERVE(gs, &CltNoTextureSnippet);
    COPYRESERVE(gs, &_prlsnippet);

    GS_Reserve(gs, numPoints*7+1);
    CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 1, 0, 1, numPoints);
    while (numPoints--)
    {
        CLT_VertexRgbaW(GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, .999998);
        p++;
        c++;
    }
}


/*****************************************************************************/


void ShadedRect(GState *gs, int32 x1, int32 y1, int32 x2, int32 y2,
                const Color4 *c)
{
Bitmap *bm;

    bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));

    /* turn on alpha blending */
    GS_Reserve(gs, 12);
    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, FV_DBUSERCONTROL_BLENDEN_MASK | FV_DBUSERCONTROL_SRCEN_MASK);
    CLT_WriteRegister(gs->gs_ListPtr, DBSRCBASEADDR, bm->bm_Buffer);
    CLT_WriteRegister(gs->gs_ListPtr, DBSRCXSTRIDE,  bm->bm_Width);
    CLT_WriteRegister(gs->gs_ListPtr, DBAMULTCNTL,   CLT_SetConst(DBAMULTCNTL,AINPUTSELECT,TEXCOLOR) | CLT_SetConst(DBAMULTCNTL,AMULTCOEFSELECT,TEXALPHA));
    CLT_WriteRegister(gs->gs_ListPtr, DBBMULTCNTL,   CLT_SetConst(DBBMULTCNTL,BINPUTSELECT,SRCCOLOR) | CLT_SetConst(DBBMULTCNTL,BMULTCOEFSELECT,TEXALPHACOMPLEMENT));
    CLT_WriteRegister(gs->gs_ListPtr, DBALUCNTL,     CLT_SetConst(DBALUCNTL,ALUOPERATION,A_PLUS_BCLAMP));

    COPYRESERVE(gs, &CltNoTextureSnippet);
    COPYRESERVE(gs, &_prlsnippet);

    GS_Reserve(gs, 8*4+1);
    CLT_TRIANGLE(GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 4);
    CLT_VertexRgbaW(GS_Ptr(gs), x1, y1, c[0].r, c[0].g, c[0].b, c[0].a, .999998);
    CLT_VertexRgbaW(GS_Ptr(gs), x1, y2, c[3].r, c[3].g, c[3].b, c[3].a, .999998);
    CLT_VertexRgbaW(GS_Ptr(gs), x2, y2, c[2].r, c[2].g, c[2].b, c[2].a, .999998);
    CLT_VertexRgbaW(GS_Ptr(gs), x2, y1, c[1].r, c[1].g, c[1].b, c[1].a, .999998);
}
