#ifndef __FILE_DIRECTORYFUNCTIONS_H
#define __FILE_DIRECTORYFUNCTIONS_H


/******************************************************************************
**
**  @(#) directoryfunctions.h 96/02/20 1.9
**
**  Function prototypes for the userland interface to the directory walker.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __FILE_DIRECTORY_H
#include <file/directory.h>
#endif


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */


Directory *OpenDirectoryItem(Item openFileItem);
Directory *OpenDirectoryPath(const char *path);
Err ReadDirectory(Directory *dir, DirectoryEntry *de);
void CloseDirectory(Directory *dir);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __FILE_DIRECTORYFUNCTIONS_H */
