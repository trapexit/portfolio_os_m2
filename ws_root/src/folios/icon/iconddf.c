/*
 *  @(#) iconddf.c 96/02/29 1.2
 *  Obtaining an Icon from a DDF file.
 */

#include <stdio.h>
#include <string.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <ui/icon.h>
#include <misc/iff.h>
#include <graphics/frame2d/loadtxtr.h>
#include <file/filefunctions.h>
#include <file/directoryfunctions.h>
#include <file/filesystem.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"

#ifndef ID_VERS
#define     ID_VERS     MAKE_ID('V','E','R','S')
#endif

/**
|||	AUTODOC -internal -class Icon -name LoadIconDDF
|||	The portion of LoadIcon() that deals with DDF Icons.
|||
|||	  Synopsis
|||
|||	    Err LoadIconDDF(char *path, Icon **iconout);
|||
|||	  Description
|||
|||	    This function attempts to locate an icon in a DDF
|||	    file and load it into memory.
|||	        
|||	  Arguments
|||
|||	    path
|||	        A filepath denoting a DDF file.
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
Err LoadIconDDF(char *path, Icon **iconout)
{
    Err              ret;
    uint32           Depth;
    bool             LoadedIcon;
    IFFParser       *p;

    /* --------- Initialization --------- */
    Depth       =    1;
    LoadedIcon  =    FALSE;

    /* --------- Try and open the IFF file --------- */

    ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, path, TAG_END);
    if (ret >= 0)
    {
        /* Entering main parsing loop */
        while (TRUE)
        {
            ret = ParseIFF(p, IFF_PARSE_RAWSTEP);

            /* Did we hit the end of file? */
            if (ret == IFF_PARSE_EOF)
            {
                ret = 0;
                break;
            }

            /* Are we leaving a chunk? */
            if (ret == IFF_PARSE_EOC)
            {
                Depth--;
                if (!Depth)
                {
                    ret = 0;
                    break;
                }
                else
                    continue;
            }

            /* Did we just enter a chunk? */
            if (!ret)
            {
                ContextNode         *cn;

                /* We're going in one level ... */
                Depth++;
    
                cn = GetCurrentContext(p);
                if ( (cn->cn_Type == ID_ICON) && (cn->cn_ID == ID_FORM) )
                {
                    /* Found a FORM ICON */
                    /* Try to load an icon from the file ... */
                    ret = LoadIconIFF(p, iconout);
                    if (ret >= 0)
                    {
                        LoadedIcon = TRUE;
                        /* Don't try to load more than one ... */
                        break;
                    }
                    Depth--;
                }
                
            } /* if (entering a chunk)  */
			else
				break;
        } /* while TRUE */
        DeleteIFFParser(p);
    }

    /* If nothing really went wrong, except that we didn't */
    /* Find any icons, that's worth reporting as bad. */
    if ( (ret >= 0) && (!LoadedIcon) )
        ret = ICON_ERR_NOTFOUND;

    return(ret);
}

/**
|||	AUTODOC -internal -class Icon -name FindDDFPath
|||	Given the name of a driver, find it's DDF file.
|||
|||	  Synopsis
|||
|||	    Err FindDDFPath(char *driver, char *outpath, int32 outlen);
|||
|||	  Description
|||
|||	    This function, given the name of a driver file, 
|||	    attempts to search all volumes to locate the latest
|||	    revision of that driver's DDF file.
|||	        
|||	  Arguments
|||
|||	    driver
|||	        The name of the driver whose DDF we're to find.
|||	  
|||	    outpath
|||	        A pointer to a buffer where we should write the
|||	        path of the DDF file, once located.
|||
|||	    outlen
|||	        The amount of storage space in outpath, given so that
|||	        we don't overwrite the buffer.
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

Err FindDDFPath(char *driver, char *outpath, int32 outlen)
{
    Err              ret;
    Directory       *d;
    char             bestpath[132];
    int32            bestversion;

    /* --------- Preinitialize --------- */
    bestpath[131] =  0;
    bestversion =    -1;

    /* --------- Scan all root devices --------- */
    d = OpenDirectoryPath("/");
    if (d)
    {
        DirectoryEntry  de;

        while (TRUE)
        {
            ret = ReadDirectory(d, &de);
            if (ret >= 0)
            {
                /* --------- Try to find the ddf file on each --------- */
                char temppath[132];
                if ( (strlen(driver)+strlen(&de.de_FileName[0])) < 100)
                {
                    int32                tempversion;
                    IFFParser           *p;
                    sprintf(temppath, "/%s/System.m2/Devices/%s.ddf",
                                        &de.de_FileName[0], driver);
                    
                    /* Does the bloody thing even EXIST? */
                
                    tempversion = -1;

                    /* Try to get the version from the IFF chunk */
                    /* Yeesh, what a mess */
                
                    ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE,
                                            temppath, TAG_END);
                    if (ret >= 0)
                    {
                        while (ret >= 0)
                        {
                            ret = ParseIFF(p, IFF_PARSE_RAWSTEP);
                            if (ret == IFF_PARSE_EOF)
                            {
                                ret = 0;    
                                break;
                            }
                            if (ret == 0)
                            {
                                ContextNode     *cn;
                    
                                cn = GetCurrentContext(p);
                                if ( cn->cn_ID == ID_VERS )
                                {
                                    ret = ReadChunk(p, &tempversion, 
                                                    sizeof(uint32) );
                                    break;
                                }
                            }
                            
                        }
                        TOUCH(ret); /* TO keep compiler quiet */
                        DeleteIFFParser(p);

                        /* If we still couldn't find a version, */
                        /* Adopt this DDF only if no others exist */
                        if ( (tempversion > bestversion) )
                        {
                            strcpy(bestpath, temppath); 
                            bestversion = tempversion;
                        }
                    } /* if (OpenDirectoryPath()) succeeded */
                    
                } /* Is the string length reasonable? */
            } /* Did ReadDirectory() succeed? */
            else
                break;
        }
        CloseDirectory(d);
    }

    if (bestpath[0])
    {
        strncpy(outpath, bestpath, outlen);
        ret = 0;
    }
    else
        ret = ICON_ERR_NOTFOUND;

    return(ret);
}


