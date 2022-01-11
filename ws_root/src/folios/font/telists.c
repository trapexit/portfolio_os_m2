/* @(#) telists.c 96/08/20 1.25 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include "fonttable.h"
#include <stdio.h>

#define DPRT(x) /* printf x */
#define DPRTTE(x) /*printf x*/
#define SHOWERR(s, x) /*{if ((x) < 0) {printf("%s\n", s); PrintfSysErr(x);}}*/

/* Each character requires 1 uint32 for the triangle type,
 * 5 uint32s for the x,y,u,v,w info,
 * and there are 4 vertices for each character.
 */
#define CHAR_SIZE (21 * (sizeof(uint32)))
#define TEXTURE_SIZE (sizeof(TextureLoadList))
#define INIT_SIZE (sizeof(InitList))
#define DBLEND_SIZE (sizeof(DBlendList))
/* Need 3 uint32s for the jump instruction */
#define RET_SIZE (3 * sizeof(uint32))
/* Need 3 uint32s for the sync instruction */
#define SYNC_SIZE (4 * sizeof(uint32))/* 2 Sync instructions used */
/* Need 11 instructions to set up the colour registers:
 * assume a colour change for every string for every character range
 */
#define COLOUR_SIZE ((11 * sizeof(uint32)) * arrayCount * rangeCount)
/* Need this many bytes to disable texture mapping and draw a solid rectangle.
 * This is the standard CltNoTextureSnippet, Steve Landrum's prllist snippet,
 * + 4 xyrgbaw vertices.
 */
#define RECT_SIZE(rect) ((CLT_GetSize(&CltNoTextureSnippet) * sizeof(uint32)) + \
                         sizeof(prllist) + \
                         (29 * sizeof(uint32) * rect))

#define FONT_CHARACTERS(r) (r->maxChar - r->minChar + 1)

static uint32 prllist[] =
{
  CLT_SetRegistersHeader(DBUSERCONTROL, 1),
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
  
  CLT_WriteRegistersHeader(DBDESTALPHACNTL, 1),
  CLA_DBDESTALPHACNTL(CLT_Const(DBDESTALPHACNTL, DESTCONSTSELECT, TEXALPHA)),
};

static CltSnippet prlsnippet =
{
  &prllist[0],
  sizeof(prllist)/sizeof(uint32),
};

#define STORE_VERTEX(v, t, x, y, f) if (v) v = StoreVertex(v, t, x, y, f->fta_Pen.pen_X, f->fta_Pen.pen_Y);
#define STORE_VERTEX_END(v) if (v){v->vertex = 0;}

vertexInfo *StoreVertex(vertexInfo *v, uint32 *t, gfloat x, gfloat y, gfloat x1, gfloat y1)
{
    v->vertex = t;
    v->dx = (x - x1);
    v->dy = (y - y1);
    v->flags = 0;
    return(++v);
}

