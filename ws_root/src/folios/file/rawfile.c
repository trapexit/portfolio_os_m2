/* @(#) rawfile.c 96/09/25 1.30 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/tags.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <string.h>


/*****************************************************************************/


struct RawFile
{
    bool    rf_Readable;
    bool    rf_Writable;
    Item    rf_File;
    Item    rf_IO;
    Err     rf_Error;
    uint32  rf_OpenMode;
    uint32  rf_OriginalBytesInFile;
    uint32  rf_UsefulBlocksInFile;
    uint32  rf_FileType;
    uint8   rf_Version;
    uint8   rf_Revision;
    TimeVal rf_Date;

    /* file info */
    uint32  rf_BlockSize;
    uint32  rf_BytesInFile;
    uint32  rf_BlocksInFile;
    uint32  rf_BytePosition;
    uint32  rf_OriginalPosition;

    /* block buffer */
    uint32  rf_BlockInBuffer;
    uint8  *rf_BlockBuffer;
    bool    rf_BufferDirty;
    bool    rf_BufferValid;

    void   *rf_Cookie;
};

#define BlockNum(file) (file->rf_BytePosition / file->rf_BlockSize)

#define IsRawFile(file) (file && (file->rf_Cookie == file))


/*****************************************************************************/


static Err ReadBlocks(RawFile *file, void *buffer,
                      uint32 blockOffset, uint32 numBytes)
{
IOInfo ioInfo;

    memset(&ioInfo,0,sizeof(ioInfo));
    ioInfo.ioi_Command         = CMD_BLOCKREAD;
    ioInfo.ioi_Offset          = blockOffset;
    ioInfo.ioi_Recv.iob_Buffer = buffer;
    ioInfo.ioi_Recv.iob_Len    = numBytes;
    return DoIO(file->rf_IO,&ioInfo);
}


/*****************************************************************************/


static Err WriteBlocks(RawFile *file, const void *buffer,
                       uint32 blockOffset, uint32 numBlocks)
{
IOInfo ioInfo;
Err    result;

    if (numBlocks == 0)
        return 0;

    if (blockOffset + numBlocks > file->rf_BlocksInFile)
    {
        memset(&ioInfo,0,sizeof(ioInfo));
        ioInfo.ioi_Command = FILECMD_ALLOCBLOCKS;
        ioInfo.ioi_Offset  = blockOffset + numBlocks - file->rf_BlocksInFile;
        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
            return result;

        file->rf_BlocksInFile = blockOffset + numBlocks;
    }

    if (blockOffset + numBlocks > file->rf_UsefulBlocksInFile)
        file->rf_UsefulBlocksInFile = blockOffset + numBlocks;

    memset(&ioInfo,0,sizeof(ioInfo));
    ioInfo.ioi_Command         = CMD_BLOCKWRITE;
    ioInfo.ioi_Offset          = blockOffset;
    ioInfo.ioi_Send.iob_Buffer = (void *)buffer;
    ioInfo.ioi_Send.iob_Len    = numBlocks * file->rf_BlockSize;
    return DoIO(file->rf_IO,&ioInfo);
}


/*****************************************************************************/


static Err FlushBlockBuffer(RawFile *file)
{
Err result;

    if (file->rf_BufferDirty)
    {
        result = WriteBlocks(file, file->rf_BlockBuffer, file->rf_BlockInBuffer, 1);
        if (result >= 0)
            file->rf_BufferDirty = FALSE;

        return result;
    }

    return 0;
}


/*****************************************************************************/


