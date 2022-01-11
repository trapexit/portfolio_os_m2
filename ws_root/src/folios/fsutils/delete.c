/* @(#) delete.c 96/09/12 1.11 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/list.h>
#include <kernel/tags.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
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


typedef struct DirNode
{
    MinNode         dn;
    const char     *dn_Name;
    List            dn_Entries;
    struct DirNode *dn_Parent;
    uint32          dn_FileType;
    bool            dn_Directory;
} DirNode;


typedef enum
{
    STATE_INIT,
    STATE_ENTERDIR,
    STATE_SCANDIR,
    STATE_ENDSCAN,
    STATE_DELETE,
    STATE_VISITDIR,
    STATE_DELETEDIR,
    STATE_EXITDIR,
    STATE_DONE
} States;


/*****************************************************************************/


static Err DefaultCB_Progress(void *userData, const char *path)
{
    TOUCH(userData);
    TOUCH(path);
    return 0;
}

static Err DefaultCB_NeedMedia(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_OFFLINE;
}

static Err DefaultCB_MediaProtected(void *userData, const char *fsName)
{
    TOUCH(userData);
    TOUCH(fsName);
    return FILE_ERR_READONLY;
}

static Err DefaultCB_Error(void *userData, const char *path, Err error)
{
    TOUCH(userData);
    TOUCH(path);
    return error;
}


/*****************************************************************************/


/* Non-recursive delete of a file or directory tree.
 *
 * For directories, we scan them completely, then try to delete all the files
 * and directories we can, then for any directories left, we step into them
 * and apply the same algorithm.
 *
 * The callback functions can tell us to skip the current operation at any
 * time. This means that undeleted files may remain in the tree. The code
 * will currently invoke the callback whenever a directory can't be deleted,
 * which means that if a deeply nested file can't be deleted, the callback's
 * gonna get invoked for each containing directory, since they will all fail
 * to be deleted, since they contain something.
 */
