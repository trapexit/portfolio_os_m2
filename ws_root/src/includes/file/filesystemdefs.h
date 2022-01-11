#ifndef __FILE_FILESYSTEMDEFS_H
#define __FILE_FILESYSTEMDEFS_H


/******************************************************************************
**
**  @(#) filesystemdefs.h 96/07/17 1.8
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_DRIVER_H
#include <kernel/driver.h>
#endif

#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif


/*****************************************************************************/


/* Data definitions for the filesystem and I/O driver kit */
extern FileFolio fileFolio;
extern Driver *fileDriver;

#ifdef BUILD_STRINGS
#define ERR(x) PrintfSysErr(x)
#else
#define ERR(x) TOUCH(x)
#endif


/*****************************************************************************/


#endif /* __FILE_FILESYSTEMDEFS_H */
