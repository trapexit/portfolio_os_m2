/* @(#) FileDriverInit.c 96/08/30 1.4 */

/*
  FileDriver.c - implements the driver which handles OFile "devices"
  and OptimizedDisk "devices".

  The driver receives IORequests issued against OFile "devices".  It
  queues these requests against the OptimizedDisk device associated with
  the actual (raw, physical) device on which the filesystem resides.
*/

#define SUPER

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/directory.h>
#include <file/discdata.h>
#include <stdlib.h>
#include <string.h>

#undef DEBUG
#undef DEBUG2

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

extern Driver *fileDriver;

extern char *fsCacheBase;
extern int32 fsCacheBusy;

int32  FileDriverDoit(IOReq *theRequest);
int32  FileDriverStatus(IOReq *theRequest);
int32  FileDriverGetPath(IOReq *theRequest);

extern int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen);
extern FileFolioTaskData *SetupFileFolioTaskData(void);

extern void InitOpera(List *fsTypes);
extern void InitAcrobat(List *fsTypes);

#ifdef BUILD_DEBUGGER
extern void InitHost(List *fsTypes);
#endif


int32 FileDriverInit(struct Driver *me)
{
  Item i;
  File *root;
  DBUG(("File driver init, driver at %x!\n", me));
  InitOpera(&fileFolio.ff_FileSystemTypes);
  InitAcrobat(&fileFolio.ff_FileSystemTypes);

#ifdef BUILD_DEBUGGER
  InitHost(&fileFolio.ff_FileSystemTypes);
#endif
  i = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
			   (void *) NULL, sizeof (File));
  if (i < 0) {
    DBUG(("Couldn't create root file (%d)\n", i));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  DBUG(("Root-of-the-world file created as item %d\n", i));
  root = (File *) LookupItem(i);
  fileFolio.ff_Root = root;
  root->fi_FileName[0] = '\0';
  root->fi_FileSystem = (FileSystem *) NULL;
  root->fi_ParentDirectory = root;
  root->fi_UniqueIdentifier = 0;
  root->fi_Type = FILE_TYPE_DIRECTORY;
  root->fi_Flags = FILE_IS_DIRECTORY | FILE_IS_READONLY |
                   FILE_IS_FOR_FILESYSTEM | FILE_SUPPORTS_DIRSCAN;
  root->fi_UseCount = 1;
  root->fi_BlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  root->fi_ByteCount = 0;
  root->fi_BlockCount = 0;
  root->fi_LastAvatarIndex = -1;
  root->fi.n_Name = root->fi_FileName;
#ifdef	FS_DIRSEMA
  InitDirSema(root, 0);
#endif	/* FS_DIRSEMA */
  DBUG(("Root-of-the-world file initialized\n"));
  AddTail(&fileFolio.ff_Files, (Node *) root);
  DBUG(("Root-of-the-world file queued\n"));
  DBUG(("File driver initialization complete\n"));
  return me->drv.n_Item;
}


