/* @(#) loadtexture.c 96/07/09 1.7 */


#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/loadtxtr.h>
#include <misc/iff.h>
#include <string.h>

/* --------- IFF UTF File parsing code --------- */
/* The following code provides for parsing UTF files in terms of
 * FORM TXTR's (old, single texture-per-file format), LIST TXTR's
 * (new, one-or-more texture-per-file format), and embedded chunks
 * of either.
 *
 * The lowest level code lies at the beginning of this file, the
 * highest level code at the end.
 *
 */

#define ID_M2TA             MAKE_ID('M','2','T','A')
#define ID_M2DB             MAKE_ID('M','2','D','B')
#define ID_M2TD             MAKE_ID('M','2','T','D')
#define ID_M2TX             MAKE_ID('M','2','T','X')
#define ID_M2PI             MAKE_ID('M','2','P','I')
#define ID_M2CI             MAKE_ID('M','2','C','I')
#define ID_M2LR             MAKE_ID('M','2','L','R')
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
#define TX_IsLiteral        0x1000
#define TX_HasAlpha         0x0800
#define TX_HasColor         0x0400
#define TX_HasSSB           0x0200
#define TX_CDepth           0x000F
#define TX_ADepth           0x00F0
#define TX_ADepth_Shift     4

/* --------- Associated with the M2CI Chunk --------- */
typedef struct UTF_CompressionInfo
{
    uint16                  TextureFormat[2];
    uint32                  TxExpColorConst[4];
} UTF_CompressionInfo;

/* TextureFormat */
#define CI_IsTrans          0x0100
#define CI_IsLiteral        0x1000
#define CI_HasAlpha         0x0800
#define CI_HasSSB           0x0200
#define CI_HasColor         0x0400
#define CI_CDepth           0x000F
#define CI_ADepth           0x00F0
#define CI_ADepth_Shift     4

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

/* --------- Associated with the M2LR Chunk --------- */
typedef struct UTF_LoadRects
{
    uint32                  NumLoadRects;
    uint32                  Reserved;
    /* ... data ... */
} UTF_LoadRects;

typedef struct UTF_LoadRectData
{
    int16                   xWrapMode;
    int16                   yWrapMode;
    int16                   firstLOD;
    int16                   nLOD;
    int16                   xOff;
    int16                   yOff;
    int16                   xSize;
    int16                   ySize;
} UTF_LoadRectData;

/* --------- Possible Prop Chunks --------- */
static const IFFTypeID Props[]=
{
    {ID_TXTR, ID_M2TX},
    {ID_TXTR, ID_M2TA},
    {ID_TXTR, ID_M2DB},
    {ID_TXTR, ID_M2PI},
    {ID_TXTR, ID_M2CI},
    {ID_TXTR, ID_M2LR},
    {ID_TXTR, ID_M2TD},
    {0,       0}
};


