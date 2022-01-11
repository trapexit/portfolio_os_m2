/*
 *  @(#) iconfs.c 96/02/28 1.1
 *  Obtaining an Icon against a filesystem or filepath.
 */

#include <stdio.h>
#include <string.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <ui/icon.h>
#include <misc/iff.h>
#include <graphics/frame2d/loadtxtr.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"

#define TEMPBUFF    512

/**
|||	AUTODOC -internal -class Icon -name LoadIconFS
|||	The portion of LoadIcon() that deals with FS Icons.
|||
|||	  Synopsis
|||
|||	    Err LoadIconFS(char *path, Icon **iconout);
|||
|||	  Description
|||
|||	    This function stands as the part of LoadIcon() that deals
|||	    with obtaining an icon from a filesystem path.
|||	    The algorithm used here is complex.  Some general 
|||	    psuedo-code follows:
|||
|||	  - Try sending CMD_GETICON to the file.
|||	  - If that doesn't work,
|||	      - Find the device stack the FS uses for this file.
|||	      - Walk that stack down to the bottom device on the stack.
|||	      - If that bottom device has a HWResource, use the
|||	        HWResource (HardwareID) to locate the icon associated
|||	        this resource.  (From Dipir).
|||	      - If that doesn't work, try to get the icon from the
|||	        given device's driver's DDF file.
|||	          - If that fails, there's no recourse -- tell 'em that
|||	            their icon doesn't exist.
|||	        
|||	  Arguments
|||
|||	    path
|||	        A filepath denoting a file, directory, or filesystem.
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

Err LoadIconFS(char *path, Icon **iconout)
{
    Err              ret;
    Item             file;
    Icon            *icon;

    /* --------- Initialization --------- */
    ret =            0;

    /* --------- Try and open the file --------- */
    icon = (Icon *) AllocMem(sizeof(Icon), MEMTYPE_ANY);
    if (icon)
    {
        memset(icon, 0, sizeof(Icon));
        *iconout = icon;
        
        file = OpenFile(path);
        if (file >= 0)
        {
            Item         r;
    
            /* First, try and send CMD_GETICON to the file */
            r = CreateIOReq(0, 0, file, 0);
            if (r >= 0)
            {
                IOInfo   ii;
                void    *tb;
            
                memset(&ii, 0, sizeof(IOInfo));
                tb = (void *) AllocMem(TEMPBUFF, MEMTYPE_ANY);
                if (tb)
                {
                    IOReq       *req;
                    ii.ioi_Recv.iob_Buffer = tb;
                    ii.ioi_Recv.iob_Len = TEMPBUFF;
                    ii.ioi_Command = CMD_GETICON;
                    DoIO(r, &ii);
                    req = (IOReq *) LookupItem(r);
                    if (req)
                    {
                        ret = req->io_Error;

                        if (!ret)
                        {
                            SpriteObj *sp;
                            
                            sp = Spr_Create(0);
                            if (sp)
                            {
                                ret = ConvertDipirToSprite(tb, sp);
                                if (ret >= 0)
                                {
                                    PrepList(&icon->SpriteObjs);
                                    AddTail(&icon->SpriteObjs, &sp->spr_Node);
                                }
                                else
                                    ret = ICON_ERR_NOMEM;
    
                                if (ret < 0)
                                    Spr_Delete(sp);
                            }
                            else
                                ret = ICON_ERR_NOMEM;
                        }
                        else    /* CMD_GETICON failed */
                        {
                            FileSystemStat      fsstat;

                            /* Send a FILECMD_FSSTAT */
                            ii.ioi_Recv.iob_Buffer = &fsstat;
                            ii.ioi_Recv.iob_Len = sizeof(FileSystemStat);
                            ii.ioi_Command = FILECMD_FSSTAT;
                            DoIO(r, &ii);
                            ret = req->io_Error;
                            if (ret == 0)
                            {
                                Device *td;
                                Item    tditem;

                                /* Grab the top-of-the-stack Item */    
                                tditem = fsstat.fst_RawDeviceItem;

                                /* Walk the stack to the bottom */
                                do
                                {
                                    td = (Device *) LookupItem(tditem);
                                    if (!td)
                                        break;
                                    tditem = td->dev_LowerDevice;
                                }
                                while (td->dev_LowerDevice);
                            
                                if (td)
                                {
                                    ret = 0;
                                    if (td->dev_HWResource)
                                    {
                                        SpriteObj *sp;
                                        
                                        sp = Spr_Create(0);
                                        if (sp)
                                        {
                                            ret = ConvertHWIDToSprite(td->dev_HWResource, sp);
                                            if (ret >= 0)
                                            {
                                                PrepList(&icon->SpriteObjs);
                                                AddTail(&icon->SpriteObjs, &sp->spr_Node);
                                            }
                
                                            if (ret < 0)
                                                Spr_Delete(sp);
                                        }
                                        else
                                            ret = ICON_ERR_NOMEM;
                                                    
                                    }
                                    /* If no HWResource, or if Dipir */
                                    /* didn't give us an icon, try to find */
                                    /* one in the DDF file itself */
                                    if ( (!td->dev_HWResource) || (ret < 0) )
                                    {
                                        char ddfpath[80];

                                        /* Try to get the icon from the DDF */
                
                                        if (td->dev_Driver->drv.n_Name)
                                        {
                                            ret = FindDDFPath(td->dev_Driver->drv.n_Name, ddfpath, 79);
                                            if (ret >= 0)
                                                ret = LoadIconDDF(ddfpath, iconout);
                                        }
                                        else
                                            ret = ICON_ERR_NOTFOUND;
                                    } /* If (HWResource failed), or no HWR .. */
                                } /* if (td) */
                                else
                                    ret = ICON_ERR_NOTFOUND;

                            }   /* if ret == 0 */
                            
                        } /* else .. GETICON failed */
                    } /* if (req) */
                    
                    FreeMem(tb, TEMPBUFF);
                }   /* if tb */
                else
                    ret = ICON_ERR_NOMEM;
    
                DeleteItem(r);
            } /* if (r >= 0) */
            else
                ret = r;
            
            CloseFile(file);
        }
        else
            ret = file;

        if (ret < 0)
            FreeMem(icon, sizeof(Icon));
    }
    else
        ret = ICON_ERR_NOMEM;
    
    return(ret);
}

