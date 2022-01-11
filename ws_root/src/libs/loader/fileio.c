/* @(#) fileio.c 96/11/06 1.29 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <loader/loader3do.h>
#include <misc/compression.h>
#include <string.h>
#include <stdio.h>
#include "fileio.h"


/*****************************************************************************/


#define TRACE(x) /*printf x*/


/*****************************************************************************/


Err OpenLoaderFile(LoaderFile *file, const char *name)
{
Err              result;
OSComponentNode *cn;

    TRACE(("OpenLoaderFile: name %s\n", name));

    memset(file, 0, sizeof(LoaderFile));

    file->lf_RSA = TRUE;

    if (RSADigestInit(&file->lf_RSAContext))
    {
        cn = NULL;
        if (KB_FIELD(kb_OSComponents))
            cn = (OSComponentNode *)FindNamedNode(KB_FIELD(kb_OSComponents), name);

        if (cn)
        {
            result = 0;
            file->lf_OSComponent = cn;
            file->lf_RSA         = FALSE;
        }
        else
        {
            result = OpenRawFile(&file->lf_RawFile, name, FILEOPEN_READ);
        }
    }
    else
    {
        result = LOADER_ERR_RSA;
    }

    return result;
}


/*****************************************************************************/


void CloseLoaderFile(LoaderFile *file)
{
    TRACE(("CloseLoaderFIle: file 0x%x\n", file));

    if (file->lf_RawFile)
        CloseRawFile(file->lf_RawFile);
}


/*****************************************************************************/


Err ReadLoaderFile(LoaderFile *file, void *buffer, uint32 numBytes)
{
Err result;

    TRACE(("ReadLoaderFile: file 0x%x, buf 0x%x, numBytes %u\n", file, buffer, numBytes));

    if (file->lf_OSComponent)
    {
        if (numBytes > file->lf_OSComponent->cn_Size - file->lf_Position)
            result = file->lf_OSComponent->cn_Size - file->lf_Position;
        else
            result = numBytes;

        memcpy(buffer, &((uint8 *)file->lf_OSComponent->cn_Addr)[file->lf_Position], result);
    }
    else
    {
        result = ReadRawFile(file->lf_RawFile, buffer, numBytes);
    }

    if (result < 0)
        return result;

    if (result != numBytes)
        return LOADER_ERR_BADFILE;

    if (file->lf_RSA)
    {
        if (RSADigestUpdate(&file->lf_RSAContext, buffer, numBytes) == 0)
            return LOADER_ERR_BADPRIV;
    }

    file->lf_Position += numBytes;

    return numBytes;
}


/*****************************************************************************/


Err ReadLoaderFileCompressed(LoaderFile *file, void *buffer,
                             uint32 numCompBytes, uint32 numDecompBytes)
{
void *compBuffer;
Err   result;

    TRACE(("ReadLoaderFileCompressed: file 0x%x, buf 0x%x, numCompBytes %u, numDecompBytes %u\n", file, buffer, numCompBytes, numDecompBytes));

    compBuffer = AllocMem(numCompBytes, MEMTYPE_NORMAL);
    if (compBuffer == NULL)
        return LOADER_ERR_NOMEM;

    result = ReadLoaderFile(file, compBuffer, numCompBytes);
    if (result >= 0)
    {
        result = SimpleDecompress(compBuffer, numCompBytes / 4,
                                  buffer, numDecompBytes / 4);
        if (result >= 0)
            result *= 4;
    }
    FreeMem(compBuffer, numCompBytes);

    return result;
}


/*****************************************************************************/