/**
|||	AUTODOC -internal -class Application -group Texture  -name LoadTXTR
|||	Loads a texture from an IFF file
|||
|||	  Synopsis
|||
|||	    Err LoadTXTR(IFFParser *iff, SpriteObj *sp);
|||
|||	  Description
|||
|||	    This function parses and attempts to load a texture from the
|||	    IFF stream provided.
|||
|||	  Arguments
|||
|||	    iff
|||	        An active IFFParser *, from the IFF Folio.
|||
|||	    sp
|||	        A SpriteObj *, from Spr_Create(), to be associated with this
|||	        texture.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative number for an error code.
|||
|||	  Associated Files
|||
|||	    multitexture.h
|||
|||	  Notes
|||
|||	    To provide for the ability to imbed texture chunks in IFF files, this
|||	    function requires that the IFF parser stream has already been parsed
|||	    up to the FORM TXTR header (identifying that chunk as a texture
|||	    chunk).  The function will parse only the levels of the IFF file that
|||	    it was called at, or deeper.  It will not parse higher levels.
|||
|||	    All resources associated with loading the texture can be freed via
|||	    the Spr_Delete() function.
|||
|||	  See Also
|||
|||	    Spr_Delete()
|||
**/
Err LoadTXTR (IFFParser *p, SpriteObj *sp)
{
    Err                  ret;

    uint32               Depth;

    /* --------- Associated with the M2TX Chunk --------- */
    UTF_TextureHeader   *TextureHeader;
    uint32               THLength;

    /* --------- Associated with the M2PI Chunk --------- */
    UTF_PenIndexPalette *PIPData;
    uint32               PIPLength;

    /* --------- Associated with the M2CI Chunk --------- */
    UTF_CompressionInfo *CIData;
    uint32               CILength;

    /* --------- Associated with the M2TD Chunk --------- */
    UTF_TextureData     *TextureData;
    uint32               TDLength;

    /* --------- Associated with the M2TA Chunk --------- */
    UTF_TextureAttrData *TextureAttrData;
    uint32               TALength;

    /* --------- Associated with the M2DB Chunk --------- */
    UTF_DestBlendData 	*DestBlendData;
    uint32               DBLength;

    /* --------- Associated with the M2LR Chunk --------- */
    UTF_LoadRectData    *LoadRectData;
    uint32               LRLength;

    /* Preinitialize */
    TextureHeader =     (UTF_TextureHeader *)       NULL;
    PIPData =           (UTF_PenIndexPalette *)     NULL;
    CIData =            (UTF_CompressionInfo *)     NULL;
    TextureData =       (UTF_TextureData *)         NULL;
    TextureAttrData =   (UTF_TextureAttrData *)     NULL;
	DestBlendData = 	(UTF_DestBlendData *) 		NULL;
    LoadRectData =      (UTF_LoadRectData *)        NULL;

    THLength    =   0;
    PIPLength   =   0;
    CILength    =   0;
    TDLength    =   0;
    TALength    =   0;
	DBLength 	=	0;
    LRLength    =   0;

    Depth       =   1;

    /*
     * Load the indiviual chunks into memory, allocating space for
     * each chunk as we go.
     *
     */
    while (TRUE)
    {
        /* Scan for the next chunk */
        ret = ParseIFF (p, IFF_PARSE_STEP);

        /* When we leave the chunk that we started in, quit */
        if (ret == IFF_PARSE_EOC)
        {
            Depth--;
            ret = 0;

            if (!Depth)
                break;
            else
                continue;
        }

        /* If we abruptly hit the end of file marker */
        if (ret == IFF_PARSE_EOF)
        {
            ret = 0;
            break;
        }

        /* If we encounter a subchunk */
        if (ret == 0)
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
                case ID_M2LR:
				case ID_M2DB:
                    break;
                default:
                    ret = LT_ERR_UNKNOWNIFFFILE;
                    break;
            } /* switch */
        } /* ret==0 from ParseIFF */
        if (ret < 0)
            break;
    } /* while TRUE */

    if (ret >= 0)
    {
        /* Success so far.  Now we ask the IFF Folio to provide
         * the chunks that we're interested in.
         */

        PropChunk  *pc;

        pc = FindPropChunk(p, ID_TXTR, ID_M2TX);
        if (pc)
        {
            TextureHeader   =   (UTF_TextureHeader *)   pc->pc_Data;
            THLength        =                           pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2PI);
        if (pc)
        {
            PIPData         =   (UTF_PenIndexPalette *) pc->pc_Data;
            PIPLength       =                           pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2CI);
        if (pc)
        {
            CIData          =   (UTF_CompressionInfo *) pc->pc_Data;
            CILength        =                           pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2TD);
        if (pc)
        {
            TextureData     =   (UTF_TextureData *)     pc->pc_Data;
            TDLength        =                           pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2TA);
        if (pc)
        {
            TextureAttrData =   (UTF_TextureAttrData *) pc->pc_Data;
            TALength        =                           pc->pc_DataSize;
        }

        pc = FindPropChunk(p, ID_TXTR, ID_M2DB);
        if (pc)
        {
            DestBlendData   =   (UTF_DestBlendData *)   pc->pc_Data;
            DBLength        =                           pc->pc_DataSize;
        }


        pc = FindPropChunk(p, ID_TXTR, ID_M2LR);
        if (pc)
        {
            LoadRectData    =   (UTF_LoadRectData *)    pc->pc_Data;
            LRLength        =                           pc->pc_DataSize;
        }

        if ( (TextureHeader) && (TextureData) )
        {
            uint32       BitsPerTexel;
            void        *AppropriateData;
            void        *NewData;
            uint32       NewDataLength;
            uint32       XSize, YSize;

            /* --------- Process the Texture Header and the Texel Data --------- */

            /* Calculate the number of bits per texel, then use that to determine
             * how large an area of memory the texel data whole occupies.
             */
            if (TextureHeader->TextureFormat & TX_HasColor)
            {
                BitsPerTexel = (TextureHeader->TextureFormat & TX_CDepth);

                /* If the data is literally RGB data, each pixel has three times
                 * as much color data associated with it -- Red, Green, and Blue
                 */
                if (TextureHeader->TextureFormat & TX_IsLiteral)
                    BitsPerTexel *= 3;
            }
            else
                BitsPerTexel = 0;

            /* If Alpha Channel data exists ... */
            if (TextureHeader->TextureFormat & TX_HasAlpha)
                BitsPerTexel += ((TextureHeader->TextureFormat & TX_ADepth) >> TX_ADepth_Shift);

            /* If SSB information exists ... */
            if (TextureHeader->TextureFormat & TX_HasSSB)
                BitsPerTexel += 1;

            /* Skip past the LODDataPtr[] variable sized field in the texel data
             * to find the beginning of the actual texel data
             */
            AppropriateData =   ((uint8 *)(TextureData+1)) +
                                (sizeof(uint32)*TextureHeader->NumLOD);

            /* Adjust the X and Y sizes due to the LOD */
            XSize = (TextureHeader->MinXSize << (TextureHeader->NumLOD-1));
            YSize = (TextureHeader->MinYSize << (TextureHeader->NumLOD-1));

            /* Because Gfx "inherits" the memory passed through these calls,
             * and later tries to free it by memtracked FreeMem(), I have
             * to preallocate exactly the size they want.
             */

            NewDataLength = XSize * YSize * BitsPerTexel;
            /* NewDataLength is now the total number of bits used for this texture */

            /* Convert to bytes, rounding up as we go */
            NewDataLength = (NewDataLength+7) >> 3;

            NewData = AllocMem( NewDataLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE );
            if (NewData)
            {
                /* Copy our original data into the gfx copy */
                memcpy(NewData, AppropriateData, NewDataLength);

                ret = Spr_CreateTxData( sp, NewData, XSize, YSize,
                                        BitsPerTexel, TextureHeader->TextureFormat);
                if (ret < 0)
                    FreeMem(NewData, TRACKED_SIZE);
                else
                    sp->spr_Flags |= SPR_SYSTEMTEXTUREDATA;
            }
            else
                ret = NOMEM;

            /* --------- Process the PIP Data --------- */
            if (PIPData)
            {
                uint32   NumEntries;
                void    *NewData;
                uint32   NewLength;

                NewLength = PIPLength-(2*sizeof(uint32));
                NewData = AllocMem( NewLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE );
                if (NewData)
                {
                    memcpy(NewData, PIPData+1, NewLength);
                    NumEntries = NewLength / sizeof(uint32);
                    ret = Spr_CreatePipControlBlock(sp, NewData, PIPData->IndexOffset,
                                                    NumEntries);
                    if (ret < 0)
                        FreeMem(NewData, TRACKED_SIZE);
                    else
                        sp->spr_Flags |= SPR_SYSTEMPIPDATA;

                }
            }

            /* --------- Process the Texture Attributes Data --------- */
            if (TextureAttrData)
            {
                uint32  NumEntries;
                uint32 *Attrs;

                Attrs =     (uint32 *)  (TextureAttrData+1);    /* Skip Reserved word */
                NumEntries = ( (TALength-sizeof(TextureAttrData)) / (2*sizeof(uint32)) );
                while (NumEntries--)
                {
                    ret = Spr_SetTextureAttr(sp, Attrs[0], Attrs[1]);
                    if (ret < 0)
                        break;
                    Attrs += 2;
                }
            }

            /* --------- Process the Destination Blend Data --------- */
            if (DestBlendData)
            {
                uint32  NumEntries;
                uint32 *Attrs;

                Attrs =     (uint32 *)  (DestBlendData+1);    /* Skip Reserved word */
                NumEntries = ( (DBLength-sizeof(DestBlendData)) / (2*sizeof(uint32)) );
                while (NumEntries--)
                {
                    ret = Spr_SetDBlendAttr(sp, Attrs[0], Attrs[1]);
                    if (ret < 0)
                        break;
                    Attrs += 2;
                }
            }
            /* --------- Process the Compression Info Data --------- */
            if (CIData)
            {
                void        *NewData;
                uint32       NewLength;

                NewLength = sizeof(CltTxDCI);
                NewData = AllocMem( NewLength, MEMTYPE_ANY | MEMTYPE_TRACKSIZE );
                if (NewData)
                {
                    CltTxData   *t;

                    memcpy( NewData, CIData, NewLength );

                    t = Spr_GetTextureData(sp);
                    if (t)
                    {
                        t->dci = (CltTxDCI *) NewData;
                    }
                    else
                        FreeMem(NewData, TRACKED_SIZE);
                }
            }

        }
        else
        {
            ret = LT_ERR_FAULTYTEXTURE;
        }
    }

    TOUCH(LoadRectData);
    TOUCH(LRLength);
    TOUCH(THLength);
    TOUCH(CILength);
    TOUCH(TDLength);


    return(ret);
}