static Err FillBlockBuffer(RawFile *file, uint32 blockNumber, bool writing)
{
Err result;

    if (file->rf_BufferValid && (file->rf_BlockInBuffer == blockNumber))
    {
        /* block's already loaded */
        return 0;
    }

    result = FlushBlockBuffer(file);
    if (result < 0)
        return result;

    if (file->rf_BlockBuffer == NULL)
    {
        file->rf_BlockBuffer = AllocMem(file->rf_BlockSize,MEMTYPE_NORMAL);
        if (file->rf_BlockBuffer == NULL)
            return FILE_ERR_NOMEM;
    }

    if (writing)
    {
        if (blockNumber >= file->rf_UsefulBlocksInFile)
        {
            /* the file is being extended, so there's no data to fetch */
            file->rf_BlockInBuffer = blockNumber;
            file->rf_BufferValid   = TRUE;
            return 0;
        }
    }

    result = ReadBlocks(file, file->rf_BlockBuffer,
                        blockNumber, file->rf_BlockSize);

    if (result >= 0)
    {
        file->rf_BlockInBuffer = blockNumber;
        file->rf_BufferValid   = TRUE;
    }
    else
    {
        file->rf_BufferValid   = FALSE;
    }

    return result;
}


/*****************************************************************************/


/* Record the occurance of an error */
static void RecordError(RawFile *file, Err error)
{
    file->rf_Error        = error;
    file->rf_BytePosition = file->rf_OriginalPosition; /* pretend nothing happened */
}


/*****************************************************************************/


Err ClearRawFileError(RawFile *file)
{
    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    file->rf_Error = 0;
    return 0;
}


/*****************************************************************************/


static Err internalOpenRawFile(RawFile **file, Item dir, const char *path,
                               FileOpenModes mode, bool useDir)
{
Err         result;
RawFile    *rf;
bool        created;
IOInfo      ioInfo;
FileStatus  fileStatus;

    rf = AllocMem(sizeof(RawFile),MEMTYPE_FILL);
    if (rf)
    {
        created = FALSE;
        result  = 0;

        switch (mode)
        {
            case FILEOPEN_READ              :
            case FILEOPEN_WRITE_EXISTING    :
            case FILEOPEN_READWRITE_EXISTING: break;

            case FILEOPEN_WRITE             :
            case FILEOPEN_READWRITE         : if (useDir)
                                              {
                                                  if (CreateFileInDir(dir, path) >= 0)
                                                      created = TRUE;
                                              }
                                              else
                                              {
                                                  if (CreateFile(path) >= 0)
                                                      created = TRUE;
                                              }
                                              break;

            case FILEOPEN_WRITE_NEW         :
            case FILEOPEN_READWRITE_NEW     : if (useDir)
                                                  result = DeleteFileInDir(dir, path);
                                              else
                                                  result = DeleteFile(path);

                                              if (result == FILE_ERR_NOFILE)
                                              {
                                                  result = 0;
                                              }

                                              if (result >= 0)
                                              {
                                                  if (useDir)
                                                      result = CreateFileInDir(dir, path);
                                                  else
                                                      result = CreateFile(path);

                                                  if (result >= 0)
                                                      created = TRUE;
                                              }
                                              break;


            default                         : result = FILE_ERR_BADMODE;
                                              break;
        }

        if (result >= 0)
        {
            if ((mode == FILEOPEN_READ)
             || (mode == FILEOPEN_READWRITE)
             || (mode == FILEOPEN_READWRITE_NEW)
             || (mode == FILEOPEN_READWRITE_EXISTING))
            {
                rf->rf_Readable = TRUE;
            }

            if ((mode == FILEOPEN_WRITE)
             || (mode == FILEOPEN_WRITE_NEW)
             || (mode == FILEOPEN_READWRITE)
             || (mode == FILEOPEN_READWRITE_NEW)
             || (mode == FILEOPEN_READWRITE_EXISTING))
            {
                rf->rf_Writable = TRUE;
            }

            if (useDir)
                result = rf->rf_File = OpenFileInDir(dir, path);
            else
                result = rf->rf_File = OpenFile(path);

            if (rf->rf_File >= 0)
            {
                result = rf->rf_IO = CreateIOReq(NULL,0,rf->rf_File,0);
                if (rf->rf_IO >= 0)
                {
                    memset(&ioInfo,0,sizeof(ioInfo));
                    ioInfo.ioi_Command         = CMD_STATUS;
                    ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
                    ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);
                    result = DoIO(rf->rf_IO,&ioInfo);
                    if (result >= 0)
                    {
                        if ((fileStatus.fs.ds_DeviceFlagWord & FILE_IS_DIRECTORY) == 0)
                        {
                            rf->rf_Cookie               = rf;
                            rf->rf_Error                = 0;
                            rf->rf_OpenMode             = mode;
                            rf->rf_OriginalBytesInFile  = fileStatus.fs_ByteCount;
                            rf->rf_BytesInFile          = fileStatus.fs_ByteCount;
                            rf->rf_UsefulBlocksInFile   = fileStatus.fs.ds_DeviceBlockCount;
                            rf->rf_BlocksInFile         = fileStatus.fs.ds_DeviceBlockCount;
                            rf->rf_BlockSize            = fileStatus.fs.ds_DeviceBlockSize;
                            rf->rf_FileType             = fileStatus.fs_Type;
                            rf->rf_Version              = fileStatus.fs_Version;
                            rf->rf_Revision             = fileStatus.fs_Revision;
                            rf->rf_Date                 = fileStatus.fs_Date;

                            *file = rf;

                            return 0;
                        }
                        result = FILE_ERR_NOTAFILE;
                    }
                    DeleteIOReq(rf->rf_IO);
                }
                CloseFile(rf->rf_File);
            }
        }

        if (created)
        {
            if (useDir)
                DeleteFileInDir(dir, path);
            else
                DeleteFile(path);
        }

        FreeMem(rf,sizeof(RawFile));
    }
    else
    {
        result = FILE_ERR_NOMEM;
    }

    *file = NULL;

    return result;
}


