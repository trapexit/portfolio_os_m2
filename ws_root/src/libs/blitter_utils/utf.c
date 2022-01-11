/***************************************************************************
**
** @(#) utf.c 96/10/15 1.4
**
**  Code to parse a UTF file and convert it into a BlitObject
**
****************************************************************************/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <misc/iff.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/frame2d/loadtxtr.h>
#include <graphics/blitter.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define DPRT(x) /* printf x; */

void SetTextureAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value);
void SetDBlendAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value);

/* --------- IFF UTF File parsing code --------- */
/* The following code provides for parsing UTF files in terms of
 * FORM TXTR's (old, single texture-per-file format), LIST TXTR's
 * (new, one-or-more texture-per-file format), and embedded chunks
 * of either.
*
 */

#define ID_M2TA             MAKE_ID('M','2','T','A')
#define ID_M2DB             MAKE_ID('M','2','D','B')
#define ID_M2TD             MAKE_ID('M','2','T','D')
#define ID_M2TX             MAKE_ID('M','2','T','X')
#define ID_M2PI             MAKE_ID('M','2','P','I')
#define ID_M2CI             MAKE_ID('M','2','C','I')
#define ID_TXTR             MAKE_ID('T','X','T','R')

/* --------- Associated with the M2TX Chunk --------- */
typedef struct UTF_TextureHeader
{
    uint32                  Reserved1;
    uint32                  Flags;
    uint16                  MinXSize;
    uint16                  MinYSize;
    uint16                  TextureFormat;
    uint8                   NumLOD;
    uint8                   Reserved2;
} UTF_TextureHeader;

/* Flags */
#define TX_IsCompressed     0x01
#define TX_HasPIP           0x02
#define TX_HasColorConst    0x04

/* TextureFormat */
#define TX_IsLiteral FV_TXTEXPTYPE_ISLITERAL_MASK
#define TX_HasAlpha FV_TXTEXPTYPE_HASALPHA_MASK
#define TX_HasColor FV_TXTEXPTYPE_HASCOLOR_MASK
#define TX_HasSSB FV_TXTEXPTYPE_HASSSB_MASK
#define TX_CDepth FV_TXTEXPTYPE_CDEPTH_MASK
#define TX_ADepth FV_TXTEXPTYPE_ADEPTH_MASK
#define TX_ADepth_Shift FV_TXTEXPTYPE_ADEPTH_SHIFT

/* --------- Associated with the M2CI Chunk --------- */
typedef struct UTF_CompressionInfo
{
    uint16                  TextureFormat[2];
    uint32                  TxExpColorConst[4];
} UTF_CompressionInfo;

/* TextureFormat */
#define CI_IsTrans FV_TXTEXPTYPE_ISTRANS_MASK
#define CI_IsLiteral FV_TXTEXPTYPE_ISLITERAL_MASK
#define CI_HasAlpha FV_TXTEXPTYPE_HASALPHA_MASK
#define CI_HasSSB FV_TXTEXPTYPE_HASSSB_MASK
#define CI_HasColor FV_TXTEXPTYPE_HASCOLOR_MASK
#define CI_CDepth FV_TXTEXPTYPE_CDEPTH_MASK
#define CI_ADepth FV_TXTEXPTYPE_ADEPTH_MASK
#define CI_ADepth_Shift FV_TXTEXPTYPE_ADEPTH_SHIFT

/* --------- Associated with the M2TD Chunk --------- */
typedef struct UTF_TextureData
{
    uint16                  NumLOD;
    uint16                  Reserved;
    /* LODDataOff[] */
    /* Texel Data */
} UTF_TextureData;


/* --------- Associated with the M2TA Chunk --------- */
typedef struct UTF_TextureAttrData
{
    uint32                  Reserved;
    /* ... data ... */
} UTF_TextureAttrData;

/* --------- Associated with the M2DB Chunk --------- */
typedef struct UTF_DestBlendData
{
    uint32                  Reserved;
    /* ... data ... */
} UTF_DestBlendData;

/* --------- Associated with the M2PI Chunk --------- */
typedef struct UTF_PenIndexPalette
{
    uint32                  IndexOffset;
    uint32                  Reserved;
    /* PIP Data ... */
} UTF_PenIndexPalette;