Err DeleteTree(const char *path, const TagArg *tags)
{
DirNode                   root;
DirNode                  *current;
DirNode                  *next;
DirNode                  *parent;
DirNode                  *dn;
DirectoryEntry            de;
Item                      file;
Item                      ioreq;
IOInfo                    ioInfo;
Err                       result;
uint32                    nameLen;
DeleteProgressFunc        progressFunc;
DeleteNeedMediaFunc       needMediaFunc;
DeleteMediaProtectedFunc  mediaProtectedFunc;
DeleteErrorFunc           errorFunc;
TagArg                   *tag;
void                     *userData;
States                    state;
bool                      callback;
bool                      skip;
bool                      nukeNode;
bool                      exit;
const char               *errorPath;
FileStatus                fileStatus;

    progressFunc        = DefaultCB_Progress;
    needMediaFunc       = DefaultCB_NeedMedia;
    mediaProtectedFunc  = DefaultCB_MediaProtected;
    errorFunc           = DefaultCB_Error;
    userData            = NULL;
    result              = 0;
    exit                = FALSE;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case DELETER_TAG_USERDATA : userData = (void *)tag->ta_Arg;
                                        break;

            case DELETER_TAG_PROGRESSFUNC
                                      : progressFunc = (DeleteProgressFunc)tag->ta_Arg;
                                        break;

            case DELETER_TAG_NEEDMEDIAFUNC
                                      : needMediaFunc = (DeleteNeedMediaFunc)tag->ta_Arg;
                                        break;

            case DELETER_TAG_MEDIAPROTECTEDFUNC
                                      : mediaProtectedFunc = (DeleteMediaProtectedFunc)tag->ta_Arg;
                                        break;

            case DELETER_TAG_ERRORFUNC
                                      : errorFunc = (DeleteErrorFunc)tag->ta_Arg;
                                        break;

            default                   : result = FSUTILS_ERR_BADTAG;
                                        break;
        }

        if (result < 0)
            break;
    }

    if (result < 0)
        return result;

    /* keep the compiler quiet */
    dn        = NULL;
    file      = -1;
    ioreq     = -1;
    nameLen   = 0;
    errorPath = NULL;

    PrepList(&root.dn_Entries);
    root.dn_Parent    = NULL;
    root.dn_Directory = TRUE;
    root.dn_Name      = path;
    current           = &root;

    state = STATE_INIT;
    skip  = FALSE;
    do
    {
        DBUG(("DeleteTree: state %d\n",state));

        callback = FALSE;
        switch (state)
        {
            case STATE_INIT     : if (skip)
                                  {
                                      state = STATE_DONE;
                                      break;
                                  }

                                  errorPath = path;

                                  DBUG(("DeleteTree: attempting to delete %s\n",path));

                                  file = result = OpenFile(path);
                                  if (file >= 0)
                                  {
                                      ioreq = result = CreateIOReq(NULL,0,file,0);
                                      if (ioreq >= 0)
                                      {
                                          memset(&ioInfo, 0, sizeof(ioInfo));
                                          ioInfo.ioi_Command         = CMD_STATUS;
                                          ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
                                          ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);

                                          result = DoIO(ioreq, &ioInfo);
                                          DeleteIOReq(ioreq);
                                      }
                                      CloseFile(file);
                                  }

                                  if (result < 0)
                                  {
                                      callback = TRUE;
                                  }
                                  else if (fileStatus.fs_Flags & FILE_IS_DIRECTORY)
                                  {
                                      state = STATE_ENTERDIR;
                                      current->dn_FileType = fileStatus.fs_Type;
                                  }
                                  else
                                  {
                                      result = DeleteFile(path);
                                      if (result >= 0)
                                      {
                                          state = STATE_DONE;
                                      }
                                      else
                                      {
                                          callback = TRUE;
                                      }
                                  }
                                  break;

            case STATE_ENTERDIR : if (skip)
                                  {
                                      state = STATE_EXITDIR;
                                      break;
                                  }

                                  errorPath = current->dn_Name;

                                  DBUG(("DeleteTree: scanning %s\n",current->dn_Name));

                                  file = result = OpenFile(current->dn_Name);
                                  if (file >= 0)
                                  {
                                      ioreq = result = CreateIOReq(NULL,0,file,0);
                                      if (ioreq >= 0)
                                      {
                                          memset(&ioInfo, 0, sizeof(IOInfo));
                                          ioInfo.ioi_Command         = FILECMD_READDIR;
                                          ioInfo.ioi_Recv.iob_Buffer = &de;
                                          ioInfo.ioi_Recv.iob_Len    = sizeof(de);
                                          ioInfo.ioi_Offset          = 1;
                                          nameLen = strlen(current->dn_Name);
                                          state = STATE_SCANDIR;
                                          break;
                                      }
                                      CloseFile(file);
                                  }
                                  callback = TRUE;
                                  break;

            case STATE_SCANDIR  : if (skip)
                                  {
                                      state = STATE_ENDSCAN;
                                      break;
                                  }

                                  result = DoIO(ioreq, &ioInfo);
                                  if (result == FILE_ERR_NOFILE)
                                  {
                                      result = 0;
                                      state = STATE_ENDSCAN;
                                      break;
                                  }
                                  else if (result < 0)
                                  {
                                      callback = TRUE;
                                      break;
                                  }

                                  dn = AllocMem(sizeof(DirNode) + nameLen + strlen(de.de_FileName) + 2, MEMTYPE_NORMAL);
                                  if (dn)
                                  {
                                      dn->dn_Name = (char *)&dn[1];
                                      sprintf((char *)&dn[1], "%s/%s", current->dn_Name, de.de_FileName);
                                      PrepList(&dn->dn_Entries);
                                      dn->dn_Parent   = current;
                                      dn->dn_FileType = de.de_Type;

                                      if (de.de_Flags & FILE_IS_DIRECTORY)
                                          dn->dn_Directory = TRUE;
               	                       else
                                          dn->dn_Directory = FALSE;

                                      AddTail(&current->dn_Entries, (Node *)dn);
                                      ioInfo.ioi_Offset++;
                                  }
                                  else
                                  {
                                      result = FSUTILS_ERR_NOMEM;
                                      callback = TRUE;
                                  }
                                  break;

            case STATE_ENDSCAN  : DeleteIOReq(ioreq);
                                  CloseFile(file);
                                  file  = -1;
                                  ioreq = -1;
                                  dn    = (DirNode *)FirstNode(&current->dn_Entries);
                                  state = STATE_DELETE;
                                  break;

            case STATE_DELETE   : nukeNode = FALSE;
                                  if (skip)
                                  {
                                      /* pretend it was deleted */
                                      nukeNode = TRUE;
                                  }
                                  else
                                  {
                                      if (!IsNode(&current->dn_Entries, dn))
                                      {
                                          /* we've looked at all the entries,
                                           * so go back and step into
                                           * subdirectories
                                           */
                                          state = STATE_VISITDIR;
                                          break;
                                      }

                                      result = (* progressFunc)(userData, dn->dn_Name);
                                      if (result)
                                      {
                                          exit = TRUE;
                                          break;
                                      }

                                      DBUG(("DeleteTree: attempting to delete %s\n",dn->dn_Name));

                                      errorPath = dn->dn_Name;
                                      if (dn->dn_Directory)
                                      {
                                          result = DeleteDirectory(dn->dn_Name);
                                          if (result >= 0)
                                          {
                                              nukeNode = TRUE;
                                          }
                                          else if (result == FILE_ERR_DIRNOTEMPTY)
                                          {
                                              result = 0;
                                          }
                                      }
                                      else
                                      {
                                          result   = DeleteFile(dn->dn_Name);
                                          nukeNode = TRUE;
                                      }

                                      if (result < 0)
                                      {
                                          callback = TRUE;
                                          break;
                                      }
                                  }

                                  next = (DirNode *)NextNode(dn);
                                  if (nukeNode)
                                  {
                                      RemNode((Node *)dn);
                                      FreeMem(dn, sizeof(DirNode) + strlen(dn->dn_Name) + 1);
                                  }
                                  dn = next;
                                  break;

            case STATE_VISITDIR : dn = (DirNode *)FirstNode(&current->dn_Entries);
                                  if (IsNode(&current->dn_Entries, dn))
                                  {
                                      /* handle this nested directory */
                                      current = dn;
                                      state   = STATE_ENTERDIR;
                                  }
                                  else
                                  {
                                      /* nothing left in this dir, get rid of it */
                                      state = STATE_DELETEDIR;
                                  }
                                  break;

            case STATE_DELETEDIR: if (skip)
                                  {
                                      /* give up */
                                      state = STATE_EXITDIR;
                                      break;
                                  }

                                  errorPath = current->dn_Name;

                                  DBUG(("DeleteTree: attempting to delete dir %s\n",current->dn_Name));

                                  result = DeleteDirectory(current->dn_Name);
                                  if (result < 0)
                                  {
                                      callback = TRUE;
                                  }
                                  else
                                  {
                                      state = STATE_EXITDIR;
                                  }
                                  break;

            case STATE_EXITDIR  : parent = current->dn_Parent;
                                  if (parent == NULL)
                                  {
                                      /* reached root, we're done */
                                      state = STATE_DONE;
                                  }
                                  else
                                  {
                                      /* delete current node, and continue visiting parent */
                                      RemNode((Node *)current);
                                      FreeMem(current, sizeof(DirNode) + strlen(current->dn_Name) + 1);
                                      current = parent;
                                      state   = STATE_VISITDIR;
                                  }
                                  break;
        }

        skip = FALSE;

        if (callback)
        {
            if (result == FILE_ERR_READONLY)
            {
                result = (* mediaProtectedFunc)(userData, errorPath);
            }
            else if (((result & 0x1ff) == ER_DeviceOffline)
                  || (result == FILE_ERR_NOFILESYSTEM))
            {
                result = (* needMediaFunc)(userData, errorPath);
            }
            else
            {
                result = (* errorFunc)(userData, errorPath, result);
            }

            if (result == 1)
            {
                /* skip file */
                result = 0;
                skip = TRUE;
            }
        }
        else if ((result >= 0) && !exit)
        {
            result = 0;
        }

        if (result)
        {
            DeleteIOReq(ioreq);
            CloseFile(file);

            current = &root;
            while (TRUE)
            {
                while (dn = (DirNode *)RemHead(&current->dn_Entries))
                {
                    if (!dn->dn_Directory)
                    {
                        FreeMem(dn, sizeof(DirNode) + strlen(dn->dn_Name) + 1);
                    }
                    else
                    {
                        current = dn;
                    }
                }

                parent = current->dn_Parent;
                if (parent == NULL)
                    break;

                FreeMem(current, sizeof(DirNode) + strlen(current->dn_Name) + 1);
                current = parent;
            }

            break;
        }
    }
    while (state != STATE_DONE);

    return result;
}
