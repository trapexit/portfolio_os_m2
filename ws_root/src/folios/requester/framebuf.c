/* @(#) framebuf.c 96/09/12 1.4 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <file/filefunctions.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <graphics/clt/gstate.h>
#include <stdio.h>
#include "req.h"
#include "framebuf.h"
#include "bg.h"


/*****************************************************************************/


Err CreateFrameBuffers(StorageReq *req)
{
uint32  i;
Bitmap *bm;
void   *mem;
Err     result;

    for (i = 0; i < 2; i++)
        req->sr_FrameBuffers[i] = -1;

    result = -1;                                    /* keep compiler quiet */

    for (i = 0; i < 2; i++)
    {
        req->sr_FrameBuffers[i] = result = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
                                               BMTAG_WIDTH,    640,
                                               BMTAG_HEIGHT,      240,
                                               BMTAG_TYPE,      BMTYPE_16,
                                               BMTAG_DISPLAYABLE, TRUE,
                                               BMTAG_RENDERABLE,  TRUE,
                                               TAG_END);
        if (req->sr_FrameBuffers[i] >= 0)
        {
            bm = (Bitmap *)LookupItem(req->sr_FrameBuffers[i]);

            mem = AllocMemMasked(bm->bm_BufferSize, bm->bm_BufMemType, bm->bm_BufMemCareBits, bm->bm_BufMemStateBits);

            if (mem)
            {
                result = ModifyGraphicsItemVA(req->sr_FrameBuffers[i], BMTAG_BUFFER, mem, TAG_END);
                if (result >= 0)
                    continue;
            }
            else
            {
                result = REQ_ERR_NOMEM;
            }
            FreeMem(mem, bm->bm_BufferSize);
        }
        break;
    }

    if (result > 0)
    {
        req->sr_GState = GS_Create();

        if (req->sr_GState)
        {
            result = GS_AllocLists(req->sr_GState, 2, 1024);

            if (result >= 0)
            {
                GS_SetDestBuffer(req->sr_GState, req->sr_FrameBuffers[1]);              /* erase the second buffer, since that's what we're gonna display first */
                CLT_SetSrcToCurrentDest(req->sr_GState);
                CLT_ClearFrameBuffer(req->sr_GState, 0.0, 0.0, 0.0, 0.0, TRUE, FALSE);
                GS_SendList(req->sr_GState);
                GS_WaitIO(req->sr_GState);

                req->sr_CurrentFrameBuffer = 0;                                         /* setup to render into buffer 0 */
                GS_SetDestBuffer(req->sr_GState, req->sr_FrameBuffers[req->sr_CurrentFrameBuffer]);
            }
        }
        else
        {
            result = REQ_ERR_NOMEM;
        }
    }

    if (result < 0)
        DeleteFrameBuffers(req);

    return result;
}


/*****************************************************************************/


void DeleteFrameBuffers(StorageReq *req)
{
uint32  i;
Bitmap *bm;

    GS_Delete(req->sr_GState);

    for (i = 0; i < 2; i++)
    {
        bm = (Bitmap *)LookupItem(req->sr_FrameBuffers[i]);

        if (bm)
        {
            FreeMem(bm->bm_Buffer, bm->bm_BufferSize);
            ModifyGraphicsItemVA(req->sr_FrameBuffers[i], BMTAG_BUFFER, NULL, TAG_END);
            DeleteItem(req->sr_FrameBuffers[i]);
        }
    }
}


/*****************************************************************************/


#if 0
void SnapshotFrameBuf(StorageReq *req, const char *name)
{
RawFile *file;
uint16  *pixel;
Bitmap  *bm;
uint32   num;
Err      result;
uint8    red, green, blue;

    result = OpenRawFile(&file, name, FILEOPEN_WRITE_NEW);

    if (result >= 0)
    {
        bm  = (Bitmap *)LookupItem(req->sr_FrameBuffers[req->sr_CurrentFrameBuffer]);
        num   = bm->bm_BufferSize / 2;
        pixel = bm->bm_Buffer;
        while (num)
        {
            red   = ((*pixel) >> 10) & 0x1f;
            green = ((*pixel) >> 5) & 0x1f;
            blue  = (*pixel) & 0x1f;

            red   = (red << 3) | (red >> 2);
            green = (green << 3) | (red >> 2);
            blue  = (blue << 3) | (red >> 2);

            WriteRawFile(file, &red,   1);
            WriteRawFile(file, &green, 1);
            WriteRawFile(file, &blue,  1);

            num--;
            pixel++;
        }

        CloseRawFile(file);
    }
}
#endif


/*****************************************************************************/


void DoNextFrame(StorageReq *req, AnimList *animList)
{
    SampleSystemTimeTV(&req->sr_CurrentTime);
    StepAnimList(req, animList);
    DrawAnimList(req, animList, req->sr_GState);
    GS_SendList(req->sr_GState);
    GS_WaitIO(req->sr_GState);

    ModifyGraphicsItemVA(req->sr_View,
                         VIEWTAG_BITMAP, req->sr_FrameBuffers[req->sr_CurrentFrameBuffer],
                         TAG_END);

    req->sr_CurrentFrameBuffer ^= 1;
    GS_SetDestBuffer(req->sr_GState, req->sr_FrameBuffers[req->sr_CurrentFrameBuffer]);
    CLT_SetSrcToCurrentDest(req->sr_GState);
}