/* Allocate a normal, short, or extended SpriteObj */
SpriteObj *CreateSprite(uint32 spritetype)
{
    switch (spritetype)
    {
        case LOADTEXTURE_SPRITETYPE_NORMAL:
            return(Spr_Create(0));
        case LOADTEXTURE_SPRITETYPE_SHORT:
            return(Spr_CreateShort(0));
        case LOADTEXTURE_SPRITETYPE_EXTENDED:
            return(Spr_CreateExtended(0));
    }
    return( (SpriteObj *) NULL );
}


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
                break;
            case IFF_PARSE_EOC:
                Depth--;
                ret = 0;
                break;
            default:
                Depth++;
                break;
        }
        if (ret < 0)
            break;
        if (!Depth)
            break;
    }

    return(ret);
}

/**
|||	AUTODOC -internal -class Application -group Texture  -name LoadMultipleTXTR
|||	Loads multiple textures from an IFF file
|||
|||	  Synopsis
|||
|||	    Err LoadMultipleTXTR(IFFParser *iff, List *SpriteList, LTCallBack cb,
|||	                         void *cbdata);
|||
|||	  Description
|||
|||	    This function parses and attempts to load all of the textures
|||	    contained in the given IFF file.  As each texture is encountered,
|||	    a SpriteObj is allocated and associated with the texture.
|||
|||	  Arguments
|||
|||	    iff
|||	        An active IFFParser *, from the IFF Folio.
|||
|||	    SpriteList
|||	        A pointer to a List structure, to be initialized and used by
|||	        LoadMultipleTXTR() to return all of the SpriteObj's loaded.
|||
|||	    cb
|||	        An pointer to a callback routine that is intended to accept
|||	        or deny any particular texture from being loaded.  The callback
|||	        routine is passed the texture number (0 ... n, counted from the
|||	        beginning of the file), and a user-private data pointer.  The
|||	        callback must return LOADTEXTURE_OK if it wishes to accept the
|||	        given texture, LOADTEXTURE_SKIP if it wishes the folio to skip
|||	        loading that texture, or LOADTEXTURE_STOP if it wishes to cease
|||	        parsing the file any further.  If this parameter is NULL, it
|||	        IS Ignored.
|||
|||	    cbdata
|||	        The user supplied data for the callback routine, above.  If
|||	        the callback isn't provided (ie, cb == NULL), cbdata is
|||	        ignored.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative number for an error code.
|||
|||	  Associated Files
|||
|||	    multitexture.h
|||
|||	  Notes
|||
|||	    To provide for embedding hierarchies of chunks in IFF files, this
|||	    function requires that the IFF parser stream has already been parsed
|||	    up to the LIST TXTR header (identifying the contexts as multiple
|||	    TXTR files).  The function will parse only the levels of the IFF
|||	    that it was called at or deeper.  It will not attempt to parse
|||	    higher levels.
|||
|||	  See Also
|||
|||	    UnloadMultipleTXTR(), LoadTXTR()
|||
**/
Err LoadMultipleTXTR(IFFParser *p, List *SpriteList, LTCallBack cb, void *cbdata,
                     uint32 spritetype)
{
    Err                  ret;
    uint32               Depth;
    uint32               TextureNumber;

    /* --------- Preinitialize --------- */
    Depth =              1;                         /* How deep we are in the hierarchy */
    ret   =              0;                         /* Error codes */
    TextureNumber   =    0;                         /* Count of TXTR from start of file */

    PrepList(SpriteList);                           /* Create the empty list */

    TOUCH(ret);                                     /* Keep compiler from complaining */

    /* Declare several chunks that might be present in a PROP chunk */
    ret = RegisterPropChunks(p, &Props[0]);
    if (ret >= 0)
    {
        /* As long as we don't step back in the hierarchy beyond our
         * entry level, keep scanning ...
         */
        while (Depth)
        {
            /* Scan for the next chunk */
            ret = ParseIFF (p, IFF_PARSE_STEP);

            /* When we leave the chunk that we started in, quit */
            if (ret == IFF_PARSE_EOC)
            {
                Depth--;
                ret = 0;

                if (!Depth)
                    break;
                else
                    continue;
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
                ContextNode            *cn;

                /* Keep track of the fact that we've stepped in a level */
                Depth++;
                cn = GetCurrentContext (p);

                /* The only thing we wish to parse is FORM TXTRs */
                if ( (cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR) )
                {
                    SpriteObj          *sp;
                    bool                accept=TRUE;

                    TextureNumber++;                /* Bump our counter */

                    /* If a callback exists, ask the caller if they
                     * want this texture or not ...
                     */
                    if (cb)
                    {
                        uint32 action;
                        action = (*cb)(TextureNumber, cbdata);
                        if (action == LOADTEXTURE_OK)
                            accept = TRUE;
                        if (action == LOADTEXTURE_STOP)
                            break;
                    }

                    if (accept)
                    {
                        /* Allocate a SpriteObj, and load the chunk into that */
                        sp = CreateSprite(spritetype);
                        if (sp)
                        {
                            /* Save our new sprite */
                            AddTail(SpriteList, &sp->spr_Node);

                            /* Try to load the bitmap into it */
                            ret = LoadTXTR(p, sp);
                        }
                        else
                            ret = NOMEM;
                    }
                    else
                        SkipOverThisLevel(p);

                    Depth--;
                } /* if ... FORM TXTR */
                else
                {
                    SkipOverThisLevel(p);
                    Depth--;
                }

            } /* ret==0 from ParseIFF */
            if (ret < 0)
                break;
        } /* while TRUE */
    }

    /* --------- Cleanup in the event of a problem --------- */
    if (ret < 0)
    {
        SpriteObj   *sp;

        /* Delete each Sprite on the list */
        while ( (sp = (SpriteObj *) RemTail(SpriteList)) )
            Spr_Delete(sp);
    }

    return(ret);
}