/* --------- Possible Prop Chunks --------- */
const IFFTypeID Props[]=
{
    {ID_TXTR, ID_M2TX},
    {ID_TXTR, ID_M2TA},
    {ID_TXTR, ID_M2DB},
    {ID_TXTR, ID_M2PI},
    {ID_TXTR, ID_M2CI},
    {ID_TXTR, ID_M2TD},
    {0, 0}
};

/*
 * This function parses and attempts to load a texture from the
 * IFF stream provided.
 */
Err LoadTXTR (IFFParser *p, BlitObject **_bo)
{
    Err err;
    uint32 Depth;
        /* --------- The various snippets we might use --------*/
    BlitObject *bo;
    
        /* --------- Associated with the M2TX Chunk --------- */
    UTF_TextureHeader *TextureHeader =  NULL;

        /* --------- Associated with the M2PI Chunk --------- */
    UTF_PenIndexPalette *PIPData =  NULL;
    uint32 PIPLength = 0;

        /* --------- Associated with the M2CI Chunk --------- */
    UTF_CompressionInfo *CIData =  NULL;

        /* --------- Associated with the M2TD Chunk --------- */
    UTF_TextureData *TextureData =  NULL;

        /* --------- Associated with the M2TA Chunk --------- */
    UTF_TextureAttrData *TextureAttrData =  NULL;
    uint32 TALength = 0;

        /* --------- Associated with the M2DB Chunk --------- */
    UTF_DestBlendData *DestBlendData =  NULL;
    uint32 DBLength = 0;
    
    Depth = 1;

        /* The Blitter Folio will create all the snippets we need. The default format 
         * of the vertices snippet happens to be the format we want anyway.
         */
    err = Blt_CreateBlitObject(&bo, NULL);
    if (err < 0)
    {
        return(err);
    }
    
        /*
         * Load the indiviual chunks into memory, allocating space for
         * each chunk as we go.
         *
         */
    while (TRUE)
    {
            /* Scan for the next chunk */
        err = ParseIFF(p, IFF_PARSE_STEP);

            /* When we leave the chunk that we started in, quit */
        if (err == IFF_PARSE_EOC)
        {
            Depth--;
            err = 0;

            if (!Depth)
            {
                break;
            }
            else
            {
                continue;
            }
        }

            /* If we abruptly hit the end of file marker */
        if (err == IFF_PARSE_EOF)
        {
            err = 0;
            break;
        }

            /* If we encounter a subchunk */
        if (err == 0)
        {
            ContextNode *cn;

            Depth++;                            /* Keep track of how deep we're in */
            cn = GetCurrentContext (p);         /* What's the scoop on this chunk? */

                /* Verify valid chunks */
            switch (cn->cn_ID)
            {
                case ID_M2TX:
                case ID_M2PI:
                case ID_M2CI:
                case ID_M2TD:
                case ID_M2TA:
                case ID_M2DB:
                {
                    DPRT(("Have type %c%c%c%c\n",
                          ((cn->cn_ID & 0xff000000) >> 24), ((cn->cn_ID & 0xff0000) >> 16),
                          ((cn->cn_ID & 0xff00) >> 8), (cn->cn_ID & 0xff)));
                    break;
                }
                default:
                {
                    DPRT(("Unknown type %c%c%c%c\n",
                          ((cn->cn_ID & 0xff000000) >> 24), ((cn->cn_ID & 0xff0000) >> 16),
                          ((cn->cn_ID & 0xff00) >> 8), (cn->cn_ID & 0xff)));
                    err = BLITTER_ERR_UNKNOWNIFFFILE;
                    break;
                }
            }
        }
        if (err < 0)
        {
            break;
        }
    }

    if (err >= 0)
    {
            /* Success so far.  Now we ask the IFF Folio to provide
             * the chunks that we're interested in.
             */

        PropChunk  *pc;

        pc = FindPropChunk(p, ID_TXTR, ID_M2TX);
        if (pc)
        {
            TextureHeader = (UTF_TextureHeader *)pc->pc_Data;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2PI);
        if (pc)
        {
            PIPData = (UTF_PenIndexPalette *)pc->pc_Data;
            PIPLength = pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2CI);
        if (pc)
        {
            CIData = (UTF_CompressionInfo *)pc->pc_Data;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2TD);
        if (pc)
        {
            TextureData = (UTF_TextureData *) pc->pc_Data;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2TA);
        if (pc)
        {
            TextureAttrData = (UTF_TextureAttrData *)pc->pc_Data;
            TALength = pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2DB);
        if (pc)
        {
            DestBlendData = (UTF_DestBlendData *) pc->pc_Data;
            DBLength = pc->pc_DataSize;
        }
        
        if ((TextureHeader) && (TextureData))
        {
            uint32 bitsPerTexel;
            void *appropriateData;
            void *newData;
            uint32 newDataLength;
            uint32 xSize, ySize;

                /* --------- Process the Texture Header and the Texel Data --------- */

                /* Calculate the number of bits per texel, then use that to determine
                 * how large an area of memory the texel data whole occupies.
                 */
            if (TextureHeader->TextureFormat & TX_HasColor)
            {
                bitsPerTexel = (TextureHeader->TextureFormat & TX_CDepth);

                    /* If the data is literally RGB data, each pixel has three times
                     * as much color data associated with it -- Red, Green, and Blue
                     */
                if (TextureHeader->TextureFormat & TX_IsLiteral)
                bitsPerTexel *= 3;
            }
            else
            {
                bitsPerTexel = 0;
            }
            
                /* If Alpha Channel data exists ... */
            if (TextureHeader->TextureFormat & TX_HasAlpha)
            {
                bitsPerTexel += ((TextureHeader->TextureFormat & TX_ADepth) >> TX_ADepth_Shift);
            }
            
                /* If SSB information exists ... */
            if (TextureHeader->TextureFormat & TX_HasSSB)
            {
                bitsPerTexel += 1;
            }
            
                /* Skip past the LODDataPtr[] variable sized field in the texel data
                 * to find the beginning of the actual texel data
                 */
            appropriateData = (((uint8 *)(TextureData+1)) +
                               (sizeof(uint32)*TextureHeader->NumLOD));

                /* Adjust the X and Y sizes due to the LOD */
            xSize = (TextureHeader->MinXSize << (TextureHeader->NumLOD-1));
            ySize = (TextureHeader->MinYSize << (TextureHeader->NumLOD-1));

                /* Because Gfx "inherits" the memory passed through these calls,
                 * and later tries to free it by memtracked FreeMem(), I have
                 * to preallocate exactly the size they want.
                 */

            newDataLength = (xSize * ySize * bitsPerTexel);
                /* newDataLength is now the total number of bits used for this texture */
            
                /* Convert to bytes, rounding up as we go */
            newDataLength = ((newDataLength+7) >> 3);

            newData = AllocMem(newDataLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE);
            if (newData)
            {
                BltTxData *btdSnippet = bo->bo_txdata;
                    /* Copy our original data into the gfx copy */
                memcpy(newData, appropriateData, newDataLength);

                    /* Set up the fields in the CltTxData and CltTxLOD structures.
                     * Blt_CreateSnippet() already set
                     * btd_txData.texelData = &btd_txLOD.
                     */
                btdSnippet->btd_txData.flags = 0;
                btdSnippet->btd_txData.minX = xSize;
                btdSnippet->btd_txData.minY = ySize;
                btdSnippet->btd_txData.maxLOD = TextureHeader->NumLOD;
                btdSnippet->btd_txData.bitsPerPixel = bitsPerTexel;
                btdSnippet->btd_txData.expansionFormat = TextureHeader->TextureFormat;
                btdSnippet->btd_txData.dci = NULL;
                btdSnippet->btd_txData.texelData->texelDataSize = newDataLength;
                btdSnippet->btd_txData.texelData->texelData = newData;
            }
            else
            {
                err = BLITTER_ERR_NOMEM;
            }
            
                /* --------- Process the PIP Data --------- */
            if (PIPData && (err >= 0))
            {
                void *newData;
                uint32 newLength;
                PIPLoadSnippet *pipSnippet;
                
                pipSnippet = bo->bo_pip;
                newLength = (PIPLength - (2 * sizeof(uint32)));
                newData = AllocMem(newLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE);
                if (newData)
                {
                    memcpy(newData, (PIPData + 1), newLength);
                    
                        /* Set up the PIPLoadSnippet */
                    pipSnippet->ppl_cntl =
                        CLA_TXTPIPCNTL(RC_TXTPIPCNTL_PIPSSBSELECT_PIP,
                                       RC_TXTPIPCNTL_PIPALPHASELECT_PIP,
                                       RC_TXTPIPCNTL_PIPCOLORSELECT_PIP,
                                       PIPData->IndexOffset);
                    pipSnippet->ppl_constant[0] = 0;
                    pipSnippet->ppl_constant[1] = 0;
                    pipSnippet->ppl_base = TEPIPRAM;
                    pipSnippet->ppl_src = (uint32)newData;
                    pipSnippet->ppl_count = newLength;
                }
            }

                /* --------- Process the Texture Attributes Data --------- */
            if (TextureAttrData && (err >= 0))
            {
                uint32  numEntries;
                uint32 *attrs;
                BlitterSnippetHeader *snippets[2];
                
                attrs = (uint32 *)(TextureAttrData + 1);    /* Skip Reserved word */
                numEntries = ((TALength - sizeof(TextureAttrData)) / (2 * sizeof(uint32)));
                snippets[0] = &bo->bo_tbl->txb_header;
                snippets[1] = &bo->bo_pip->ppl_header;
                while (numEntries--)
                {
                    SetTextureAttr(snippets, attrs[0], attrs[1]);
                    attrs += 2;
                }
            }
                /* --------- Process the Destination Blend Data --------- */
            if (DestBlendData && (err >= 0))
            {
                uint32  numEntries;
                uint32 *attrs;
                BlitterSnippetHeader *snippets[1];

                attrs = (uint32 *)(DestBlendData + 1);    /* Skip Reserved word */
                numEntries = ((DBLength - sizeof(DestBlendData)) / (2 * sizeof(uint32)));
                snippets[0] = &bo->bo_dbl->dbl_header;
                while (numEntries--)
                {
                    SetDBlendAttr(snippets, attrs[0], attrs[1]);
                    attrs += 2;
                }
            }
                /* --------- Process the Compression Info Data --------- */
            if (CIData && (err >= 0))
            {
                CltTxDCI *newData;
                uint32 newLength;

                newLength = sizeof(CltTxDCI);
                newData = (CltTxDCI *)AllocMem(newLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE);
                if (newData)
                {
                    memcpy(newData, CIData, newLength);
                    bo->bo_txdata->btd_txData.dci = newData;
                }
                if (TextureHeader->Flags & TX_IsCompressed)
                {
                    bo->bo_txl->txl_cntl |= (1 << FV_TXTLDCNTL_COMPRESSED_SHIFT);
                }
            }
                /* --------- Create the verices to render this image --------*/
            if (err >= 0)
            {
                VerticesSnippet *vtxSnippet;

                vtxSnippet = bo->bo_vertices;
                    /* define the vertices (X, Y, W, U, V) */
                *BLITVERTEX_X(vtxSnippet, 0) = 0.0;
                *BLITVERTEX_Y(vtxSnippet, 0) = 0.0;
                *BLITVERTEX_W(vtxSnippet, 0) = W_2D;
                *BLITVERTEX_U(vtxSnippet, 0) = 0.0;
                *BLITVERTEX_V(vtxSnippet, 0) = 0.0;
                    
                *BLITVERTEX_X(vtxSnippet, 1) = xSize;
                *BLITVERTEX_Y(vtxSnippet, 1) = 0.0;
                *BLITVERTEX_W(vtxSnippet, 1) = W_2D;
                *BLITVERTEX_U(vtxSnippet, 1) = xSize;
                *BLITVERTEX_V(vtxSnippet, 1) = 0.0;
                    
                *BLITVERTEX_X(vtxSnippet, 2) = xSize;
                *BLITVERTEX_Y(vtxSnippet, 2) = ySize;
                *BLITVERTEX_W(vtxSnippet, 2) = W_2D;
                *BLITVERTEX_U(vtxSnippet, 2) = xSize;
                *BLITVERTEX_V(vtxSnippet, 2) = ySize;
                    
                *BLITVERTEX_X(vtxSnippet, 3) = 0.0;
                *BLITVERTEX_Y(vtxSnippet, 3) = ySize;
                *BLITVERTEX_W(vtxSnippet, 3) = W_2D;
                *BLITVERTEX_U(vtxSnippet, 3) = 0.0;
                *BLITVERTEX_V(vtxSnippet, 3) = ySize;

                err = Blt_SetVertices(vtxSnippet, &vtxSnippet->vtx_vertex[0]);
            }
                /* --------- Set up all the dimension registers -------- */
            if (err >= 0)
            {
                Blt_InitDimensions(bo, xSize, ySize, bitsPerTexel);
            }
        }
        else
        {
            err = BLITTER_ERR_FAULTYTEXTURE;
        }
    }
    
    if (err >= 0)
    {
        *_bo = bo;
    }
    
    return(err);
}


