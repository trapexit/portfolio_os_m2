/* @(#) fileio.h 96/11/06 1.6 */

#ifndef __FILEIO_H
#define __FILEIO_H


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __DIPIR_DIPIRPUB_H
#include <dipir/dipirpub.h>
#endif


/*****************************************************************************/


typedef struct
{
    RawFile           *lf_RawFile;
    OSComponentNode   *lf_OSComponent;

    uint32             lf_Position;
    DipirDigestContext lf_RSAContext;
    uint8              lf_RSABuffer[256];
    bool               lf_RSA;
} LoaderFile;


/*****************************************************************************/


Err  OpenLoaderFile(LoaderFile *file, const char *name);
void CloseLoaderFile(LoaderFile *file);
Err  ReadLoaderFile(LoaderFile *file, void *buffer, uint32 numBytes);
Err  ReadLoaderFileCompressed(LoaderFile *file, void *buffer, uint32 numCompBytes, uint32 numDecompBytes);
Err  SeekLoaderFile(LoaderFile *file, uint32 newPos);
Err  CheckRSALoaderFile(LoaderFile *file);
void StopRSALoaderFile(LoaderFile *file);
Err  IsLoaderFileOnBlessedFS(const LoaderFile *file);
Item OpenLoaderFileParent(const LoaderFile *file);
Err  GetLoaderFilePath(const LoaderFile *file, char *path, uint32 pathSize);


/*****************************************************************************/


#endif /* __FILEIO_H */