/**
|||	AUTODOC -public -class frame2d -name Spr_LoadUTF
|||	Load a sprite from a UTF file.
|||
|||	  Synopsis
|||
|||	    Err Spr_LoadUTF (SpriteObj *sp, const char *fname);
|||
|||	  Description
|||
|||	    Load a sprite object from a UTF file.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to load the file into
|||
|||	    fname
|||	        Name of the file to load
|||
|||	  Return Value
|||
|||	    >= 0 on success or a negative error code for failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/loadtxtr.h>
|||
|||	  See Also
|||
|||	    Spr_Create()
|||
|||	  Notes
|||
|||	    This function is retained for compatibility.  Spr_LoadTexture()
|||	    acts as a superset, permitting loading of multiple textures per
|||	    UTF file.
**/
Err Spr_LoadUTF(SpriteObj *sp, const char *fname)
{
    Err          ret;

    ret = OpenIFFFolio();
    if (ret >= 0)
    {
        IFFParser   *p;

        ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, fname, TAG_END);
        if (ret >= 0)
        {
            /* Scan for the first chunk */
            ret = ParseIFF (p, IFF_PARSE_STEP);
            if (ret >= 0)
            {
                ContextNode     *cn;

                /* Since we're expecting a particular file type,
                 * we'll be particularly authoritarian about not finding
                 * the exact ID and Type that we desire.
                 */

                cn = GetCurrentContext(p);
                if ( (cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR) )
                {
                    /* Declare several chunks that might be present in a PROP chunk */
                    ret = RegisterPropChunks(p, &Props[0]);
                    if (ret >= 0)
                    {
                        ret = LoadTXTR(p, sp);
                    }

                }
                else
                    ret = LT_ERR_UNKNOWNIFFFILE;
            }
            DeleteIFFParser(p);
        }
        CloseIFFFolio();
    }
    return(ret);
}


