/*
 *  @(#) iconhw.c 96/04/10 1.3
 *  Converting Dipir HW Icons to SpriteObjs.
 */

#include <string.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <ui/icon.h>
#include <misc/iff.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/loadtxtr.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"

/*
 * This exists solely because the kernel call to obtain a
 * DIPIR Icon for a given HWID is a supervisor-level call.
 * Therefore, we have a SWI in the icon folio, called only
 * by the icon folio, for getting this information from user
 * mode.  internalGetHWIcon() is the actual code for doing 
 * this.  The external 'attachment' is in stubs.s.
 * For more information, see the kernel/dipir function
 * ChannelGetHWIcon().
 */
int32 internalGetHWIcon(HardwareID id, void *b, uint32 buflen)
{
    if (IsMemWritable(b, buflen))
        return(ChannelGetHWIcon(id, b, buflen));
    else
        return(ICON_ERR_BADPTR);
}

/**
|||	AUTODOC -internal -class Icon -name ConvertDipirToSprite
|||	Converts a Dipir-style icon to a frame2d SpriteObj
|||
|||	  Synopsis
|||
|||	    Err ConvertDipirToSprite(VideoImage *vi, SpriteObj *sp);
|||
|||	  Description
|||
|||	    This function takes a VideoImage *, the format for Dipir
|||	    style icons, and converts the data within into texture
|||	    and possibly PIP data for the SpriteObj given.  After the
|||	    call is made, the VideoImage pointer is no longer referenced
|||	    and can be freed.  Any attachments made to the SpriteObj
|||	    will be automatically freed upon Spr_Delete().
|||
|||	  Arguments
|||
|||	    vi
|||	        Pointer to the VideoImage structure and data.
|||	
|||	    sp
|||	        Pointer to a SpriteObj to attach texture and PIP data to.
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

Err ConvertDipirToSprite(VideoImage *vi, SpriteObj *sp)
{
    Err                  ret;
    void                *txdata;
    uint32              *clutptr;
    uint8               *dataptr;

    static const uint32 StdClut1[] = { 0x000000, 0xFFFFFF };

    /* The type of image contained -- is there a CLUT? */

    switch (vi->vi_Type)
    {
        case VI_CLUT:
            /* A CLUT follows the header.  Bitmap data follows the clut */
            clutptr = (uint32 *) (vi+1);
            dataptr = ((uint8 *)(vi+1)) + (*clutptr);
            break;
        case VI_STDCLUT:
	    /* Use "standard" CLUT.  Only supported for depth == 1. */
	    switch (vi->vi_Depth)
	    {
	    case 1:
                clutptr = (uint32 *) StdClut1; 
		break;
	    default:
		/* Should not happen. */
		clutptr = (uint32 *) NULL;
		break;
	    }
            dataptr = ((uint8 *) (vi+1));
            break;
        default:
            /* Presumably direct, literal RGB data.  No CLUT. */
            clutptr = (uint32 *) NULL;
            dataptr = (uint8 *) (vi+1);
            break;
    }

    /* Copy out the data portion */
    txdata = AllocMem(vi->vi_Size, MEMTYPE_TRACKSIZE);
    if (txdata)
    {
        uint32  expf, BitsPerTexel;

        memcpy(txdata, dataptr, vi->vi_Size);

        BitsPerTexel = vi->vi_Depth;

        /* The CLT EXPF value appears to be black magic */
        /* The bit mappings are, according to the hardware */
        /* doc next to me:
         * 0x1000 = LITERAL (no PIP/CLUT -- straight RGB values)
         * 0x0800 = HasAlpha (indicates that Alpha data exists )
         * 0x0400 = HasColor (indicates that Color data exists )
         * 0x0200 = HasSSB (indicates that an SSB exists)
         * 0x00F0 = 4 bits of Alpha depth
         * 0x000F = 4 bits of Color depth
         * For literal textures, the color depth in terms of the
         * number of bits per color element, rather than pixel.
         *
         * Example: for 16 bpp ...
         * A value of 5 = 5 bits for R, 5 for B, and 5 for G. = 15.
         * That's not a valid M2 #, so we allocate the last bit
         * as an (unused) SSB.  LITERAL + HasColor + 5 = $1605.
         */
        if (vi->vi_Type != VI_DIRECT)
        {
            /* For PIP/Clut icons */
            expf = 0x400 + vi->vi_Depth;    /* Color bit + # bpp */
        }
        else
        {
            /* For literal/direct icons */
            if (vi->vi_Depth == 16)
            {
                BitsPerTexel = 16;
                expf = 0x1605; 
            }
            if (vi->vi_Depth == 32)
            {
                BitsPerTexel = 24;
                expf = 0x1408;
            }
        }

        /* Attach the data to the spriteobj */
        ret = Spr_CreateTxData(sp, txdata, vi->vi_Width, vi->vi_Height,
                                BitsPerTexel, expf);
        if (ret >= 0)
        {
            sp->spr_Flags |= SPR_SYSTEMTEXTUREDATA;
            txdata = (uint8 *) NULL;    /* Data is on sprite, don't free */
            /* Is there a PIP? */
            if (clutptr)
            {
                uint8           *pipdata;
                uint32           pipsize;

                pipsize = *clutptr - (sizeof(uint32));
                pipdata = (uint8 *) AllocMem( pipsize, MEMTYPE_TRACKSIZE);
                if (pipdata)
                {
                    memcpy( pipdata, &clutptr[1], pipsize);
                    ret = Spr_CreatePipControlBlock(sp, pipdata, 0, 
                            pipsize/sizeof(uint32));
                    if (ret >= 0)
                    {
                        sp->spr_Flags |= SPR_SYSTEMPIPDATA;
                        pipdata = (uint8 *) NULL;
                        ret = 0;
                    }
                    if (pipdata)
                        FreeMem(pipdata, TRACKED_SIZE);
                }
                else
                    ret = ICON_ERR_NOMEM;
            }

            if (ret >= 0)
            {
                /* Set some simple defaults */
                /* Without these set, Frame2d will display nothing */

                Spr_SetTextureAttr(sp, TXA_ColorOut, TX_BlendOutSelectTex);
                Spr_SetTextureAttr(sp, TXA_TextureEnable, 1);
                
                if (Spr_GetPIP(sp))
                    Spr_SetTextureAttr(sp, TXA_PipColorSelect, TX_PipSelectColorTable);
                else
                    Spr_SetTextureAttr(sp, TXA_PipColorSelect, TX_PipSelectTexture);
                
                Spr_SetTextureAttr(sp, TXA_PipAlphaSelect, TX_AlphaSelectTexAlpha);
                Spr_SetTextureAttr(sp, TXA_PipSSBSelect,   0);
                
                Spr_ResetCorners(sp, SPR_TOPLEFT);
            }
        }

        if (txdata)
            FreeMem(txdata, TRACKED_SIZE);
    }
    else
        ret = ICON_ERR_NOMEM;

    return(ret);
}