/*
 * Loads a BlitObject from a UTF file. See the autodocs for more details.
 */
Err Blt_LoadUTF(BlitObject **bo, const char *fname)
{
    Err err;

    err = OpenIFFFolio();
    if (err >= 0)
    {
        IFFParser *p;

        err = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, fname, TAG_END);
        if (err >= 0)
        {
                /* Scan for the first chunk */
            err = ParseIFF(p, IFF_PARSE_STEP);
            if (err >= 0)
            {
                ContextNode *cn;

                    /* Since we're expecting a particular file type,
                     * we'll be particularly authoritarian about not finding
                     * the exact ID and Type that we desire.
                     */

                cn = GetCurrentContext(p);
                if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR))
                {
                        /* Declare several chunks that might be present in a PROP chunk */
                    err = RegisterPropChunks(p, &Props[0]);
                    if (err >= 0)
                    {
                        err = LoadTXTR(p, bo);
                    }
                }
                else
                {
                    err = BLITTER_ERR_UNKNOWNIFFFILE;
                }
            }
            DeleteIFFParser(p);
        }
        CloseIFFFolio();
    }
    return(err);
}

/* Frees the resources allocated with Blt_LoadUTF(). See the autodocs for more details. */
Err Blt_FreeUTF(BlitObject *bo)
{
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    if (bo->bo_txdata)
    {
        FreeMem(bo->bo_txdata->btd_txData.texelData->texelData, TRACKED_SIZE);
        if (bo->bo_txdata->btd_txData.dci)
        {
            FreeMem(bo->bo_txdata->btd_txData.dci, TRACKED_SIZE);
        }
    }
    if (bo->bo_pip)
    {
        FreeMem((void *)bo->bo_pip->ppl_src, TRACKED_SIZE);
    }

    Blt_DeleteBlitObject(bo);
    return(0);
}