/**
|||	AUTODOC -public -class frame2d -name Spr_UnloadTexture
|||	Frees resources associated with previously loaded texture files
|||
|||	  Synopsis
|||
|||	    void Spr_UnloadTexture(List *SpriteList);
|||
|||	  Description
|||
|||	    This function attempts to free the resources associated with
|||	    a previous successful use of Spr_LoadTexture().
|||
|||	  Arguments
|||
|||	    SpriteList
|||	        A pointer to a List structure previously passed to the
|||	        Spr_LoadTexture() function, containing any number of SpriteObj
|||	        structures.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/loadtxtr.h>
|||
|||	  See Also
|||
|||	    Spr_LoadTexture()
|||
**/

void Spr_UnloadTexture(List *SpriteList)
{
    /* --------- Free the sprites --------- */
    SpriteObj   *sp;

    /* Delete each Sprite on the list */
    while ( (sp = (SpriteObj *) RemTail(SpriteList)) )
        Spr_Delete(sp);
}

/**
|||	AUTODOC -public -class frame2d -name Spr_LoadTexture
|||	Loads one or more textures from a UTF file
|||
|||	  Synopsis
|||
|||	    Err Spr_LoadTexture(List *SpriteList, const TagArg *tags);
|||	    Err Spr_LoadTextureVA(List *SpriteList, uint32 tag, ... );
|||
|||	  Description
|||
|||	    This function parses and attempts to load all of the textures
|||	    contained in the given UTF file.  As each texture is encountered,
|||	    a SpriteObj is allocated, associated with the texture, and added
|||	    to the list SpriteList.
|||
|||	  Arguments
|||
|||	    SpriteList
|||	        A pointer to a List structure, to be initialized and used by
|||	        Spr_LoadTexture() to return all of the SpriteObj's loaded.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function.  See below for a description of the tags
|||	        supported.
|||
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function.  The array must be terminated with TAG_END.
|||
|||	    LOADTEXTURE_TAG_FILENAME (const char *)
|||	        This tag defines the name of the UTF file to load textures
|||	        from.  This tag is mutually exclusive with
|||	        LOADTEXTURE_TAG_IFFPARSER.
|||
|||	    LOADTEXTURE_TAG_IFFPARSER (IFFParser *)
|||	        This tag permits the caller to utilize an already existing
|||	        IFFParser structure from the IFF folio.  This is useful for
|||	        embedding texture data in other IFF files.  This tag is
|||	        mutually exclusive with LOADTEXTURE_TAG_FILENAME, and requires
|||	        the the presence of LOADTEXTURE_TAG_IFFPARSETYPE to be functional.
|||
|||	    LOADTEXTURE_TAG_IFFPARSETYPE (uint32)
|||	        This tag defines options for IFF parsing, and is only valid
|||	        when present with the LOADTEXTURE_TAG_IFFPARSER tag.  The
|||	        following options are available, and can be OR'd together:
|||
|||	        LOADTEXTURE_TYPE_AUTOPARSE
|||	        Requests that Spr_LoadTexture() parse for either a
|||	        FORM TXTR or LIST TXTR header.
|||
|||	        LOADTEXTURE_TYPE_SINGLE
|||	        Indicates that the caller has already parsed up to a
|||	        FORM TXTR, denoting this stream as a single-texture UTF file.
|||
|||	        LOADTEXTURE_TYPE_MULTIPLE
|||	        Indicates that the caller has already parsed up to a
|||	        LIST TXTR, denoting this stream as a multiple-texture UTF file.
|||
|||	    LOADTEXTURE_TAG_CALLBACK (LTCallBack)
|||	        This tag provides the caller with the ability to selectively
|||	        accept or deny loading of textures based on their order in
|||	        the source data file.  Providing the address of the callback
|||	        routine in ta_Arg, and also providing the tag
|||	        LOADTEXTURE_TAG_CALLBACKDATA, whenever a new texture is
|||	        encountered, a callback will be made with the number of the
|||	        encountered texture (0 ... n) and the user data provided by
|||	        LOADTEXTURE_TAG_CALLBACKDATA.  The callback routine itself
|||	        must return LOADTEXTURE_OK if it wishes that texture to be
|||	        loaded, LOADTEXTURE_SKIP if it would like to skip loading
|||	        the current texture, or LOADTEXTURE_STOP if it would like to
|||	        cease parsing the IFF file at the current point.
|||
|||	    LOADTEXTURE_TAG_CALLBACKDATA
|||	        This tag, valid only when presented with the tag
|||	        LOADTEXTURE_TAG_CALLBACK, defines user private callback data
|||	        to be provided to a callback routine when new textures are
|||	        about to be loaded.
|||
|||	    LOADTEXTURE_TAG_SPRITETYPE (uint32)
|||	        When Spr_LoadTexture() creates SprObjects for each texture it
|||	        encounters, it defaults to creating normal sprites.  This tag
|||	        provides a way to have the function create either short or
|||	        extended sprites instead.  LOADTEXTURE_SPRITETYPE_NORMAL
|||	        is the default, while LOADTEXTURE_SPRITETYPE_SHORT, and
|||	        LOADTEXTURE_SPRITETYPE_EXTENDED request short and extended
|||	        sprites respectively.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative number for an error code.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/loadtxtr.h>
|||
|||	  See Also
|||
|||	    Spr_UnloadTexture()
|||
**/
Err Spr_LoadTexture(List *SpriteList, const TagArg *tags)
{
    IFFParser   *p;
    char        *filename;
    uint8        ScanMode;
    TagArg      *LocatedTag;
    Err          ret;
    LTCallBack   cb;
    void        *cbdata;
    uint32       spritetype;

    /* --------- Preinitialize --------- */
    p           =   (IFFParser *)   NULL;
    filename    =                   NULL;
    ScanMode    =                   0;
    cb          =   (LTCallBack)    NULL;
    cbdata      =   (void *)        NULL;
    spritetype  =                   LOADTEXTURE_SPRITETYPE_NORMAL;

    PrepList(SpriteList);

    ret = OpenIFFFolio();
    if (ret >= 0)
    {
        /* --------- Check the taglist for arguments --------- */
        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_FILENAME);
        if (LocatedTag)
        {
            filename = (char *)         LocatedTag->ta_Arg;
            ScanMode =                  LOADTEXTURE_TYPE_AUTOPARSE;
        }

        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_IFFPARSER);
        if (LocatedTag)
        {
            /* IFFPARSER & FILENAME are mutually exclusive ... */
            if (!filename)
            {
                p  = (IFFParser *)      LocatedTag->ta_Arg;
                /* For IFFPARSER, IFFPARSETYPE must also be present */
                LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_IFFPARSETYPE);
                if (LocatedTag)
                {
                    ScanMode = (uint8) LocatedTag->ta_Arg;

                    /* Make sure ScanMode is legal */
                    if ( (ScanMode != LOADTEXTURE_TYPE_AUTOPARSE) &&
                         (ScanMode != LOADTEXTURE_TYPE_SINGLE) &&
                         (ScanMode != LOADTEXTURE_TYPE_MULTIPLE) )
                    {
                        ret = LT_ERR_BADTAGARG;
                    }
                    /* else ... continue processing */
                }
                else
                    ret = LT_ERR_INCOMPLETEPARAMETERS;
            }
            else
                ret = LT_ERR_MUTUALLYEXCLUSIVE;
        }

        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_CALLBACK);
        if (LocatedTag)
        {
            cb = (LTCallBack) LocatedTag->ta_Arg;

            LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_CALLBACKDATA);
            if (LocatedTag)
            {
                cbdata = (void *) LocatedTag->ta_Arg;
            }
            else
                ret = LT_ERR_INCOMPLETEPARAMETERS;
        }

        LocatedTag = FindTagArg(tags, LOADTEXTURE_TAG_SPRITETYPE);
        if (LocatedTag)
        {
            spritetype = (uint32) LocatedTag->ta_Arg;
        }

        /* --------- Start actually doing work --------- */

        /* If they gave us a filename, get an IFFParse from the
         * IFF folio for it.
         */
        if ( (filename) && (ret >= 0) )
        {
            ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, filename, TAG_END);
            if (ret < 0)
                p = (IFFParser *) NULL;
        }

        /* If all has gone well so far ... */
        if (ret >= 0)
        {
            /* Should we be parsing for the header? */
            if (ScanMode == LOADTEXTURE_TYPE_AUTOPARSE)
            {
                /* Scan for the first chunk */
                ret = ParseIFF (p, IFF_PARSE_STEP);
                if (ret >= 0)
                {
                    ContextNode     *cn;

                    /* We're expecting one of two Type/ID pairs:
                     * FORM TXTR implies a single-texture UTF file.
                     * LIST TXTR implies one or more textures in a file.
                     */
                    cn = GetCurrentContext(p);

                    if ( (cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_TXTR) )
                        ScanMode = LOADTEXTURE_TYPE_SINGLE;
                    else if ( (cn->cn_ID == ID_LIST) || (cn->cn_ID == ID_CAT) )
                        ScanMode = LOADTEXTURE_TYPE_MULTIPLE;
                    else
                        ret = LT_ERR_UNKNOWNIFFFILE;
                }
            }
            if (ScanMode == LOADTEXTURE_TYPE_SINGLE)
            {
                SpriteObj           *sp;
                bool                 skip = FALSE;

                sp = CreateSprite(spritetype);
                if (sp)
                {
                    /* Ask the IFF folio to collect any of our chunks */
                    ret = RegisterPropChunks(p, &Props[0]);
                    if (ret >= 0)
                    {
                        /* As weird as it sounds, to be consistent, if they
                         * defined a callback -- but are trying to load a
                         * single-texture file, still honor the callback
                         * and permit them to say 'yes' or 'no' to loading
                         * the texture.
                         */
                        if (cb)
                        {
                            if ( (*cb)(0, cbdata) == LOADTEXTURE_OK)
                                ret = LoadTXTR(p, sp);
                            else
                                skip = TRUE;
                        }
                        else    /* Otherwise, just load it */
                            ret = LoadTXTR(p, sp);
                    }

                    if ( (ret >= 0) && (!skip) )
                        AddTail(SpriteList, &sp->spr_Node);
                    else
                        Spr_Delete(sp);
                }
                else
                    ret = LT_ERR_NOMEM;
            }
            if (ScanMode == LOADTEXTURE_TYPE_MULTIPLE)
            {
                ret = LoadMultipleTXTR(p, SpriteList, cb, cbdata, spritetype);
            }
        }

        /* --------- Allocation Cleanup --------- */
        /* In the event we created an IFFParser for their filename, */
        /* Delete it as cleanup */
        if ( (filename) && (p) )
            DeleteIFFParser(p);

        /* If an error, and we had loaded some textures, free them */
        if (ret < 0)
            Spr_UnloadTexture(SpriteList);

        CloseIFFFolio();
    }

    return(ret);
}