/**
|||	AUTODOC -internal -class Icon -name ConvertHWIDToSprite
|||	Obtains a Dipir Icon from a HWID and Converts to a SpriteObj
|||
|||	  Synopsis
|||
|||	    Err ConvertHWIDToSprite(HardwareID hwid, SpriteObj *sp);
|||
|||	  Description
|||
|||	    This function takes a Dipir HardwareID, looks up the Icon
|||	    associated with it (if found), converts that data into
|||	    M2 texture and pip data, and then finally attaches that
|||	    data to the given SpriteObj.
|||
|||	  Arguments
|||
|||	    hwid
|||	        The Dipir HWID associated with the Icon requested.
|||	
|||	    sp
|||	        Pointer to a SpriteObj to attach texture and PIP data to.
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
|||	    ConvertDipirToSprite()
|||
**/

Err ConvertHWIDToSprite(HardwareID id, SpriteObj *sp)
{
    VideoImage          *vi;
    Err                  ret;
    int32                iconlen;

    /* --------- Preinitialize --------- */
    ret             =    0;
    
    /* --------- Find Icon Size --------- */    
    iconlen = GetHWIcon(id, (void *) &iconlen, sizeof(iconlen));
    if (iconlen >= 0)
    {
        ret = ICON_ERR_NOTFOUND;  /* Weird, we gave 'em no space */
    }   
    else
    {
        iconlen = -iconlen;

        /* Make sure the size is at least reasonably sane */
        /* Yeah, I know this sucks ... */
        if (!( (iconlen < (50*1024)) && (iconlen > sizeof(VideoImage)) ))
            ret = ICON_ERR_NOTFOUND;
        else
            iconlen += 512;
    }

    /* --------- Try to read the bloody icon --------- */
    if (ret >= 0)
    {
        vi = (VideoImage *) AllocMem(iconlen, MEMTYPE_TRACKSIZE);
        if (vi)
        {
            int32       retlen;
    
            retlen = GetHWIcon(id, (void *) vi, iconlen);
            if (retlen > 0)
            {
                ret = ConvertDipirToSprite(vi, sp);
            }
            else
            {
                if (retlen < 0)
                    ret = ICON_ERR_OVERSIZEDICON;
                else
                    ret = ICON_ERR_NOTFOUND;
            }
            
            FreeMem(vi, TRACKED_SIZE);
        }
        else
            ret = ICON_ERR_NOMEM;
    }

    return(ret);
}

/**
|||	AUTODOC -internal -class Icon -name LoadIconDIPIR
|||	The portion of LoadIcon() that fetches icons from Dipir
|||
|||	  Synopsis
|||
|||	    Err LoadIconDIPIR(HardwareID hwid, Icon **icon);
|||
|||	  Description
|||
|||	    This stands as the particular part of the LoadIcon()
|||	    function that deals with obtaining and loading 
|||	    Dipir icons.
|||
|||	  Arguments
|||
|||	    hwid
|||	        The Dipir HardwareID to obtain an icon for.
|||	
|||	    icon
|||	        A pointer to an Icon *, to be filled in by this
|||	        function.
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

Err LoadIconDIPIR(HardwareID hwid, Icon **icon)
{

    Err          ret;
    Icon        *i;

    /* --------- Preinitialize --------- */
    ret     =    0;


    /* --------- Allocate the Icon structure --------- */

    i   =   (Icon *)    AllocMem(sizeof(Icon), MEMTYPE_ANY);
    if (i)
    {   
        SpriteObj       *sp;

        memset(i, 0, sizeof(Icon));

        *icon   =        i;
            
        sp = Spr_Create(0);
        if (sp)
        {
            /* Actually convert the data */
            ret = ConvertHWIDToSprite(hwid, sp);
            if (ret >= 0)
            {
                /* If everything went well, add it to the list */
                PrepList( &i->SpriteObjs );
                AddTail( &i->SpriteObjs, &sp->spr_Node );
            }
            if (ret < 0)
                Spr_Delete(sp);
        }
        if (ret < 0)
            FreeMem(i, sizeof(Icon));
    }
    else
        ret = ICON_ERR_NOMEM;

    return(ret);
}