Err CreateTELists(TextState *ts, renderTable *table, uint32 rangeCount,
                  uint32 totalChars, uint32 arrayCount, FontTextArray *fta)
{
    renderTable *nextTable = table;
    renderTableHeader *rth;
    charInfo *ci;
    vertexInfo *nextVertex = NULL;
    uint32 i, r;
    uint32 lastFg;
    uint32 tesize;
    uint32 rectCount = 0;
    CmdListP te;
    
    /* Init list is a list of instructions to set registers
     * that will remain constant whilst we render all the
     * characters.
     */
    uint32 InitList[] =
    {
        CLT_WriteRegistersHeader(TXTEXPTYPE, 1),
        (CLT_Bits(TXTEXPTYPE, HASALPHA, 1) | 
         CLT_Bits(TXTEXPTYPE, ADEPTH, 4)),    /* [1] Change this to the bpp value from the font */

        CLT_WriteRegistersHeader(TXTLDCNTL, 1),
        CLT_SetConst(TXTLDCNTL, LOADMODE, MMDMA),

        CLT_WriteRegistersHeader(TXTADDRCNTL,1),
        (CLT_Bits(TXTADDRCNTL, TEXTUREENABLE, 1) |
         CLT_SetConst(TXTADDRCNTL, MINFILTER, POINT) |
         CLT_SetConst(TXTADDRCNTL, INTERFILTER, POINT) |
         CLT_SetConst(TXTADDRCNTL, MAGFILTER, POINT) |
         CLT_Bits(TXTADDRCNTL, LODMAX, 0)),
        
        CLT_WriteRegistersHeader(TXTPIPCNTL,1),
        CLA_TXTPIPCNTL(CLT_Const(TXTPIPCNTL,PIPSSBSELECT,CONSTANT),
                       CLT_Const(TXTPIPCNTL,PIPALPHASELECT,TEXTURE),
                       CLT_Const(TXTPIPCNTL,PIPCOLORSELECT,CONSTANT),
		       0 /* index offset */),

        CLT_WriteRegistersHeader(TXTTABCNTL, 1),
        (CLT_SetConst(TXTTABCNTL, FIRSTCOLOR, PRIMALPHA) |
         CLT_SetConst(TXTTABCNTL, SECONDCOLOR, PRIMALPHA) |
         CLT_SetConst(TXTTABCNTL, THIRDCOLOR, PRIMALPHA) |
         CLT_SetConst(TXTTABCNTL, COLOROUT, TEXCOLOR) |
         CLT_SetConst(TXTTABCNTL, FIRSTALPHA, PRIMALPHA) |
         CLT_SetConst(TXTTABCNTL, SECONDALPHA, PRIMALPHA) |
         CLT_SetConst(TXTTABCNTL, ALPHAOUT, TEXALPHA) |
         CLT_SetConst(TXTTABCNTL, BLENDOP, LERP)),

        CLT_SetRegistersHeader(DBUSERCONTROL,1),
        (CLT_Bits(DBUSERCONTROL,BLENDEN,1) |
         CLT_Bits(DBUSERCONTROL,SRCEN,1) |
         CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,ALPHA) |
         CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,RED) |
         CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,GREEN) |
         CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,BLUE)),
        
        CLT_WriteRegistersHeader(DBSSBDSBCNTL, 3),
        0,          /* DBSSBDSBCNTL */
        0,          /* DBCONSTIN */
        0,          /* DBAMULTCNTL */
        
        CLT_WriteRegistersHeader(DBALUCNTL, 1),
        0,
    };
    CltSnippet InitSnippet =
    {
        NULL,
        sizeof(InitList)/sizeof(uint32),
    };
    /* DBlend list will be set up in DrawText() when the destination buffer is known */
    uint32 DBlendList[] =
    {
        CLT_WriteRegistersHeader(DBSRCCNTL, 4),
        0,          /* DBSRCCNTL */
        0,         /* DBSRCBASEADDR */
        0,        /* DBSRCXSTRIDE */
        0,        /* DBSRCOFFSET */
    };
    CltSnippet DBlendSnippet =
    {
        NULL,
        sizeof(DBlendList)/sizeof(uint32),
    };
    
    uint32 TextureLoadList[] = 
    {
        CLT_WriteRegistersHeader(DCNTL, 1),
        CLT_Bits(DCNTL, SYNC, 1),

        CLT_WriteRegistersHeader(TXTLDSRCADDR, 1),
        0,			/* [3] stick texture address here */
       
        CLT_WriteRegistersHeader(TXTLODBASE0, 1),
        0,

        CLT_WriteRegistersHeader(TXTCOUNT, 1),
        0,			/* [7] stick true count of texture bytes here */

        CLT_WriteRegistersHeader(DCNTL, 1),
        CLT_Bits(DCNTL, TLD, 1),
        
        CLT_WriteRegistersHeader(TXTLODBASE0, 1),
        0,
        
        CLT_WriteRegistersHeader(TXTUVMAX, 2),
        (CLT_Bits(TXTUVMAX, UMAX, 0x3f) |      /* [13] Stick true texture size here */
         CLT_Bits(TXTUVMAX, VMAX, 0x3f)),
        (CLT_Bits(TXTUVMASK, UMASK, 0x3ff) |
         CLT_Bits(TXTUVMASK, VMASK, 0x3ff)),
    };
    CltSnippet TextureSnippet =
    {
        NULL,
        sizeof(TextureLoadList)/sizeof(uint32),
    };

    DPRT(("CalcTELists() - totalChars = %ld, ranges = %ld, arrays = %ld\n", totalChars,
          rangeCount, arrayCount));
    
        /* Go through the render table, and build the TE list for each character.
         * We will then add a JMP instruction to the App's GState to execute these
         * instructions.
         */

    rth = (renderTableHeader *)nextTable;
    if (rth->texel == NULL)
    {
        rectCount = rth->charCnt;
    }

    
        /* Calculate the RAM needed to render these strings: */
    tesize = ((totalChars * CHAR_SIZE) +            /* Bytes for all the vertices */
              (rangeCount * TEXTURE_SIZE) +         /* Bytes to set up all the textures */
              (arrayCount * COLOUR_SIZE) +          /* Bytes to set up the colour registers */
              RECT_SIZE(rectCount) +                /* Bytes for the solid rectangles */
              INIT_SIZE +                           /* Bytes to initialise the TE */
              DBLEND_SIZE +                           /* Bytes to initialise the DBlender */
              RET_SIZE +                            /* Bytes for the JMP back */
              SYNC_SIZE);                           /* Bytes for the SYNC */
    if ((ts->ts_TEList == NULL) || /* Came from CreateTextState() */
        (ts->ts_TESize < tesize))  /* Came from DrawString() but buffer is too small */
    {
        te = (CmdListP)AllocMem(tesize, 0);
        if (te == NULL)
        {
            return(FONT_ERR_NOMEM);
        }
        DPRTTE(("TE List at 0x%lx, size 0x%lx (last = 0x%lx)\n", te, tesize, ((uint32)te + tesize - 4)));
    
        ts->ts_TEList = (void *)te;
        ts->ts_TESize = tesize;
    }
    else
    {
        te = ts->ts_TEList;
    }
    
    if (ts->ts_Size)
    {
        nextVertex = (vertexInfo *)&ts->ts_Vertex;
    }
    
        /* Initialise the TE */
    if (rectCount)
    {
        /* render all the rectangles */
        bgRectInfo *ri = (bgRectInfo *)&table->toRender;
        gfloat r, g ,b;

        CLT_CopySnippetData(&te, &CltNoTextureSnippet);
        CLT_CopySnippetData(&te, &prlsnippet);
        for (i = 0; i < rectCount; i++)
        {
            if (!ri->clipped)
            {
                DPRT(("Rendering box in 0x%lx at (%g, %g) - (%g, %g). w = %g\n", ri->rgb,
                      ri->box.min.x, ri->box.min.y, ri->box.max.x, ri->box.max.y, ri->w));
        
                r = ((ri->rgb >> 16) / 255.0);
                g = (((ri->rgb >> 8) & 0xff) / 255.0);
                b = ((ri->rgb & 0xff) / 255.0);
        
                STORE_VERTEX(nextVertex, te, ri->box.min.x, ri->box.min.y, fta);
                CLT_TRIANGLE (&te, 1, RC_FAN, 1, 0, 1, 4);
                CLT_VertexRgbaW(&te, ri->box.min.x, ri->box.min.y, r, g, b, 1.0, ri->w);
                CLT_VertexRgbaW(&te, ri->box.min.x, ri->box.max.y, r, g, b, 1.0, ri->w);
                CLT_VertexRgbaW(&te, ri->box.max.x, ri->box.max.y, r, g, b, 1.0, ri->w);
                CLT_VertexRgbaW(&te, ri->box.max.x, ri->box.min.y, r, g, b, 1.0, ri->w);
            }
            ri++;
        }
        rth = (renderTableHeader *)ri;
    }
        
    CLT_Sync(&te);
    InitSnippet.data = &InitList[0],
    InitList[1] = (CLT_Bits(TXTEXPTYPE, HASALPHA, 1) |
                   CLT_Bits(TXTEXPTYPE, ADEPTH, rth->bpp));
    CLT_CopySnippetData(&te, &InitSnippet);
    DBlendSnippet.data = &DBlendList[0];
    ts->ts_dBlend = te;
    CLT_CopySnippetData(&te, &DBlendSnippet);

    lastFg = 0xffffffff;
    for (r = 0; r < rangeCount; r++)
    {
        uint32 foo;

            /* Load texture for this range of characters */
        DPRT(("looking at range %c (0x%lx) - %c (0x%lx)\n", rth->minChar, rth->minChar, rth->maxChar, rth->maxChar));
        
        if (rth->charCnt)
        {
            TextureSnippet.data = &TextureLoadList[0];
            TextureLoadList[3] = (uint32)rth->texel;
            TextureLoadList[7] = rth->bytesToLoad;
            TextureLoadList[13] = (CLT_Bits(TXTUVMAX, UMAX,
                                            (ts->ts_Font->fd_bytesPerRow *
                                             (8 / rth->bpp) - 1)) |
                                   CLT_Bits(TXTUVMAX, VMAX, (rth->height *
                                                             FONT_CHARACTERS(rth) - 1)));
            DPRT(("Data @ 0x%lx\n", rth->texel));
            CLT_CopySnippetData(&te, &TextureSnippet);
            
                /* The next character entry starts immediately after this header */ 
            foo = (uint32)rth;
            foo += sizeof(renderTableHeader);
            ci = (charInfo *)foo;
            /* Set the vertices and colours of the characters in this range */
            for (i = 0; i < rth->charCnt; i++)
            {
                if (ci->FgColor != lastFg)
                {
                    /* Change the colours */
                    DPRT(("Changing colours to 0x%lx, 0x%lx\n", ci->FgColor, ci->BgColor));      
                    CLT_Sync(&te);
                    ts->ts_colorChangeCnt++;
                    CLT_Write2Registers(te, TXTCONST0, ci->FgColor, ci->FgColor);
                    CLT_WriteRegister(te, DBCONSTIN, ci->BgColor);
                    CLT_WriteRegister(te, DBDISCARDCONTROL,
                                      CLT_Bits(DBDISCARDCONTROL, ALPHA0, 1));
                    CLT_WriteRegister(te, DBBMULTCNTL,
                                      CLT_SetConst(DBBMULTCNTL, BINPUTSELECT, SRCCOLOR) |
                                      CLT_SetConst(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHACOMPLEMENT));
                    lastFg = ci->FgColor;
                }
                
                    /* This is a strip triangle, no perspective, textured,
                     * unshaded, with 4 vertices.
                     */
                DPRT(("Rendering character %c at (%g, %g, %g)\n", ci->entry, ci->X, ci->Y, ci->W));
                STORE_VERTEX(nextVertex, te, ci->X, ci->Y, fta);
                CLT_TRIANGLE(&te, 1, RC_FAN, 1, 1, 0, 4);
                CLT_VertexUvW(&te, ci->X, ci->Y, 0, ci->offset, ci->W);
                CLT_VertexUvW(&te, (ci->X + ci->width), ci->Y, ci->twidth, ci->offset, ci->W);
                CLT_VertexUvW(&te, (ci->X + ci->width), (ci->Y + ci->height),
                              ci->twidth, (ci->offset + rth->height), ci->W);
                CLT_VertexUvW(&te, ci->X, (ci->Y + ci->height), 0, (ci->offset + rth->height), ci->W);

                /* On to the next character */
                ci++;
            }
            /* Look at the next renderTableHeader */
            rth = (renderTableHeader *)ci;
        }
        else
        {
            DPRT(("No characters in this range!!\n"));      
            rth++;
        }
    }
    
    CLT_Sync(&te);
    
        /* Note where we will add the JumpAbsolute instruction */
    DPRT(("Put the ret instruction at 0x%lx\n", te));
    ts->ts_TERet = te;
    STORE_VERTEX_END(nextVertex);