Err SeekLoaderFile(LoaderFile *file, uint32 newPos)
{
uint32 delta;
Err    result;

    TRACE(("SeekLoaderFile: file 0x%x, currentPos %u, newPos %u\n", file, file->lf_Position, newPos));

    if (newPos == file->lf_Position)
        return 0;

#ifdef BUILD_PARANOIA
    if (newPos < file->lf_Position)
    {
        printf("ERROR: An improperly organized executable file has resulted\n");
        printf("       in an illegal attempt to seek backwards by %u bytes\n", file->lf_Position - newPos);
        printf("       within the loader.\n");
        return LOADER_ERR_BADFILE;
    }
#endif

    if (file->lf_RSA)
    {
        delta = newPos - file->lf_Position;
        while (delta >= sizeof(file->lf_RSABuffer))
        {
            result = ReadLoaderFile(file, file->lf_RSABuffer, sizeof(file->lf_RSABuffer));
            if (result < 0)
                return result;

            delta -= sizeof(file->lf_RSABuffer);
        }
        result = ReadLoaderFile(file, file->lf_RSABuffer, delta);
    }
    else
    {
        if (file->lf_OSComponent == NULL)
            result = SeekRawFile(file->lf_RawFile, newPos, FILESEEK_START);
    }

    file->lf_Position = newPos;

    return result;
}


/*****************************************************************************/


void StopRSALoaderFile(LoaderFile *file)
{
    TRACE(("StopRSALoaderFile: file 0x%x\n", file));
    file->lf_RSA = FALSE;
}


/*****************************************************************************/


Err CheckRSALoaderFile(LoaderFile *file)
{
FileInfo info;
Err      result;
uint8    sig[RSA_KEY_SIZE];

    TRACE(("CheckRSALoaderFIle: file 0x%x\n", file));

    result = GetRawFileInfo(file->lf_RawFile, &info, sizeof(info));
    if (result < 0)
        return result;

    result = SeekLoaderFile(file, info.fi_ByteCount - RSA_KEY_SIZE);
    if (result < 0)
        return result;

    result = ReadRawFile(file->lf_RawFile, sig, RSA_KEY_SIZE);

    if (result < 0)
        return result;

    if (result != RSA_KEY_SIZE)
        return LOADER_ERR_BADPRIV;

    if (RSADigestFinal(&file->lf_RSAContext, sig, RSA_KEY_SIZE) == 0)
        result = LOADER_ERR_BADPRIV;
    else
        result = 0;

    return result;
}


/*****************************************************************************/


Err IsLoaderFileOnBlessedFS(const LoaderFile *file)
{
FileInfo    info;
OFile      *ofp;
FileSystem *fs;
Err         result;

    TRACE(("IsLoaderFileOnBlessedFS: file 0x%x\n", file));

    result = GetRawFileInfo(file->lf_RawFile, &info, sizeof(info));
    if (result < 0)
        return result;

    /* FIXME: innappropriate internal FS knowledge */
    ofp = (OFile *) LookupItem(info.fi_File);
    fs  = ofp->ofi_File->fi_FileSystem;

    if (fs->fs_VolumeFlags & VF_BLESSED)
        return 1;

    return 0;
}


/*****************************************************************************/


Err GetLoaderFilePath(const LoaderFile *file, char *path, uint32 pathSize)
{
FileInfo info;
Err      result;
IOInfo   ioInfo;
Item     ior;

    TRACE(("GetLoaderFilePath: file 0x%x\n", file));

    path[0] = 0;
    if (file->lf_OSComponent)
        return 0;

    result = GetRawFileInfo(file->lf_RawFile, &info, sizeof(info));
    if (result < 0)
        return result;

    ior = CreateIOReq(NULL, 0, info.fi_File, 0);
    if (ior >= 0)
    {
        /* get file path */
        memset(&ioInfo, 0, sizeof(IOInfo));
        ioInfo.ioi_Command         = FILECMD_GETPATH;
        ioInfo.ioi_Recv.iob_Buffer = path;
        ioInfo.ioi_Recv.iob_Len    = pathSize;
        result = DoIO(ior, &ioInfo);
        DeleteIOReq(ior);
    }

    return result;
}


/*****************************************************************************/


Item OpenLoaderFileParent(const LoaderFile *file)
{
Err    result;
uint32 len;
char   path[FILESYSTEM_MAX_PATH_LEN];

    TRACE(("OpenLoaderFileParent: file 0x%x\n", file));

    result = GetLoaderFilePath(file, path, sizeof(path));
    if (result >= 0)
    {
        len = strlen(path);
        while (len > 0)
        {
            if (path[len] == '/')
                break;

            len--;
        }
        path[len] = 0;

        result = OpenFile(path);
    }

    return result;
}