/* --------------------------------------------------*/
/* The following code is part of Blt_LoadTexture() */

/*
 * Skips over the chunk we're currently in, and exits
 * When we've hit this chunk's EOC.  [Skips over chunks
 * that're deeper than this level as well.]
 */
Err SkipOverThisLevel(IFFParser *i)
{
    uint32 Depth =  1;
    Err ret;

    while (TRUE)
    {
        ret = ParseIFF(i, IFF_PARSE_STEP);
        switch (ret)
        {
            case IFF_PARSE_EOF:
            {
                break;
            }
            case IFF_PARSE_EOC:
            {
                Depth--;
                ret = 0;
                break;
            }
            default:
            {
                Depth++;
                break;
            }
        }
        if (ret < 0)
        {
            break;
        }
        if (!Depth)
        {
            break;
        }
    }

    return(ret);
}

Err LoadMultipleTXTR(IFFParser *p, List *blitObjectList, LTCallBack cb, void *cbdata)
{
    Err ret;
    uint32 Depth;
    uint32 TextureNumber;

        /* --------- Preinitialize --------- */
    Depth = 1;                         /* How deep we are in the hierarchy */
    TextureNumber = 0;                         /* Count of TXTR from start of file */

    PrepList(blitObjectList);                           /* Create the empty list */
    
        /* Declare several chunks that might be present in a PROP chunk */
    ret = RegisterPropChunks(p, &Props[0]);
    if (ret >= 0)
    {
            /* As long as we don't step back in the hierarchy beyond our
             * entry level, keep scanning ...
             */
        while(Depth)
        {
                /* Scan for the next chunk */
            ret = ParseIFF(p, IFF_PARSE_STEP);

                /* When we leave the chunk that we started in, quit */
            if (ret == IFF_PARSE_EOC)
            {
                Depth--;
                ret = 0;

                if (!Depth)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

                /* If we abruptly hit the end of file marker */
            if (ret == IFF_PARSE_EOF)
            {
                ret = 0;
                break;
            }

                /* Have we stepped into a chunk? */
            if (ret == 0)
            {
                ContextNode *cn;

                    /* Keep track of the fact that we've stepped in a level */
                Depth++;
                cn = GetCurrentContext(p);

                    /* The only thing we wish to parse is FORM TXTRs */
                if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR))
                {
                    BlitObjectNode *bon;
                    bool accept=TRUE;

                    TextureNumber++;                /* Bump our counter */

                        /* If a callback exists, ask the caller if they
                         * want this texture or not ...
                         */
                    if (cb)
                    {
                        uint32 action;
                        action = (*cb)(TextureNumber, cbdata);
                        if (action == LOADTEXTURE_OK)
                        {
                            accept = TRUE;
                        }
                        if (action == LOADTEXTURE_STOP)
                        {
                            break;
                        }
                    }

                    if (accept)
                    {
                            /* Allocate a BlitObjectNode, and load the chunk into that */
                        bon = (BlitObjectNode *)AllocMem(sizeof(BlitObjectNode), MEMTYPE_NORMAL);
                        if (bon)
                        {
                                /* Save our new BlitObject */
                            AddTail(blitObjectList, (Node *)&bon->bon_Node);

                                /* Try to load the bitmap into it */
                            ret = LoadTXTR(p, &bon->bon_BlitObject);
                        }
                        else
                        {
                            ret = BLITTER_ERR_NOMEM;
                        }
                    }
                    else
                    {
                        SkipOverThisLevel(p);
                    }
                    
                    Depth--;
                } /* if ... FORM TXTR */
                else
                {
                    SkipOverThisLevel(p);
                    Depth--;
                }

            } /* ret==0 from ParseIFF */
            if (ret < 0)
            {
                break;
            }
        } /* while TRUE */
    }

    return(ret);
}

