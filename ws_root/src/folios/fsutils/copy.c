/* @(#) copy.c 96/10/28 1.18 */

/* NOTE: The asynchronous operation mode is not currently implemented. All
 *       accesses to the active file list are done with semaphore locking,
 *       which is an important part of supporting one thread per state
 *       machine. The thing missing is the signalling at opportune moments
 *       from one state machine to the other to inform that progress has been
 *       made.
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/tags.h>
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/fsutils.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#endif


/*****************************************************************************/


/* we don't need these for now since we don't support asynchronous reads/writes */
#define LOCK(co)   /* LockSemaphore(co->co_Lock, SEM_WAIT) */
#define UNLOCK(co) /* UnlockSemaphore(co->co_Lock) */


/*****************************************************************************/


/* This copy engine is implemented by using two state machines that are
 * loosely coupled and activated in sequence.
 *
 * The first machine is the file reader. It accumulates files and directories
 * in memory until the system runs out of memory, or a predefined threshold
 * is attained.
 *
 * When the reader returns, the writer machine is invoked. It drains the
 * list of files and directories and outputs them to the destination media.
 *
 * The reason for the state machines is that it allows the operations to be
 * easily interrupted and restarted at any time. In particular, the reader
 * may decide at any time that it has consumed enough resources and exit.
 * To continue the read operation later on, we must reenter the read loop
 * at the same place as we left it.
 *
 * During a copy operation, callbacks get invoked whenever user-interface
 * support is needed. Whenever a callback returns a value other than 0, it
 * is returned immediately to the caller of the copy function. A return of
 * 0 means "situation addressed, please continue/retry".
 */


/*****************************************************************************/


typedef enum
{
    ET_FILE,
    ET_DIRECTORY
} EntryTypes;

typedef struct
{
    MinNode db;
    uint32  db_BufferSize;
    uint32  db_NumBytes;

    /* data follows in memory */
} DataBuffer;

typedef struct DirNode
{
    MinNode         dn;
    uint8           dn_EntryType;
    bool            dn_Complete;
    char            dn_Name[FILESYSTEM_MAX_NAME_LEN+1];
    struct DirNode *dn_Parent;

    List            dn_Entries;
    Directory      *dn_Directory;
} DirNode;

typedef struct
{
    MinNode    fn;
    uint8      fn_EntryType;
    bool       fn_Complete;
    char       fn_Name[FILESYSTEM_MAX_NAME_LEN+1];
    DirNode   *fn_Parent;

    uint32     fn_FileSize;
    uint32     fn_FileType;
    uint8      fn_Version;
    uint8      fn_Revision;
    List       fn_Data;              /* list of DataBuffer structures */
} FileNode;

typedef enum
{
    STATE_INIT,
    STATE_ENTERDIR,
    STATE_EXITDIR,
    STATE_SCANDIR,
    STATE_OPENFILE,
    STATE_SETFILESIZE,
    STATE_PROCESSFILE,
    STATE_CLOSEFILE,
    STATE_DONE
} States;

/* info associated with the reader state machine */
typedef struct
{
    States          rc_State;
    DirectoryEntry  rc_DirectoryEntry;
    RawFile        *rc_RawFile;
    DirNode        *rc_CurrentDir;
    FileNode       *rc_CurrentFile;
    uint32          rc_CurrentBufferSize;
    char            rc_Directory[FILESYSTEM_MAX_PATH_LEN+1];
    char            rc_WorkPath[FILESYSTEM_MAX_PATH_LEN+1];
	uint32			rc_DirCount;
	uint32			rc_FileCount;
	uint32			rc_FileBytes;
} ReaderContext;

/* info associated with the writer state machine */
typedef struct
{
    States     wc_State;
    RawFile   *wc_RawFile;
    DirNode   *wc_CurrentDir;
    FileNode  *wc_CurrentFile;
    char       wc_Directory[FILESYSTEM_MAX_PATH_LEN+1];
    char       wc_WorkPath[FILESYSTEM_MAX_PATH_LEN+1];
} WriterContext;

