#ifndef __FILE_FILEFUNCTIONS_H
#define __FILE_FILEFUNCTIONS_H


/******************************************************************************
**
**  @(#) filefunctions.h 96/09/06 1.35
**
**  Function prototypes for primary file folio functions.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */


extern Err  CreateFile(const char *path);
extern Err  CreateFileInDir(Item dirItem, const char *path);
extern Err  DeleteFile(const char *path);
extern Err  DeleteFileInDir(Item dirItem, const char *path);
extern Item OpenFile(const char *path);
extern Item OpenFileInDir(Item dirItem, const char *path);
extern Err  CloseFile(Item fileItem);
extern Err  SetFileAttrs(const char *path, const TagArg *tags);
extern Err  SetFileAttrsVA(const char *path, uint32 tag, ...);

extern Item FindFileAndOpen(const char *partialPath, const TagArg *tags);
extern Item FindFileAndOpenVA(const char *partialPath, uint32 tag, ...);
extern Err  FindFileAndIdentify(char *pathBuf, int32 pathBufLen,
                                const char *partialPath, const TagArg *tags);
extern Err  FindFileAndIdentifyVA(char *pathBuf, int32 pathBufLen,
                                  const char *partialPath, uint32 tag, ...);

extern Err  Rename(const char *path, const char *newName);
extern Err  CreateDirectory(const char *path);
extern Err  CreateDirectoryInDir(Item dirItem, const char *path);
extern Err  DeleteDirectory(const char *path);
extern Err  DeleteDirectoryInDir(Item dirItem, const char *path);
extern Item ChangeDirectory(const char *path);
extern Item ChangeDirectoryInDir(Item dirItem, const char *path);
extern Item GetDirectory(char *pathBuf, int32 pathBufLen);

extern void MinimizeFileSystem(uint32 minimizationFlags);
extern Item FileSystemMountedOn(const DeviceStack *ds);

extern Err  FormatFileSystem(Item deviceItem, const char *name, const TagArg *tags);
extern Err  FormatFileSystemVA(Item deviceItem, const char *name, uint32 tag, ...);
extern Err  GetUnmountedList(List **lp);
extern Err  DeleteUnmountedList(List *l);

#ifndef EXTERNAL_RELEASE
extern Item MountFileSystem(Item deviceItem, uint32 blockOffset);
extern Err  DismountFileSystem(const char *name);
extern Err  MountAllFileSystems(void);
extern Err  SuperFindFileAndIdentify(char *pathBuf, int32 pathBufLen,
				     const char *partialPath,
				     const TagArg *tags);

extern int32 SetMountLevel(int32 newLevel);
extern int32 GetMountLevel(void);
extern Item CreateAlias(const char *aliasPath, char *realPath);
extern void PlugNPlay(void);
extern Err  RecheckAllFileSystems(void);
#endif

/* for source compatibility only, do not use in new code */
#define OpenDiskFile(path)               OpenFile(path)
#define OpenDiskFileInDir(dirItem,path)  OpenFileInDir(dirItem,path)
#define CloseDiskFile(fileItem)          CloseFile(fileItem)

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __FILE_FILEFUNCTIONS_H */