Err Blt_LoadTexture(List *blitObjectList, const TagArg *tags)
{
    IFFParser *p;
    char *filename;
    uint8 ScanMode;
    TagArg *LocatedTag;
    Err ret;
    LTCallBack cb;
    void *cbdata;

        /* --------- Preinitialize --------- */
    p = NULL;
    filename = NULL;
    ScanMode = 0;
    cb = NULL;
    cbdata = NULL;

    PrepList(blitObjectList);

    ret = OpenIFFFolio();
    if (ret >= 0)
    {
            /* --------- Check the taglist for arguments --------- */
        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_FILENAME);
        if (LocatedTag)
        {
            filename = (char *)LocatedTag->ta_Arg;
            ScanMode = LOADTEXTURE_TYPE_AUTOPARSE;
        }

        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_IFFPARSER);
        if (LocatedTag)
        {
                /* IFFPARSER & FILENAME are mutually exclusive ... */
            if (!filename)
            {
                p  = (IFFParser *)LocatedTag->ta_Arg;
                    /* For IFFPARSER, IFFPARSETYPE must also be present */
                LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_IFFPARSETYPE);
                if (LocatedTag)
                {
                    ScanMode = (uint8)LocatedTag->ta_Arg;

                        /* Make sure ScanMode is legal */
                    if ((ScanMode != LOADTEXTURE_TYPE_AUTOPARSE) &&
                        (ScanMode != LOADTEXTURE_TYPE_SINGLE) &&
                        (ScanMode != LOADTEXTURE_TYPE_MULTIPLE))
                    {
                        ret = BLITTER_ERR_BADTAGARG;
                    }
                        /* else ... continue processing */
                }
                else
                {
                    ret = BLITTER_ERR_BADTAGARG; /* Incomplete parameters */;
                }
            }
            else
            {
                ret = BLITTER_ERR_BADTAGARG; /* Mutually exclusive */
            }
        }

        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_CALLBACK);
        if (LocatedTag)
        {
            cb = (LTCallBack)LocatedTag->ta_Arg;

            LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_CALLBACKDATA);
            if (LocatedTag)
            {
                cbdata = (void *)LocatedTag->ta_Arg;
            }
            else
            {
                ret = BLITTER_ERR_BADTAGARG; /* Incomplete parameters */
            }
        }
        
            /* --------- Start actually doing work --------- */

            /* If they gave us a filename, get an IFFParse from the
             * IFF folio for it.
             */
        if ((filename) && (ret >= 0))
        {
            ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, filename, TAG_END);
            if (ret < 0)
            {
                p = (IFFParser *) NULL;
            }
        }

            /* If all has gone well so far ... */
        if (ret >= 0)
        {
                /* Should we be parsing for the header? */
            if (ScanMode == LOADTEXTURE_TYPE_AUTOPARSE)
            {
                    /* Scan for the first chunk */
                ret = ParseIFF(p, IFF_PARSE_STEP);
                if (ret >= 0)
                {
                    ContextNode *cn;

                        /* We're expecting one of two Type/ID pairs:
                         * FORM TXTR implies a single-texture UTF file.
                         * LIST TXTR implies one or more textures in a file.
                         */
                    cn = GetCurrentContext(p);

                    if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR))
                    {
                        ScanMode = LOADTEXTURE_TYPE_SINGLE;
                    }
                    else if ((cn->cn_ID == ID_LIST) || (cn->cn_ID == ID_CAT))
                    {
                        ScanMode = LOADTEXTURE_TYPE_MULTIPLE;
                    }
                    else
                    {
                        ret = BLITTER_ERR_UNKNOWNIFFFILE;
                    }
                }
            }
            if (ScanMode == LOADTEXTURE_TYPE_SINGLE)
            {
                BlitObjectNode *bon;
                bool skip = FALSE;

                bon = (BlitObjectNode *)AllocMem(sizeof(BlitObjectNode), MEMTYPE_NORMAL);
                if (bon)
                {
                    bon->bon_BlitObject = NULL;
                    
                        /* Ask the IFF folio to collect any of our chunks */
                    ret = RegisterPropChunks(p, &Props[0]);
                    if (ret >= 0)
                    {
                            /* As weird as it sounds, to be consistent, if you
                             * defined a callback -- but are trying to load a
                             * single-texture file, still honor the callback
                             * and allow you to say 'yes' or 'no' to loading
                             * the texture.
                             */
                        if (cb)
                        {
                            if ((*cb)(0, cbdata) == LOADTEXTURE_OK)
                            {
                                ret = LoadTXTR(p, &bon->bon_BlitObject);
                            }
                            else
                            {
                                skip = TRUE;
                            }
                        }
                        else    /* Otherwise, just load it */
                        {
                            ret = LoadTXTR(p, &bon->bon_BlitObject);
                        }
                    }
                    if ((ret >= 0) && (!skip))
                    {
                        AddTail(blitObjectList, (Node *)&bon->bon_Node);
                    }
                    else
                    {
                        Blt_FreeUTF(bon->bon_BlitObject);
                        FreeMem(bon, sizeof(BlitObjectNode));
                    }
                }
                else
                {
                    ret = BLITTER_ERR_NOMEM;
                }
            }
            if (ScanMode == LOADTEXTURE_TYPE_MULTIPLE)
            {
                ret = LoadMultipleTXTR(p, blitObjectList, cb, cbdata);
            }
        }

            /* --------- Allocation Cleanup --------- */
            /* In the event we created an IFFParser for their filename, */
            /* Delete it as cleanup */
        if ((filename) && (p))
        {
            DeleteIFFParser(p);
        }
        
            /* If an error, and we had loaded some textures, free them */
        if (ret < 0)
        {
            Blt_FreeTexture(blitObjectList);
        }
        
        CloseIFFFolio();
    }

    return(ret);
}

void Blt_FreeTexture(List *blitObjectList)
{
    BlitObjectNode *bon;

    /* Delete each BlitObject on the list */
    while ((bon = (BlitObjectNode *)RemTail(blitObjectList)))
    {
        Blt_FreeUTF(bon->bon_BlitObject);
        FreeMem(bon, sizeof(BlitObjectNode));
    }
}