struct CopyObj
{
    CopyProgressFunc             co_Progress;
    CopyNeedSourceFunc           co_NeedSource;
    CopyNeedDestinationFunc      co_NeedDestination;
    CopyDuplicateFunc            co_Duplicate;
    CopyDestinationFullFunc      co_DestinationFull;
    CopyDestinationProtectedFunc co_DestinationProtected;
    CopyReadErrorFunc            co_ReadError;
    CopyWriteErrorFunc           co_WriteError;
    void                        *co_UserData;
    uint32                       co_MemoryThreshold;
    uint32                       co_SystemFreeMemory;
    bool                         co_OutOfMemory;

    uint32                       co_MemoryUsed;
    ReaderContext                co_ReaderContext;
    WriterContext                co_WriterContext;
    Item                         co_Lock;
    DirNode                      co_RootDir;
};


/*****************************************************************************/


static void stccpy(char *to, const char *from, uint32 maxChars)
{
uint32 i;

    if (maxChars == 0)
        return;

    i = 0;
    while (from[i] && (i < (maxChars - 1)))
        to[i] = from[i++];

    to[i] = 0;
}


/*****************************************************************************/


static void MakePath(CopyObj *co, bool readMode, void *entry)
{
FileNode *fn;
char     *rootPath;
char     *result;
uint32    len;
uint32    nameLen;

    fn = (FileNode *)entry;

    if (readMode)
    {
        rootPath = co->co_ReaderContext.rc_Directory;
        result   = co->co_ReaderContext.rc_WorkPath;
    }
    else
    {
        rootPath = co->co_WriterContext.wc_Directory;
        result   = co->co_WriterContext.wc_WorkPath;
    }

    len = strlen(rootPath);
    while (fn->fn_Parent)
    {
        len += strlen(fn->fn_Name) + 1;
        fn = (FileNode *)fn->fn_Parent;
    }

    result[len] = 0;
    fn = (FileNode *)entry;
    while (fn->fn_Parent)
    {
        nameLen = strlen(fn->fn_Name) + 1;
        len    -= nameLen;

        strncpy(&result[len + 1], fn->fn_Name, nameLen - 1);
        result[len] = '/';

        fn = (FileNode *)fn->fn_Parent;
    }
    strncpy(result, rootPath, strlen(rootPath));
}


/*****************************************************************************/


static void *GetMem(CopyObj *co, uint32 numBytes)
{
void    *result;
MemInfo  mi;
uint32   pageSize;

    LOCK(co);

    result = NULL;
    if (numBytes + co->co_MemoryUsed < co->co_MemoryThreshold)
    {
        if (numBytes + co->co_MemoryUsed + 16384 >= co->co_SystemFreeMemory)
        {
            GetMemInfo(&mi, sizeof(mi), MEMTYPE_NORMAL);
            pageSize = GetPageSize(MEMTYPE_NORMAL);
            co->co_SystemFreeMemory = pageSize * mi.minfo_FreePages;
        }

        if (numBytes + co->co_MemoryUsed + 16384 < co->co_SystemFreeMemory)
        {
            result = AllocMem(numBytes, MEMTYPE_NORMAL);
            if (result)
                co->co_MemoryUsed += numBytes;
        }
    }

    if (result == NULL)
        co->co_OutOfMemory = TRUE;

    UNLOCK(co);

    return result;
}


/*****************************************************************************/


static void ReturnMem(CopyObj *co, void *p, uint32 numBytes)
{
    if (p)
    {
        FreeMem(p, numBytes);

        LOCK(co);
        co->co_MemoryUsed -= numBytes;
        co->co_OutOfMemory = FALSE;
        UNLOCK(co);
    }
}


/*****************************************************************************/


static void InitReaderMachine(CopyObj *co, const char *sourceDir,
                              const char *objName)
{
ReaderContext *rc;

    rc                = &co->co_ReaderContext;
    rc->rc_State      = STATE_INIT;
    rc->rc_CurrentDir = &co->co_RootDir;
    stccpy(rc->rc_Directory, sourceDir, sizeof(rc->rc_Directory));
    stccpy(rc->rc_DirectoryEntry.de_FileName, objName, sizeof(rc->rc_DirectoryEntry.de_FileName));
    stccpy(rc->rc_WorkPath, sourceDir, sizeof(rc->rc_WorkPath));
    AppendPath(rc->rc_WorkPath, objName, sizeof(rc->rc_WorkPath));
}


/*****************************************************************************/


