/* @(#) directory.c 96/02/07 1.12 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <string.h>


/*****************************************************************************/


struct Directory
{
    Item    dir_OpenFileItem;
    Item    dir_IOReqItem;
    uint32  dir_EntryNum;
    void   *dir_Cookie;
};

#define IsDirectory(dir) (dir && (dir->dir_Cookie == dir))


/*****************************************************************************/


Directory *OpenDirectoryItem(Item openFileItem)
{
Directory *dir;
FileStatus fileStatus;
IOInfo     ioInfo;

    dir = AllocMem(sizeof(Directory), MEMTYPE_FILL);
    if (dir)
    {
        dir->dir_OpenFileItem = openFileItem;
        dir->dir_IOReqItem = CreateIOReq(NULL, 0, openFileItem, 0);
        if (dir->dir_IOReqItem >= 0)
        {
            memset(&ioInfo, 0, sizeof(IOInfo));
            ioInfo.ioi_Command         = CMD_STATUS;
            ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
            ioInfo.ioi_Recv.iob_Len    = sizeof(FileStatus);
            if (DoIO(dir->dir_IOReqItem, &ioInfo) >= 0)
            {
                if (fileStatus.fs.ds_DeviceFlagWord & FILE_IS_DIRECTORY)
                {
                    dir->dir_Cookie = dir;
                    return dir;
                }
            }
            DeleteItem(dir->dir_IOReqItem);
        }
        FreeMem(dir, sizeof(Directory));
    }

    return NULL;
}


/*****************************************************************************/


Directory *OpenDirectoryPath(const char *path)
{
Item       openFileItem;
Directory *result;

    openFileItem = OpenFile(path);
    if (openFileItem < 0)
        return NULL;

    result = OpenDirectoryItem(openFileItem);
    if (result == NULL)
        CloseFile(openFileItem);

    return result;
}


/*****************************************************************************/


Err ReadDirectory(Directory *dir, DirectoryEntry *de)
{
IOInfo ioInfo;

    if (!IsDirectory(dir))
        return FILE_ERR_BADPTR;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command         = FILECMD_READDIR;
    ioInfo.ioi_Offset          = (int32) ++dir->dir_EntryNum;
    ioInfo.ioi_Recv.iob_Buffer = de;
    ioInfo.ioi_Recv.iob_Len    = sizeof(DirectoryEntry);

    return DoIO(dir->dir_IOReqItem, &ioInfo);
}


/*****************************************************************************/


void CloseDirectory(Directory *dir)
{
    if (IsDirectory(dir))
    {
        DeleteItem(dir->dir_IOReqItem);
        CloseFile(dir->dir_OpenFileItem);
        dir->dir_Cookie = NULL;
        FreeMem(dir, sizeof(Directory));
    }
}
