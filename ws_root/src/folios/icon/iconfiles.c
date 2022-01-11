
/*
 *  @(#) iconfiles.c 96/02/29 1.2
 *  Loading and saving Icon files directly.
 */

#include <string.h>
#include <kernel/mem.h>
#include <ui/icon.h>
#include <misc/iff.h>
#include <graphics/frame2d/loadtxtr.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"

#ifndef ID_TXTR
#define ID_TXTR     MAKE_ID('T','X','T','R')
#endif

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
|||	AUTODOC -internal -class Icon -name LoadIconIFF
|||	The portion of LoadIcon() that deals with IFF files.
|||
|||	  Synopsis
|||
|||	    Err LoadIconIFF(IFFParser *p, Icon **iconout);
|||
|||	  Description
|||
|||	    This function stands as the part of LoadIcon() that deals 
|||	    with IFF-based icon files.
|||
|||	  Arguments
|||
|||	    p
|||	        An IFFParser *, from the IFF Folio.
|||	  
|||	    iconout
|||	        A pointer to an Icon *, to be filled in by this function.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Icon folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/icon.h>
|||
|||	  See Also
|||
|||	    LoadIcon()
|||
**/

Err LoadIconIFF(IFFParser *p, Icon **iconout)
{
    Err          ret;
    bool         ReadInIDTA, ReadInTextures;
    Icon        *icon;
    uint32       Depth;

    /* --------- Preinitialization --------- */
    ReadInIDTA      =               FALSE;
    ReadInTextures  =               FALSE;
    Depth           =               1;

    /* --------- Allocate an Icon structure --------- */
    icon = (Icon *) AllocMem(sizeof(Icon), MEMTYPE_ANY);
    if (icon)
    {   
        memset(icon, 0, sizeof(Icon));
        /* Tell the App about it */
        *iconout = icon;

        /* --------- Main loop --------- */
        while (TRUE)
        {
            /* Start parsing the IFF stream */
            ret = ParseIFF(p, IFF_PARSE_STEP);
    
            /* If we hit an EOF marker, stop scanning. */
            if (ret == IFF_PARSE_EOF)
                break;
    
            /* If we hit an EOC marker, stop scanning -- as we're
             * leaving the FORM ICON chunk that we were called in
             */
            if (ret == IFF_PARSE_EOC)
            {
                Depth--;
                if (Depth == 0)
                {
                    ret = 0;
                    break;
                }
                else
                    continue;
            }
                
            if (ret == 0)
            {
                ContextNode     *cn;
        
                Depth++;
                
                cn = GetCurrentContext(p);

                /* What kind of chunk follows?
                 * FORM TXTR == 1 texture alone
                 * LIST TXTR or CAT TXTR == 1 or more textures
                 * IDTA == Icon-specific data
                 */
                if ( (cn->cn_Type == ID_TXTR) && (cn->cn_ID == ID_FORM) )
                {
                    ret = Spr_LoadTextureVA(&icon->SpriteObjs, 
                                    LOADTEXTURE_TAG_IFFPARSER, p,
                                    LOADTEXTURE_TAG_IFFPARSETYPE, LOADTEXTURE_TYPE_SINGLE,
                                    TAG_END);
                    if (ret >= 0)
                    {
                        ReadInTextures = TRUE;
                        Depth--;
                    }
                }
                else if ( ( (cn->cn_Type == ID_TXTR) && (cn->cn_ID == ID_LIST) ) ||
                          ( (cn->cn_Type == ID_TXTR) && (cn->cn_ID == ID_CAT ) ) )
                {
                    ret = Spr_LoadTextureVA(&icon->SpriteObjs, 
                                    LOADTEXTURE_TAG_IFFPARSER, p,
                                    LOADTEXTURE_TAG_IFFPARSETYPE, LOADTEXTURE_TYPE_MULTIPLE,
                                    TAG_END);
                    if (ret >= 0)
                    {
                        ReadInTextures = TRUE;
                        Depth--;
                    }
                }
                else if ( (cn->cn_Type == ID_ICON) && (cn->cn_ID == ID_IDTA) )
                {
                    IconData        id;
                    
                    ret = ReadChunk(p, &id, sizeof(IconData));
                    if (ret >= 0)
                    {
                        /* Copy data into the Icon itself */
                        icon->TimeBetweenFrames = id.TimeBetweenFrames;
                        strncpy(&icon->ApplicationName[0], &id.ApplicationName[0], 31);
                        icon->ApplicationName[31]=0;
                        ReadInIDTA = TRUE;
                    }
                }
                else
                {
                    /* This is an unknown chunk -- skip it and it's
                     * children entirely
                     */
                    SkipOverThisLevel(p);
                }
                if (ret < 0)
                    break;
            }
			else
				break;
        }
    }
    else
        ret = ICON_ERR_NOMEM;

    if ( (!ReadInIDTA) || (!ReadInTextures) )
        ret = ICON_ERR_INCOMPLETE;

    /* --------- If in error, clean up resources --------- */
    if (ret < 0)
    {
        /* If any textures were loaded, free 'em now */
        if (ReadInTextures)
            Spr_UnloadTexture(&icon->SpriteObjs);

        /* Free the Icon struct itself */
        if (icon)
        {
            /* Don't let the App get any nutty ideas */
            *iconout =      (Icon *)    NULL;
            FreeMem(icon, sizeof(Icon));
        }
    }

    /* As long as nothing went wrong, give 'em zero on return */
    if (ret > 0)
        ret = 0;

    return(ret);
}