static Err ReaderMachine(CopyObj *co)
{
ReaderContext *rc;
DirNode       *dn;
FileNode      *fn;
DataBuffer    *db;
bool           exit;
States         state;
Err            result;
FileInfo       info;

    rc = &co->co_ReaderContext;

    state = rc->rc_State;
    exit  = FALSE;
    do
    {
        DBUG(("ReaderMachine: state %d\n",state));

        result = (* co->co_Progress)(co->co_UserData, rc->rc_WorkPath);
        if (result)
            break;

        switch (state)
        {
            case STATE_INIT       : DBUG(("ReaderMachine: opening file %s\n",rc->rc_WorkPath));

                                    result = OpenRawFile(&rc->rc_RawFile, rc->rc_WorkPath, FILEOPEN_READ);
                                    if (result >= 0)
                                    {
                                        if (rc->rc_RawFile)
                                            state = STATE_OPENFILE;
                                        else
                                            state = STATE_ENTERDIR;
                                    }
                                    else if (result == FILE_ERR_NOTAFILE)
                                    {
                                        if (rc->rc_RawFile)
                                            state = STATE_OPENFILE;
                                        else
                                            state = STATE_ENTERDIR;

                                        result = 0;
                                    }
                                    else
                                    {
                                        exit = TRUE;
                                    }
                                    break;

            case STATE_ENTERDIR   : dn = GetMem(co, sizeof(DirNode));
                                    if (dn)
                                    {
                                        dn->dn_EntryType = ET_DIRECTORY;
                                        dn->dn_Complete  = FALSE;
                                        strcpy(dn->dn_Name, rc->rc_DirectoryEntry.de_FileName);
                                        dn->dn_Parent = rc->rc_CurrentDir;
                                        PrepList(&dn->dn_Entries);

                                        MakePath(co, TRUE, dn);

                                        DBUG(("ReaderMachine: opening directory %s\n",rc->rc_WorkPath));

                                        dn->dn_Directory = OpenDirectoryPath(rc->rc_WorkPath);
                                        if (dn->dn_Directory)
                                        {
                                            LOCK(co);
                                            AddTail(&rc->rc_CurrentDir->dn_Entries, (Node *)dn);
                                            UNLOCK(co);

                                            rc->rc_CurrentDir = dn;
											rc->rc_DirCount++;
                                            state = STATE_SCANDIR;
                                            break;
                                        }
                                        else
                                        {
                                            result = -1;
                                        }
                                        ReturnMem(co, dn, sizeof(DirNode));
                                    }
                                    else
                                    {
                                        result = FSUTILS_ERR_NOMEM;
                                    }
                                    exit = TRUE;
                                    break;

            case STATE_EXITDIR    : CloseDirectory(rc->rc_CurrentDir->dn_Directory);
                                    rc->rc_CurrentDir->dn_Complete  = TRUE;
                                    rc->rc_CurrentDir->dn_Directory = NULL;
                                    rc->rc_CurrentDir = rc->rc_CurrentDir->dn_Parent;
                                    if (rc->rc_CurrentDir)
                                    {
                                        /* resume scanning of the parent */
                                        state = STATE_SCANDIR;
                                    }
                                    else
                                    {
                                        /* can't exit the top level, means we're done! */
                                        state = STATE_DONE;
                                    }
                                    break;

            case STATE_SCANDIR    : if (rc->rc_CurrentDir->dn_Parent == NULL)
                                    {
                                        /* never scan the root */
                                        state = STATE_EXITDIR;
                                        break;
                                    }

                                    result = ReadDirectory(rc->rc_CurrentDir->dn_Directory,
                                                           &rc->rc_DirectoryEntry);
                                    if (result == FILE_ERR_NOFILE)
                                    {
                                        /* exhausted directory */
                                        state = STATE_EXITDIR;
                                    }
                                    else if (result < 0)
                                    {
                                        /* failure */
                                        exit = TRUE;
                                    }
                                    else if (rc->rc_DirectoryEntry.de_Flags & FILE_IS_DIRECTORY)
                                    {
                                        /* found a directory, process it */
                                        state = STATE_ENTERDIR;
                                    }
                                    else
                                    {
                                        /* found a file, process it */
                                        state = STATE_OPENFILE;
                                    }
                                    break;

            case STATE_OPENFILE   : fn = GetMem(co, sizeof(FileNode));
                                    if (fn)
                                    {
                                        fn->fn_EntryType = ET_FILE;
                                        fn->fn_Complete  = FALSE;
                                        fn->fn_Parent    = rc->rc_CurrentDir;
                                        strcpy(fn->fn_Name, rc->rc_DirectoryEntry.de_FileName);
                                        PrepList(&fn->fn_Data);

                                        if (!rc->rc_RawFile)
                                        {
                                            /* the first time through, it's
                                             * possible that the file will have
                                             * been opened by STATE_INIT
                                             */

                                            MakePath(co, TRUE, fn);

                                            DBUG(("ReaderMachine: opening file %s\n",rc->rc_WorkPath));

                                            result = OpenRawFile(&rc->rc_RawFile, rc->rc_WorkPath, FILEOPEN_READ);
                                        }

                                        if (result >= 0)
                                        {
                                            result = GetRawFileInfo(rc->rc_RawFile, &info, sizeof(info));
                                            if (result >= 0)
                                            {
                                                fn->fn_FileSize = info.fi_ByteCount;
                                                fn->fn_FileType = info.fi_FileType;
                                                fn->fn_Version  = info.fi_Version;
                                                fn->fn_Revision = info.fi_Revision;

                                                /* to reduce I/O overhead, the minimal
                                                 * buffer size is arbitrarily set to
                                                 * 1024 bytes.
                                                 */
                                                if (info.fi_BlockSize < 1024)
                                                    rc->rc_CurrentBufferSize = 1024;
                                                else
                                                    rc->rc_CurrentBufferSize = info.fi_BlockSize;

                                                LOCK(co);
                                                AddTail(&rc->rc_CurrentDir->dn_Entries, (Node *)fn);
                                                UNLOCK(co);
                                                rc->rc_CurrentFile = fn;
												rc->rc_FileCount++;
												rc->rc_FileBytes += info.fi_ByteCount;
                                                state = STATE_PROCESSFILE;
                                                break;
                                            }
                                            CloseRawFile(rc->rc_RawFile);
                                            rc->rc_RawFile = NULL;
                                        }
                                        ReturnMem(co, fn, sizeof(FileNode));
                                    }
                                    else
                                    {
                                        result = FSUTILS_ERR_NOMEM;
                                    }
                                    exit = TRUE;
                                    break;

            case STATE_PROCESSFILE: db = GetMem(co, sizeof(DataBuffer) + rc->rc_CurrentBufferSize);
                                    if (db)
                                    {
                                        result = ReadRawFile(rc->rc_RawFile, &db[1], rc->rc_CurrentBufferSize);
                                        if (result >= 0)
                                        {
                                            db->db_BufferSize = sizeof(DataBuffer) + rc->rc_CurrentBufferSize;
                                            db->db_NumBytes   = result;
                                            LOCK(co);
                                            AddTail(&rc->rc_CurrentFile->fn_Data, (Node *)db);
                                            UNLOCK(co);

                                            if (result < rc->rc_CurrentBufferSize)
                                            {
                                                /* ran out of data, means we're done */
                                                state = STATE_CLOSEFILE;
                                            }
                                            break;
                                        }
                                        ReturnMem(co, db, sizeof(DataBuffer) + rc->rc_CurrentBufferSize);
                                    }
                                    else
                                    {
                                        result = FSUTILS_ERR_NOMEM;
                                    }
                                    exit = TRUE;
                                    break;

            case STATE_CLOSEFILE  : CloseRawFile(rc->rc_RawFile);
                                    rc->rc_RawFile                  = NULL;
                                    rc->rc_CurrentFile->fn_Complete = TRUE;
                                    rc->rc_CurrentFile              = NULL;

                                    /* done with file, resume directory scan */
                                    state = STATE_SCANDIR;
                                    break;

            case STATE_DONE       : exit = TRUE;
                                    break;
        }
    }
    while (!exit);

    DBUG(("ReaderMachine: exiting with state %d\n",state));
    DBUG(("               "));
#ifdef DEBUG
    PrintfSysErr(result);
#endif

    rc->rc_State = state;

    return result;
}


