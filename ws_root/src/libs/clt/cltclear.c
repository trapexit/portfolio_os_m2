
/******************************************************************************
**
**  @(#) cltclear.c 96/07/09 1.15
**
**  Clear the screen and/or frame buffer to the specified color
**
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>

#define CLEAR_SIZE	41		/* size, not incl. NoTextureSnippet */

void CLT_ClearFrameBuffer(GState *gs, float red, float green, float blue,
			  float alpha, bool clearScreen, bool clearZ)
{
    Bitmap		*bm;
    uint32		width, height;
    uint32		clearFlags;
#ifdef BUILD_PARANOIA
    CmdListP		origListPtr;
#endif

    bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
    width = bm->bm_Width;
    height = bm->bm_Height;

    GS_Reserve(gs, CLEAR_SIZE+CltNoTextureSnippet.size);
    CLT_CopySnippetData(GS_Ptr(gs), &CltNoTextureSnippet);
#ifdef BUILD_PARANOIA
    origListPtr = gs->gs_ListPtr;
#endif

    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL,
		    CLT_Mask(DBUSERCONTROL, DESTOUTMASK));

    clearFlags = (CLT_Mask(DBUSERCONTROL, BLENDEN) |
		  CLT_Mask(DBUSERCONTROL, SRCEN));
    if (!clearZ)
	clearFlags |= (CLT_Mask(DBUSERCONTROL, ZBUFFEN) |
		       CLT_Mask(DBUSERCONTROL, ZOUTEN));

    CLT_ClearRegister(gs->gs_ListPtr, DBUSERCONTROL, clearFlags);
    CLT_DBDISCARDCONTROL(GS_Ptr(gs), 0, 0, 0, 0);
    if (clearZ) {
	CLT_DBZCNTL(GS_Ptr(gs), 1,clearScreen,1,clearScreen,1,clearScreen);
	CLT_DBZOFFSET(GS_Ptr(gs), 0, 0);
    }

    CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 1, 0, 1, 4);
    CLT_VertexRgbaW(GS_Ptr(gs), 0.0,   0.0,    red, green, blue, alpha, 0.0);
    CLT_VertexRgbaW(GS_Ptr(gs), 0.0,   height, red, green, blue, alpha, 0.0);
    CLT_VertexRgbaW(GS_Ptr(gs), width, 0.0,    red, green, blue, alpha, 0.0);
    CLT_VertexRgbaW(GS_Ptr(gs), width, height, red, green, blue, alpha, 0.0);
    CLT_Sync(GS_Ptr(gs));
#ifdef BUILD_PARANOIA
    assert (gs->gs_ListPtr <= (origListPtr + CLEAR_SIZE));
#endif
}
