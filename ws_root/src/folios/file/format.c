/* @(#) format.c 96/09/09 1.4 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/devicecmd.h>
#include <kernel/super.h>
#include <misc/event.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/filesystemdefs.h>
#include <string.h>


/*****************************************************************************/


/* Dave just doesn't grasp what header files are for... */
extern Semaphore         *fsListSemaphore;
extern FileFolioTaskData *SetupFileFolioTaskData(void);
extern Item MountFileSystemSWI(Item deviceItem, uint32 blockOffset);


/*****************************************************************************/


Err FormatFileSystemSWI(Item device, const char *name, const TagArg *tags)
{
Item            iorItem;
IOReq          *ior;
Err             result;
DeviceStatus    stat;
char           *fsName;
uint32          fsType;
bool            found;
FileSystemType *fst;
TagArg         *tag;
uint8           oldPriv;

    fsName = NULL;
    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case FORMATFS_TAG_FSNAME: fsName = (char *)tag->ta_Arg;
                                      break;

            case FORMATFS_TAG_OFFSET: /* ignored for now */
                                      break;

            default                 : return FILE_ERR_BADTAG;
        }
    }

    SetupFileFolioTaskData();

    oldPriv = PromotePriv(CURRENTTASK);

    iorItem = result = CreateIOReq(NULL, 0, device, 0);
    if (iorItem >= 0)
    {
        ior = IOREQ(iorItem);

        ior->io_Info.ioi_Command         = CMD_STATUS;
        ior->io_Info.ioi_Recv.iob_Buffer = &stat;
        ior->io_Info.ioi_Recv.iob_Len    = sizeof(stat);
        result = SuperInternalDoIO(ior);
        if (result >= 0)
        {
            if (stat.ds_DeviceUsageFlags & DS_USAGE_FILESYSTEM)
            {
                SuperInternalLockSemaphore(fsListSemaphore, SEM_WAIT | SEM_SHAREDREAD);

                found = FALSE;

                if (fsName)
                {
                    ScanList(&fileFolio.ff_FileSystemTypes, fst, FileSystemType)
                    {
                        if (strcasecmp(fst->fst.n_Name, fsName) == 0)
                        {
                            found = TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    ior->io_Info.ioi_Command         = CMD_PREFER_FSTYPE;
                    ior->io_Info.ioi_Recv.iob_Buffer = &fsType;
                    ior->io_Info.ioi_Recv.iob_Len    = sizeof(fsType);
                    result = SuperInternalDoIO(ior);
                    if (result < 0)
                        fsType = VOLUME_STRUCTURE_ACROBAT;

                    ScanList(&fileFolio.ff_FileSystemTypes, fst, FileSystemType)
                    {
                        if (fst->fst_VolumeStructureVersion != 0 &&
                            fst->fst_VolumeStructureVersion == fsType)
                        {
                            found = TRUE;
                            break;
                        }
                    }
                }

                if (found)
                {
                    if (fst->fst_Format)
                    {
                        result = (* fst->fst_Format)(iorItem, &stat, name, tags);
                        if (result >= 0)
                        {
                            result = MountFileSystemSWI(device, 0);
                        }
                    }
                    else
                    {
                        result = FILE_ERR_NOSUPPORT;
                    }
                }

                SuperInternalUnlockSemaphore(fsListSemaphore);
            }
            else
            {
                result = FILE_ERR_NOSUPPORT;
            }
        }
        DeleteIOReq(iorItem);
    }

    DemotePriv(CURRENTTASK, oldPriv);

    return result;
}