/*****************************************************************************/


static void InitWriterMachine(CopyObj *co, const char *path)
{
WriterContext *wc;

    wc                = &co->co_WriterContext;
    wc->wc_State      = STATE_INIT;
    wc->wc_CurrentDir = &co->co_RootDir;
    stccpy(wc->wc_Directory, path, sizeof(wc->wc_Directory));
    stccpy(wc->wc_WorkPath, path, sizeof(wc->wc_WorkPath));
}


/*****************************************************************************/


static Err WriterMachine(CopyObj *co)
{
WriterContext *wc;
DirNode       *dn;
FileNode      *fn;
DataBuffer    *db;
bool           exit;
States         state;
Err            result;
Item           file;

    wc = &co->co_WriterContext;

    state = wc->wc_State;
    exit  = FALSE;
    do
    {
        DBUG(("WriterMachine: state %d\n",state));

        result = (* co->co_Progress)(co->co_UserData, wc->wc_WorkPath);
        if (result)
            break;

        switch (state)
        {
            case STATE_INIT       : if (result >= 0)
                                        state = STATE_SCANDIR;
                                    else
                                        exit = TRUE;
                                    break;

            case STATE_ENTERDIR   : wc->wc_CurrentDir = (DirNode *)FirstNode(&wc->wc_CurrentDir->dn_Entries);
                                    MakePath(co, FALSE, wc->wc_CurrentDir);

                                    if (wc->wc_CurrentDir->dn_Parent->dn_Parent == NULL)
                                    {
                                        file = OpenFile(wc->wc_WorkPath);
                                        if (file >= 0)
                                        {
                                            CloseFile(file);
                                            result = (* co->co_Duplicate)(co->co_UserData, wc->wc_WorkPath);
                                            if (result)
                                            {
                                                exit = TRUE;
                                                break;
                                            }
                                        }
                                    }

                                    DBUG(("WriterMachine: creating directory %s\n",wc->wc_WorkPath));

                                    result = CreateDirectory(wc->wc_WorkPath);
                                    if (result == FILE_ERR_DUPLICATEFILE)
                                        result = 0;

                                    if (result >= 0)
                                    {
                                        /* We've created the directory,
                                         * now scan it.
                                         */
                                        state = STATE_SCANDIR;
                                        break;
                                    }
                                    exit = TRUE;
                                    break;

            case STATE_EXITDIR    : if (wc->wc_CurrentDir->dn_Parent == NULL)
                                    {
                                        /* If the directory has no parent, it
                                         * means it is the root. If we hit the
                                         * root here, it means we're actually
                                         * all done!
                                         */
                                        state = STATE_DONE;
                                    }
                                    else
                                    {
                                        dn = wc->wc_CurrentDir;
                                        LOCK(co);
                                        RemNode((Node *)dn);
                                        UNLOCK(co);
                                        wc->wc_CurrentDir = dn->dn_Parent;
                                        ReturnMem(co, dn, sizeof(DirNode));

                                        /* continue scanning this directory */
                                        state = STATE_SCANDIR;
                                    }
                                    break;

            case STATE_SCANDIR    : if (IsListEmpty(&wc->wc_CurrentDir->dn_Entries))
                                    {
                                        if (wc->wc_CurrentDir->dn_Complete)
                                        {
                                            /* current dir is done, exit from it */
                                            state = STATE_EXITDIR;
                                        }
                                        else
                                        {
                                            /* No more file entries but the
                                             * current dir is not marked as done.
                                             * This means we've caught up with
                                             * the reader state machine, so
                                             * get outta here.
                                             */
                                            exit = TRUE;
                                        }
                                    }
                                    else
                                    {
                                        fn = (FileNode *)FirstNode(&wc->wc_CurrentDir->dn_Entries);
                                        if (fn->fn_EntryType == ET_FILE)
                                        {
                                            /* next thing to process is a file */
                                            state = STATE_OPENFILE;
                                        }
                                        else
                                        {
                                            /* next thing to process is a directory */
                                            state = STATE_ENTERDIR;
                                        }
                                    }
                                    break;

            case STATE_OPENFILE   : wc->wc_CurrentFile = (FileNode *)FirstNode(&wc->wc_CurrentDir->dn_Entries);
                                    MakePath(co, FALSE, wc->wc_CurrentFile);
                                    if (wc->wc_CurrentDir->dn_Parent == NULL)
                                    {
                                        file = OpenFile(wc->wc_WorkPath);
                                        if (file >= 0)
                                        {
                                            CloseFile(file);
                                            result = (* co->co_Duplicate)(co->co_UserData, wc->wc_WorkPath);
                                            if (result)
                                            {
                                                exit = TRUE;
                                                break;
                                            }
                                        }
                                    }

                                    DBUG(("WriterMachine: creating file %s\n",wc->wc_WorkPath));

                                    result = OpenRawFile(&wc->wc_RawFile, wc->wc_WorkPath, FILEOPEN_WRITE_NEW);
                                    if (result >= 0)
                                    {
                                        state = STATE_SETFILESIZE;
                                    }
                                    else
                                    {
                                        exit = TRUE;
                                    }
                                    break;

            case STATE_SETFILESIZE: result = SetRawFileSize(wc->wc_RawFile, wc->wc_CurrentFile->fn_FileSize);
                                    if (result >= 0)
                                    {
                                        state = STATE_PROCESSFILE;
                                    }
                                    else
                                    {
                                        exit = TRUE;
                                    }
                                    break;

            case STATE_PROCESSFILE: LOCK(co);
                                    db = (DataBuffer *)RemHead(&wc->wc_CurrentFile->fn_Data);
                                    UNLOCK(co);

                                    if (db)
                                    {
                                        result = WriteRawFile(wc->wc_RawFile, &db[1], db->db_NumBytes);
                                        if (result >= 0)
                                        {
                                            /* data written, nuke buffer */
                                            ReturnMem(co, db, db->db_BufferSize);
                                        }
                                        else
                                        {
                                            LOCK(co);
                                            AddHead(&wc->wc_CurrentFile->fn_Data, (Node *)db);
                                            UNLOCK(co);
                                            exit = TRUE;
                                        }
                                    }
                                    else
                                    {
                                        if (wc->wc_CurrentFile->fn_Complete)
                                        {
                                            /* file is all written out, close it */
                                            state = STATE_CLOSEFILE;
                                        }
                                        else
                                        {
                                            /* No buffers available for the current
                                             * file. This means we've caught up
                                             * with the reader, and there's nothing
                                             * left to do.
                                             */
                                            exit = TRUE;
                                        }
                                    }
                                    break;

            case STATE_CLOSEFILE  : result = CloseRawFile(wc->wc_RawFile);
                                    wc->wc_RawFile = NULL;

                                    if (result < 0)
                                    {
                                        exit = TRUE;
                                    }
                                    LOCK(co);
                                    RemNode((Node *)wc->wc_CurrentFile);
                                    UNLOCK(co);

                                    ReturnMem(co, wc->wc_CurrentFile, sizeof(FileNode));
                                    wc->wc_CurrentFile = NULL;

                                    state = STATE_SCANDIR;
                                    break;

            case STATE_DONE       : exit = TRUE;
                                    break;
        }
    }
    while (!exit);

    DBUG(("WriterMachine: exiting with state %d\n",state));
    DBUG(("               "));
#ifdef DEBUG
    PrintfSysErr(result);
#endif

    wc->wc_State = state;

    return result;
}