/*****************************************************************************/


Err OpenRawFile(RawFile **file, const char *path, FileOpenModes mode)
{
    return internalOpenRawFile(file, 0, path, mode, FALSE);
}


/*****************************************************************************/


Err OpenRawFileInDir(RawFile **file, Item dir, const char *path, FileOpenModes mode)
{
    return internalOpenRawFile(file, dir, path, mode, TRUE);
}


/*****************************************************************************/


Err CloseRawFile(RawFile *file)
{
Err    result;
IOInfo ioInfo;

    result = 0;
    if (file)
    {
        if (!IsRawFile(file))
            return FILE_ERR_BADFILE;

        result = file->rf_Error;
        if (result >= 0)
        {
            if (file->rf_Writable)
            {
                result = FlushBlockBuffer(file);

                if (file->rf_OriginalBytesInFile != file->rf_BytesInFile)
                {
                    if (result >= 0)
                    {
                        memset(&ioInfo,0,sizeof(ioInfo));
                        ioInfo.ioi_Command = FILECMD_SETEOF;
                        ioInfo.ioi_Offset  = file->rf_BytesInFile;
                        result = DoIO(file->rf_IO,&ioInfo);
                    }
                }
            }
        }

        DeleteIOReq(file->rf_IO);
        CloseFile(file->rf_File);
        FreeMem(file->rf_BlockBuffer,file->rf_BlockSize);

        file->rf_Cookie = NULL;

        FreeMem(file,sizeof(RawFile));
    }

    return result;
}


/*****************************************************************************/