#ifdef BUILD_PARANOIA
    if ((uint32)te > ((uint32)ts->ts_TEList + ts->ts_TESize - 8))
    {
        printf("**** WARNING te = 0x%lx, list @ 0x%lx, size 0x%lx\n", te, ts->ts_TEList, ts->ts_TESize);
        WaitSignal(0);
    }
#endif
    
    return(0);
}

/**
|||	AUTODOC -public -class Font -name DrawText
|||	Draws the text strings into the GState.
|||
|||	  Synopsis
|||
|||	    Err DrawText(GState *gs, TextState *ts);
|||
|||	  Description
|||
|||	    Links the pre-computed Triangle Engine lists created by
|||	    CreateTextState() into the GState, for rendering with
|||	    GS_SendList() and GS_WaitIO().
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState.
|||
|||	    ts
|||	        The TextState to render.
|||
|||	  Return Value
|||	    Negative value returned if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextState()
|||
**/

Err DrawText(GState *gs, TextState *ts)
{
    uint32 *ret;
    Item bmI;
    Bitmap *bm;
    uint32 *db;
    
    /* Calculate the DestinatioBlend registers */
    bmI = GS_GetDestBuffer(gs);
    if (bmI < 0)
    {
        return((Err)bmI);
    }
    bm = (Bitmap *)LookupItem(bmI);
    db = ts->ts_dBlend;
        /* DBSRCCNTL */
    db++;   /* Skip the instruction */
    *db = ((bm->bm_Type == BMTYPE_32) ?
           CLA_DBSRCCNTL(0, 1) : CLA_DBSRCCNTL(1, 0));
        /* DBSRCBASEADDR */
    db++;
    *db = (uint32)bm->bm_Buffer;
        /* DBSRCXSTRIDE */
    db++;
    *db = bm->bm_Width;
    
    GS_Reserve(gs, 3);
    
        /* Ensure we are synced up... */
    CLT_Sync(&gs->gs_ListPtr);
    
        /* Jump into the te list we've already calculated */
    CLT_JumpAbsolute(&gs->gs_ListPtr, ts->ts_TEList);
    /* Make sure we return again */
    ret = ts->ts_TERet;
    CLT_JumpAbsolute(&ret, gs->gs_ListPtr);
    ts->ts_NextClip = FirstNode(&ts->ts_ClipList);
    
    return(0);
}