Err SaveIconIFF(IFFParser *p, char *utfs, char *appname, TimeVal *tbf)
{
    Err ret;

    /* --------- Write out the FORM ICON chunk header --------- */
    ret = PushChunk(p, ID_ICON, ID_FORM, IFF_SIZE_UNKNOWN_32);
    if (ret >= 0)
    {
        /* Write out the IDTA chunk */
        ret = PushChunk(p, 0, ID_IDTA, IFF_SIZE_UNKNOWN_32);
        if (ret >= 0)
        {
            IconData        id;

            /* Build the Icon Data */
            strncpy(&id.ApplicationName[0], appname, 31);
            id.ApplicationName[31]=0;
            memcpy(&id.TimeBetweenFrames, tbf, sizeof(TimeVal));
            
            ret = WriteChunk(p, &id, sizeof(IconData));
            
            /* Pop us out of the IDTA */    
            if (ret >= 0)
            {
                ret = PopChunk(p);

                /* Now clone-copy the contents of the UTF file into
                 * this file
                 */

                if (ret >= 0)
                {
                    IFFParser   *texture;

                    ret = CreateIFFParserVA(&texture, FALSE, IFF_TAG_FILE,
                                            utfs, TAG_END);
                    if (ret >= 0)
                    {
                        while (TRUE)
                        {
                            ContextNode *chunk;
    
                            ret = ParseIFF(texture, IFF_PARSE_RAWSTEP);
                        
                            switch(ret)
                            {       
                                case 0:
                                {
                                    chunk = GetCurrentContext(texture); 
                                    ret = PushChunk(p, chunk->cn_Type, chunk->cn_ID, IFF_SIZE_UNKNOWN_32);
        
                                    if (ret >= 0)
                                    {
                                        if (!chunk->cn_Offset)
                                        {
                                            uint8 *temp;
                                            temp = AllocMem(chunk->cn_Size, MEMTYPE_ANY);
                                            if (temp)
                                            {
                                                ret = ReadChunk(texture, temp, chunk->cn_Size);
                                                if (ret >= 0)
                                                {
                                                    WriteChunk(p, temp, chunk->cn_Size);
                                                }
                                                FreeMem(temp, chunk->cn_Size);
                                            }
                                    
                                        }
                                    }
                                    break;
                                    
                                }
                
                                case IFF_PARSE_EOC:
                                {
                                    PopChunk(p);
                                    break;
                                }
                                case IFF_PARSE_EOF:
                                default:
                                {
                                    break;
                                }
                            }
                            if (ret == IFF_PARSE_EOF)
                            {
                                ret = 0;    
                                break;
                            }
                            if ( (ret < 0) && (ret != IFF_PARSE_EOC) )
                                break;
                        }
                        DeleteIFFParser(texture);
                    }
                }
            
            }

        }
    
        /* Pop us out of the FORM ICON */
        if (ret >= 0)
            ret = PopChunk(p);
    }   

    return(ret);
}