int32 ReadRawFile(RawFile *file, void *buffer, int32 numBytes)
{
int32  amount;
int32  result;
uint32 blockOffset;
uint32 numBlocks;
uint32 extraRoom;
uint32 mod;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    if (!file->rf_Readable)
        return FILE_ERR_BADMODE;

    if (file->rf_Error < 0)
        return file->rf_Error;

    if (numBytes < 0)
        return FILE_ERR_BADCOUNT;

    /* keep track of where we started */
    file->rf_OriginalPosition = file->rf_BytePosition;

    /* make sure we don't try to read beyond the end of the file */
    if (file->rf_BytePosition + numBytes > file->rf_BytesInFile)
    {
        extraRoom = numBytes - (file->rf_BytesInFile - file->rf_BytePosition);
        numBytes  = file->rf_BytesInFile - file->rf_BytePosition;
    }
    else
    {
        extraRoom = 0;
    }

    /* extraRoom contains the number of extra bytes that the buffer contains
     * compared to the number of bytes left in the file.
     */

    blockOffset = file->rf_BytePosition % file->rf_BlockSize;
    if (blockOffset)
    {
        result = FillBlockBuffer(file, BlockNum(file), FALSE);
        if (result < 0)
        {
            RecordError(file,result);
            return result;
        }

        /* see how many bytes we can get out of the block buffer */

        if (numBytes > file->rf_BlockSize - blockOffset)
            amount = file->rf_BlockSize - blockOffset;
        else
            amount = numBytes;

        if (amount)
        {
            /* read bytes out of the block buffer */
            memcpy(buffer,&file->rf_BlockBuffer[blockOffset],amount);
            file->rf_BytePosition += amount;
            numBytes              -= amount;
            buffer                 = (void *)((uint32)buffer + amount);
        }
    }

    /* At this point, we know the byte offset within the file is aligned on a
     * block boundary.
     */

    if (extraRoom)
    {
        /* There's more room in the user's buffer than there is data.
         * We want to take advantage of this extra room if it allows
         * us to read the last block directly without having to go
         * through a temp buffer.
         */

        mod = numBytes % file->rf_BlockSize;
        if (mod)
        {
            if (extraRoom >= file->rf_BlockSize - mod)
            {
                numBlocks = (numBytes / file->rf_BlockSize) + 1;
                amount    = numBlocks * file->rf_BlockSize;

                result = ReadBlocks(file, buffer, BlockNum(file), amount);
                if (result < 0)
                {
                    RecordError(file, result);
                    return result;
                }

                file->rf_BytePosition += numBytes;
                return file->rf_BytePosition - file->rf_OriginalPosition;
            }
        }
    }

    if (numBytes >= file->rf_BlockSize)
    {
        /* There's at least a block of data to read. Transfer all
         * whole blocks directly to the user's buffer.
         */
        numBlocks = numBytes / file->rf_BlockSize;
        amount    = numBlocks * file->rf_BlockSize;

        result = ReadBlocks(file, buffer, BlockNum(file), amount);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }

        file->rf_BytePosition += amount;
        numBytes              -= amount;
        buffer                 = (void *)((uint32)buffer + amount);
    }

    if (numBytes)
    {
        /* We have some left over data to read. We must load up the
         * block buffer, and fetch the data from there.
         */

        result = FillBlockBuffer(file, BlockNum(file), FALSE);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }

        file->rf_BytePosition += numBytes;
        memcpy(buffer,file->rf_BlockBuffer,numBytes);
    }

    /* return the number of bytes read from the file */
    return file->rf_BytePosition - file->rf_OriginalPosition;
}


/*****************************************************************************/


int32 WriteRawFile(RawFile *file, const void *buffer, int32 numBytes)
{
uint32 amount;
Err    result;
uint32 blockOffset;
uint32 numBlocks;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    if (!file->rf_Writable)
        return FILE_ERR_BADMODE;

    if (file->rf_Error < 0)
        file->rf_Error = 0;

    if (numBytes < 0)
        return FILE_ERR_BADCOUNT;

    file->rf_OriginalPosition = file->rf_BytePosition;

    blockOffset = file->rf_BytePosition % file->rf_BlockSize;
    if (blockOffset)
    {
        result = FillBlockBuffer(file, BlockNum(file), TRUE);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }

        amount = file->rf_BlockSize - blockOffset;
        if (numBytes < amount)
            amount = numBytes;

        memcpy(&file->rf_BlockBuffer[blockOffset],buffer,amount);
        file->rf_BytePosition += amount;
        file->rf_BufferDirty   = TRUE;
        numBytes              -= amount;
        buffer                 = (void *)((uint32)buffer + amount);
    }

    /* At this point, we know the byte offset within the file is aligned on a
     * block boundary.
     */

    if (numBytes >= file->rf_BlockSize)
    {
        /* There's at least a block of data to write. Transfer all
         * whole blocks directly from the user's buffer.
         */

        /* We do a flush here to try and maintain sequentiality in our
         * writes. The flush could be deferred and only done when the
         * block buffer is needed, but this would tend to force backwards
         * seeks into the file to flush the buffer data.
         */
        result = FlushBlockBuffer(file);
        if (result < 0)
        {
            RecordError(file,result);
            return result;
        }

        numBlocks = numBytes / file->rf_BlockSize;
        amount    = numBlocks * file->rf_BlockSize;

        result = WriteBlocks(file, buffer, BlockNum(file), numBlocks);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }

        file->rf_BytePosition += amount;
        numBytes              -= amount;
        buffer                 = (void *)((uint32)buffer + amount);
    }

    /* We've done all the whole blocks we can, see if there's any tail data
     * left.
     */

    if (numBytes)
    {
        result = FillBlockBuffer(file, BlockNum(file), TRUE);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }

        file->rf_BytePosition += numBytes;
        file->rf_BufferDirty   = TRUE;
        memcpy(file->rf_BlockBuffer,buffer,numBytes);
    }

    if (file->rf_BytePosition >= file->rf_BytesInFile)
        file->rf_BytesInFile = file->rf_BytePosition;

    return file->rf_BytePosition - file->rf_OriginalPosition;
}


