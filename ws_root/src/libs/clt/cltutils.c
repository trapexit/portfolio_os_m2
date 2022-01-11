
/******************************************************************************
**
**  @(#) cltutils.c 96/07/25 1.10
**
**  Misc. utilities which you may find useful
**
******************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>


#define SRCTODEST_SIZE		9


void CLT_SetSrcToCurrentDest(GState *g)
{
    Bitmap *bm;

    bm = (Bitmap*)LookupItem(GS_GetDestBuffer(g));

    GS_Reserve(g, SRCTODEST_SIZE);

    CLT_Sync (GS_Ptr(g));

    CLT_WriteRegister((*GS_Ptr(g)),DBSRCBASEADDR,bm->bm_Buffer);

    CLT_DBSRCXSTRIDE (GS_Ptr(g), bm->bm_Width);
    CLT_DBSRCOFFSET (GS_Ptr(g), 0, 0);
    CLT_DBSRCCNTL(GS_Ptr(g), 1, (bm->bm_Type==BMTYPE_32));
}