/*****************************************************************************/


static Err DefaultCB_Progress(void *userData, const char *path)
{
    TOUCH(userData);
    TOUCH(path);
    return 0;
}

static Err DefaultCB_NeedSource(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_OFFLINE;
}

static Err DefaultCB_NeedDestination(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_OFFLINE;
}

static Err DefaultCB_Duplicate(void *userData, const char *path)
{
    TOUCH(userData);
    TOUCH(path);
    return 0;
}

static Err DefaultCB_DestinationFull(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_NOSPACE;
}

static Err DefaultCB_DestinationProtected(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_READONLY;
}

static Err DefaultCB_ReadError(void *userData, const char *path, Err error)
{
    TOUCH(userData);
    TOUCH(path);
    return error;
}

static Err DefaultCB_WriteError(void *userData, const char *path, Err error)
{
    TOUCH(userData);
    TOUCH(path);
    return error;
}


/*****************************************************************************/


static Err DoWrite(CopyObj *co)
{
Err   result;
int32 i;
char  fsName[FILESYSTEM_MAX_NAME_LEN+1];

    result = WriterMachine(co);
    if (result < 0)
    {
        i = -1;
        do
        {
            i++;
            fsName[i] = co->co_WriterContext.wc_WorkPath[i+1];
        }
        while (fsName[i] && (fsName[i] != '/'));
        fsName[i] = 0;

        if (result == FILE_ERR_READONLY)
        {
            result = (* co->co_DestinationProtected)(co->co_UserData, fsName);
        }
        else if (result == FILE_ERR_NOSPACE)
        {
            result = (* co->co_DestinationFull)(co->co_UserData, fsName);
        }
        else if (((result & 0x1ff) == ER_DeviceOffline)
              || (result == FILE_ERR_NOFILESYSTEM))
        {
            result = (* co->co_NeedDestination)(co->co_UserData, fsName);
        }
        else
        {
            result = (* co->co_WriteError)(co->co_UserData, co->co_WriterContext.wc_WorkPath, result);
        }

        if (!result)
            ClearRawFileError(co->co_WriterContext.wc_RawFile);
    }

    return result;
}