/*****************************************************************************/


int32 SeekRawFile(RawFile *file, int32 position, FileSeekModes mode)
{
uint32 oldPos;
uint32 pos;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    if (file->rf_Error < 0)
        file->rf_Error = 0;

    oldPos = file->rf_BytePosition;
    switch (mode)
    {
        case FILESEEK_START  : if (position <= 0)
                               {
                                   pos = 0;
                               }
                               else
                               {
                                   pos = position;

                                   if (pos > file->rf_BytesInFile)
                                       pos = file->rf_BytesInFile;
                               }
                               file->rf_BytePosition = pos;
                               break;

        case FILESEEK_CURRENT: if (position < 0)
                               {
                                   pos = -position;
                                   if (pos >= file->rf_BytePosition)
                                       file->rf_BytePosition = 0;
                                   else
                                       file->rf_BytePosition -= pos;
                               }
                               else
                               {
                                   pos = position;
                                   if (pos + file->rf_BytePosition >= file->rf_BytesInFile)
                                   {
                                       file->rf_BytePosition = file->rf_BytesInFile;
                                   }
                                   else
                                   {
                                       file->rf_BytePosition += pos;

                                       if (file->rf_BytePosition < oldPos)
                                       {
                                           /* overflow! */
                                           file->rf_BytePosition = file->rf_BytesInFile;
                                       }
                                   }
                               }
                               break;

        case FILESEEK_END    : if (position < 0)
                               {
                                   pos = -position;
                                   if (pos >= file->rf_BytesInFile)
                                       file->rf_BytePosition = 0;
                                   else
                                       file->rf_BytePosition = file->rf_BytesInFile - pos;
                               }
                               else
                               {
                                   file->rf_BytePosition = file->rf_BytesInFile;
                               }
                               break;

        default              : return FILE_ERR_BADMODE;
    }

    return oldPos;
}


/*****************************************************************************/


Err SetRawFileSize(RawFile *file, uint32 newSize)
{
IOInfo ioInfo;
uint32 numBlocks;
Err    result;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    if (!file->rf_Writable)
        return FILE_ERR_BADMODE;

    if (file->rf_Error < 0)
        return file->rf_Error;

    file->rf_OriginalPosition = file->rf_BytePosition;

    numBlocks = ((newSize + file->rf_BlockSize - 1) / file->rf_BlockSize);
    if (numBlocks != file->rf_BlocksInFile)
    {
        memset(&ioInfo,0,sizeof(ioInfo));
        ioInfo.ioi_Command = FILECMD_ALLOCBLOCKS;

        if (numBlocks > file->rf_BlocksInFile)
            ioInfo.ioi_Offset = numBlocks - file->rf_BlocksInFile;
        else
            ioInfo.ioi_Offset = -(file->rf_BlocksInFile - numBlocks);

        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
        {
            RecordError(file, result);
            return result;
        }
    }

    file->rf_BytesInFile  = newSize;
    file->rf_BlocksInFile = numBlocks;

    if (file->rf_BytePosition > file->rf_BytesInFile)
        file->rf_BytePosition = file->rf_BytesInFile;

    if (file->rf_BlockInBuffer >= file->rf_BlocksInFile)
    {
        file->rf_BufferValid = FALSE;
        file->rf_BufferDirty = FALSE;
    }

    return 0;
}