/*****************************************************************************/


static Err DoRead(CopyObj *co)
{
Err   result;
int32 i;
char  fsName[FILESYSTEM_MAX_NAME_LEN+1];

    result = ReaderMachine(co);
    if (result < 0)
    {
        if (((result & 0x1ff) == ER_DeviceOffline)
         || (result == FILE_ERR_NOFILESYSTEM))
        {
            i = -1;
            do
            {
                i++;
                fsName[i] = co->co_ReaderContext.rc_WorkPath[i+1];
            }
            while (fsName[i] && (fsName[i] != '/'));
            fsName[i] = 0;

            result = (* co->co_NeedSource)(co->co_UserData, fsName);
        }
        else if ((result & 0x1ff) == ER_NoMem)
        {
            /* Lack of memory ain't considered an error, merely an
             * inconvenience.
             */
            return 0;
        }
        else
        {
            result = (* co->co_ReadError)(co->co_UserData, co->co_ReaderContext.rc_WorkPath, result);
        }

        if (result)
            ClearRawFileError(co->co_ReaderContext.rc_RawFile);
    }

    return result;
}


/*****************************************************************************/


Err CreateCopyObj(CopyObj **co, const char *sourceDir, const char *objName,
                  const TagArg *tags)
{
Err     result;
TagArg *tag;

    result = FSUTILS_ERR_NOMEM;

    *co = AllocMem(sizeof(CopyObj), MEMTYPE_FILL);
    if (*co)
    {
        (*co)->co_MemoryThreshold         = 1024*1024;  /* 1 meg by default */
        (*co)->co_RootDir.dn_EntryType    = ET_DIRECTORY;
        (*co)->co_RootDir.dn_Complete     = FALSE;
        (*co)->co_RootDir.dn_Name[0]      = 0;
        (*co)->co_RootDir.dn_Parent       = NULL;
        (*co)->co_RootDir.dn_Directory    = NULL;
        (*co)->co_Progress                = DefaultCB_Progress;
        (*co)->co_NeedSource              = DefaultCB_NeedSource;
        (*co)->co_NeedDestination         = DefaultCB_NeedDestination;
        (*co)->co_Duplicate               = DefaultCB_Duplicate;
        (*co)->co_DestinationFull         = DefaultCB_DestinationFull;
        (*co)->co_DestinationProtected    = DefaultCB_DestinationProtected;
        (*co)->co_ReadError               = DefaultCB_ReadError;
        (*co)->co_WriteError              = DefaultCB_WriteError;
        PrepList(&(*co)->co_RootDir.dn_Entries);

        (*co)->co_Lock = result = CreateSemaphore(NULL, 0);
        if ((*co)->co_Lock >= 0)
        {
            while ((tag = NextTagArg(&tags)) != NULL)
            {
                switch (tag->ta_Tag)
                {
                    case COPIER_TAG_USERDATA : (*co)->co_UserData = (void *)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_MEMORYTHRESHOLD
                                             : (*co)->co_MemoryThreshold = (uint32)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_PROGRESSFUNC
                                             : (*co)->co_Progress = (CopyProgressFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_NEEDSOURCEFUNC
                                             : (*co)->co_NeedSource = (CopyNeedSourceFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_NEEDDESTINATIONFUNC
                                             : (*co)->co_NeedDestination = (CopyNeedDestinationFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_DUPLICATEFUNC
                                             : (*co)->co_Duplicate = (CopyDuplicateFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_DESTINATIONFULLFUNC
                                             : (*co)->co_DestinationFull = (CopyDestinationFullFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_DESTINATIONPROTECTEDFUNC:
                                               (*co)->co_DestinationProtected = (CopyDestinationProtectedFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_READERRORFUNC:
                                               (*co)->co_ReadError = (CopyReadErrorFunc)tag->ta_Arg;
                                               break;

                    case COPIER_TAG_WRITEERRORFUNC:
                                               (*co)->co_WriteError = (CopyWriteErrorFunc)tag->ta_Arg;
                                               break;

                    default                  : result = FSUTILS_ERR_BADTAG;
                                               break;
                }

                if (result < 0)
                    break;
            }

            if (result >= 0)
            {
                InitReaderMachine(*co, sourceDir, objName);

                result = DoRead(*co);
                if (result == 0)
                    return 0;

                DeleteCopyObj(*co);
                *co = NULL;

                return result;
            }
            DeleteSemaphore((*co)->co_Lock);
        }
        FreeMem(*co, sizeof(CopyObj));
    }

    *co = NULL;

    return result;
}


/*****************************************************************************/


Err DeleteCopyObj(CopyObj *co)
{
DirNode    *dn;
DirNode    *parent;
FileNode   *fn;
DataBuffer *db;

    if (co)
    {
        CloseRawFile(co->co_ReaderContext.rc_RawFile);

        if (co->co_WriterContext.wc_RawFile)
        {
            CloseRawFile(co->co_WriterContext.wc_RawFile);
            DeleteFile(co->co_WriterContext.wc_WorkPath);
        }

        dn = &co->co_RootDir;
        while (TRUE)
        {
            while (fn = (FileNode *)RemHead(&dn->dn_Entries))
            {
                if (fn->fn_EntryType == ET_FILE)
                {
                    while (db = (DataBuffer *)RemHead(&fn->fn_Data))
                        ReturnMem(co, db, db->db_BufferSize);

                    ReturnMem(co, fn, sizeof(FileNode));
                }
                else
                {
                    dn = (DirNode *)fn;
                }
            }

            CloseDirectory(dn->dn_Directory);
            dn->dn_Directory = NULL;

            parent = dn->dn_Parent;
            if (parent == NULL)
                break;

            ReturnMem(co, dn, sizeof(DirNode));
            dn = parent;
        }

        DeleteSemaphore(co->co_Lock);
        FreeMem(co, sizeof(CopyObj));
    }

    return 0;
}


/*****************************************************************************/


Err PerformCopy(CopyObj *co, const char *destinationDir)
{
bool writeMode;
Err  result;

    InitWriterMachine(co, destinationDir);

    writeMode = TRUE;
    while (TRUE)
    {
        if (writeMode)
        {
            result = DoWrite(co);
            if (result)
                return result;

            if (co->co_WriterContext.wc_State == STATE_DONE)
            {
                /* copy completed! */
                return 0;
            }

            writeMode = FALSE;
        }
        else
        {
            if (co->co_OutOfMemory)
                return FSUTILS_ERR_NOMEM;

            result = DoRead(co);
            if (result)
                return result;

            writeMode = TRUE;
        }
    }
}


/*****************************************************************************/


Err QueryCopyObj(CopyObj *co, const TagArg *tags)
{
TagArg *tag;
uint32 *data;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        data = (uint32 *)tag->ta_Arg;
        if (!IsMemWritable(data,4))
            return FSUTILS_ERR_BADPTR;

        switch (tag->ta_Tag)
        {
            case COPIER_TAG_DIRECTORYCOUNT	: *data = co->co_ReaderContext.rc_DirCount;
                                         	  break;

            case COPIER_TAG_FILECOUNT		: *data = co->co_ReaderContext.rc_FileCount;
                                         	  break;

            case COPIER_TAG_FILEBYTES		: *data = co->co_ReaderContext.rc_FileBytes;
                                         	  break;

            case COPIER_TAG_ISREADCOMPLETE	: *data = (uint32)(co->co_ReaderContext.rc_State == STATE_DONE);
                                         	  break;

            default                    		: return FSUTILS_ERR_BADTAG;
        }
    }

    return 0;
}