/*****************************************************************************/


Err GetRawFileInfo(const RawFile *file, FileInfo *info, uint32 infoSize)
{
FileInfo fi;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    fi.fi_File         = file->rf_File;
    fi.fi_ByteCount    = file->rf_BytesInFile;
    fi.fi_BlockCount   = file->rf_BlocksInFile;
    fi.fi_BlockSize    = file->rf_BlockSize;
    fi.fi_FileType     = file->rf_FileType;
    fi.fi_Version      = file->rf_Version;
    fi.fi_Revision     = file->rf_Revision;
    fi.fi_BytePosition = file->rf_BytePosition;
    fi.fi_Error        = file->rf_Error;
    fi.fi_Date         = file->rf_Date;

    if (infoSize > sizeof(FileInfo))
    {
        memset(info,0,sizeof(infoSize));
        infoSize = sizeof(FileInfo);
    }

    memcpy(info, &fi, infoSize);
    return 0;
}


/*****************************************************************************/


Err SetRawFileAttrs(RawFile *file, const TagArg *tags)
{
TagArg *tag;
bool    setType;
bool    setVersion;
bool    setBlockSize;
bool    setDate;
IOInfo  ioInfo;
Err     result;
uint32  newBlockSize;

    if (!IsRawFile(file))
        return FILE_ERR_BADFILE;

    if (!file->rf_Writable)
        return FILE_ERR_BADMODE;

    if (file->rf_Error < 0)
        return file->rf_Error;

    setType      = FALSE;
    setVersion   = FALSE;
    setDate      = FALSE;
    setBlockSize = FALSE;
    newBlockSize = file->rf_BlockSize;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case FILEATTRS_TAG_FILETYPE : file->rf_FileType = (uint32)tag->ta_Arg;
                                          setType = TRUE;
                                          break;

            case FILEATTRS_TAG_VERSION  : file->rf_Version = (uint8)tag->ta_Arg;
                                          setVersion = TRUE;
                                          break;

            case FILEATTRS_TAG_REVISION : file->rf_Revision = (uint8)tag->ta_Arg;
                                          setVersion = TRUE;
                                          break;

            case FILEATTRS_TAG_BLOCKSIZE: if (file->rf_BytesInFile)
                                          {
                                              /* can only do this before writing
                                               * data to the file.
                                               */
                                              return FILE_ERR_BADMODE;
                                          }
                                          newBlockSize = (uint32)tag->ta_Arg;
                                          setBlockSize = TRUE;
                                          break;

            case FILEATTRS_TAG_DATE     : file->rf_Date = *(TimeVal *)tag->ta_Arg;
                                          setDate = TRUE;
                                          break;

            default                     : return FILE_ERR_BADTAG;
        }
    }

    memset(&ioInfo,0,sizeof(ioInfo));

    result = 0;

    if (setType)
    {
        ioInfo.ioi_Command = FILECMD_SETTYPE;
        ioInfo.ioi_Offset  = file->rf_FileType;
        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
            return result;
    }

    if (setVersion)
    {
        ioInfo.ioi_Command = FILECMD_SETVERSION;
        ioInfo.ioi_Offset  = (file->rf_Version << 8) | file->rf_Revision;
        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
            return result;
    }

    if (setBlockSize)
    {
        ioInfo.ioi_Command = FILECMD_SETBLOCKSIZE;
        ioInfo.ioi_Offset  = newBlockSize;
        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
            return result;

        file->rf_BlockSize = newBlockSize;
    }

    if (setDate)
    {
        ioInfo.ioi_Command         = FILECMD_SETDATE;
        ioInfo.ioi_Send.iob_Buffer = &file->rf_Date;
        ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);
        result = DoIO(file->rf_IO,&ioInfo);
        if (result < 0)
            return result;
    }

    return result;
}


/*****************************************************************************/


Err SetFileAttrs(const char *name, const TagArg *tags)
{
Err      result;
RawFile *file;

    result = OpenRawFile(&file,name,FILEOPEN_WRITE);
    if (result >= 0)
    {
        result = SetRawFileAttrs(file,tags);
        CloseRawFile(file);
    }

    return result;
}
