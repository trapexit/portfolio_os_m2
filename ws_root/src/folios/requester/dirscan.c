/* @(#) dirscan.c 96/10/30 1.6 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/list.h>
#include <kernel/task.h>
#include <kernel/io.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/fsutils.h>
#include <ui/requester.h>
#include <misc/savegame.h>
#include <international/intl.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "msgstrings.h"
#include "req.h"
#include "dirscan.h"


/*****************************************************************************/


static void DeleteEntry(EntryNode *en)
{
    switch (EntryType(en))
    {
        case ET_FILE      : FreeMem(en, sizeof(FileEntry) + strlen(en->en_Name) + 1);
                            break;

        case ET_DIRECTORY : FreeMem(en, sizeof(DirEntry) + strlen(en->en_Name) + 1);
                            break;

        case ET_FILESYSTEM: FreeMem(en, sizeof(FileSysEntry) + strlen(en->en_Name) + 1);
                            break;

#ifdef BUILD_PARANOIA
        default           : printf("Unknown entry type %d in DeleteEntry\n",EntryType(en));
                            break;
#endif
    }
}


/*****************************************************************************/


static Err CreateEntry(DirScanner *ds, EntryNode **resultEntry, DirectoryEntry *de, bool fileSystem)
{
EntryTypes     entryType;
Err            result;
Item           file;
Item           ioreq;
IOInfo         ioInfo;
FileSysEntry  *fsEntry;
DirEntry      *dirEntry;
FileEntry     *fileEntry;
FileSystemStat fsStat;
char           name[FILESYSTEM_MAX_PATH_LEN+3];
unichar        unibuffer[80];
Locale        *loc;

    result = 0;
    *resultEntry = NULL;

    loc = intlLookupLocale(ds->ds_Requester->sr_Locale);

    if (de->de_Flags & FILE_IS_DIRECTORY)
    {
        if (fileSystem)
            entryType = ET_FILESYSTEM;
        else
            entryType = ET_DIRECTORY;
    }
    else
    {
        entryType = ET_FILE;
    }

    if (entryType == ET_FILE)
    {
        fileEntry = AllocMem(sizeof(FileEntry) + strlen(de->de_FileName) + 1, MEMTYPE_FILL);

        if (fileEntry)
        {
            fileEntry->fe.en_Name = (char *)&fileEntry[1];
            strcpy(fileEntry->fe.en_Name, de->de_FileName);
            fileEntry->fe.en_ByteSize = de->de_ByteCount;
			
            intlFormatNumber(ds->ds_Requester->sr_Locale, &loc->loc_Numbers,
                             fileEntry->fe.en_ByteSize, 0, FALSE, FALSE,
                             unibuffer, sizeof(unibuffer));

            intlTransliterateString(unibuffer, INTL_CS_UNICODE, fileEntry->fe_SizeString, INTL_CS_ASCII, sizeof(fileEntry->fe_SizeString), ' ');

            EntryType(fileEntry)  = ET_FILE;

            *resultEntry = (EntryNode *)fileEntry;
        }
        else
        {
            result = REQ_ERR_NOMEM;
        }
    }
    else
        if (entryType == ET_DIRECTORY)
        {
            dirEntry = AllocMem(sizeof(DirEntry) + strlen(de->de_FileName) + 1, MEMTYPE_FILL);

            if (dirEntry)
            {
                dirEntry->de.en_Name = (char *)&dirEntry[1];
                strcpy(dirEntry->de.en_Name, de->de_FileName);
                dirEntry->de.en_ByteSize = 0;
                EntryType(dirEntry)      = ET_DIRECTORY;

                *resultEntry = (EntryNode *)dirEntry;
            }
            else
            {
                result = REQ_ERR_NOMEM;
            }
        }
        else
        {
            if (de->de_Flags & FILE_USER_STORAGE_PLACE)
            {
				/* FIXME: if FindFinalComponent() gets changed to strip leading		*/
				/* slashes from a filesystem name, the following logic MAY not be	*/
				/* be needed anymore... [then again, it may still be needed].		*/
				/* due to quirks in the program, sometimes we're doing a filesystem */
				/* which already has a leading slash on the name, sometimes not.	*/
				/* to make life easier, get the name into the variable 'name' with	*/
				/* a leading slash on it so that following code can just assume it.	*/
				
				if (de->de_FileName[0] == '/') 
				{
					strcpy(name, de->de_FileName);
				}
				else
				{
	                sprintf(name,"/%s",de->de_FileName);
				}
				
                file = result = OpenFile(name);
                if (file >= 0)
                {
                    ioreq = result = CreateIOReq(NULL,0,file,0);
                    if (ioreq >= 0)
                    {
                        memset(&ioInfo,0,sizeof(IOInfo));

                        ioInfo.ioi_Command         = FILECMD_FSSTAT;
                        ioInfo.ioi_Recv.iob_Buffer = &fsStat;
                        ioInfo.ioi_Recv.iob_Len    = sizeof(fsStat);
                        result = DoIO(ioreq,&ioInfo);

                        if (result >= 0)
                        {
                            fsEntry = AllocMem(sizeof(FileSysEntry) + strlen(&name[1]) + 1, MEMTYPE_FILL);
                            if (fsEntry)
                            {
                                fsEntry->fs.en_Name       = (char *)&fsEntry[1];
                                fsEntry->fs_Stat          = fsStat;
                                fsEntry->fs_SizeString[0] = 0;
                                strcpy(fsEntry->fs.en_Name, &name[1]);
                                EntryType(fsEntry)  = ET_FILESYSTEM;

                                *resultEntry = (EntryNode *)fsEntry;
                            }
                            else
                            {
                                result = REQ_ERR_NOMEM;
                            }
                        }
                        DeleteIOReq(ioreq);
                    }
                    CloseFile(file);
                }
            }
        }

    return result;
}


/*****************************************************************************/


Err GetEntryInfo(const char *fileName, EntryInfo *info)
{
Err             result;
Item            file;
Item            ioreq;
IOInfo          ioInfo;
FileStatus      fileStatus;
char            name[FILESYSTEM_MAX_PATH_LEN + 10];

    file = result = OpenFile(fileName);

    if (file >= 0)
    {
        ioreq = result = CreateIOReq(NULL, 0, file, 0);
        if (ioreq >= 0)
        {
            memset(&ioInfo, 0, sizeof(IOInfo));

            ioInfo.ioi_Command       = FILECMD_GETPATH;
            ioInfo.ioi_Recv.iob_Buffer = name;
            ioInfo.ioi_Recv.iob_Len = sizeof(name);
            result = DoIO(ioreq, &ioInfo);

            if (result >= 0)
            {
                info->ei_FileSystem = IsFilesystemName(name);

                ioInfo.ioi_Command         = CMD_STATUS;
                ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
                ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);

                result = DoIO(ioreq, &ioInfo);
                if (result >= 0)
                {
                    info->ei_DirEntry.de_Flags     = fileStatus.fs_Flags;
                    info->ei_DirEntry.de_Type      = fileStatus.fs_Type;
                    info->ei_DirEntry.de_ByteCount = fileStatus.fs_ByteCount;
                    info->ei_DirEntry.de_Version   = fileStatus.fs_Version;
                    info->ei_DirEntry.de_Revision  = fileStatus.fs_Revision;
                    info->ei_DirEntry.de_Date      = fileStatus.fs_Date;
                    strcpy(info->ei_DirEntry.de_FileName, FindFinalComponent(name));
                }
            }
            DeleteIOReq(ioreq);
        }
        CloseFile(file);
    }

    if (result >= 0)
    {
        if (!info->ei_FileSystem
         && ((info->ei_DirEntry.de_Flags & FILE_IS_DIRECTORY) == 0))
        {
            if (LoadGameDataVA(LOADGAMEDATA_TAG_FILENAME, fileName,
                               LOADGAMEDATA_TAG_IDSTRING, info->ei_Creator,
                               TAG_END) < 0)
            {
                info->ei_Creator[0] = 0;
            }
        }
    }

    return result;
}


/*****************************************************************************/


Err AddEntry(DirScanner *ds, const char *fileName)
{
Err        result;
EntryNode *en;
EntryInfo  ei;

    if (FindNamedNode(&ds->ds_Entries, FindFinalComponent(fileName)))
        return FILE_ERR_DUPLICATEFILE;

    result = GetEntryInfo(fileName, &ei);
    if (result >= 0)
    {
        result = CreateEntry(ds, &en, &ei.ei_DirEntry, ei.ei_FileSystem);
        if (result >= 0)
        {
			if (ds->ds_IsInCopyList)				 	/* if whole dir is in list, file is is list */
				en->en_Flags |= ENTRY_IS_IN_COPYLIST;
			else										/* else check the file specifically... */
				en->en_Flags |= NameIsInCopyList(ds->ds_UserData, fileName);
            InsertNodeAlpha(&ds->ds_Entries, (Node *)en);
            ds->ds_NumEntries++;
            result = GetNodePosFromHead(&ds->ds_Entries, (Node *)en);
        }
    }

    return result;
}


/*****************************************************************************/


void RemoveEntry(DirScanner *ds, uint32 entryNum)
{
EntryNode *en;

    ScanList(&ds->ds_Entries, en, EntryNode)
    {
        if (entryNum == 0)
        {
            RemNode((Node *)en);
            DeleteEntry(en);
            ds->ds_NumEntries--;
            break;
        }
        entryNum--;
    }
}


/*****************************************************************************/


void ResetDirScanner(DirScanner *ds)
{
EntryNode *en;

    while (en = (EntryNode *)RemHead(&ds->ds_Entries))
    {
        DeleteEntry(en);
    }
    ds->ds_NumEntries = 0;
	ds->ds_IsInCopyList = FALSE;
}


/*****************************************************************************/


void PrepDirScanner(DirScanner *ds, ScanProgressFunc spf, ScanNeedMediaFunc snmf,
                    ScanErrorFunc sef, StorageReq *req, void *userData)
{
    PrepList(&ds->ds_Entries);
    ds->ds_NumEntries = 0;

    ds->ds_Progress  = spf;
    ds->ds_NeedMedia = snmf;
    ds->ds_Error     = sef;
    ds->ds_Requester = req;
    ds->ds_UserData  = userData;
}


/*****************************************************************************/


void UnprepDirScanner(DirScanner *ds)
{
    ResetDirScanner(ds);
}


/*****************************************************************************/


Err ScanDirectory(DirScanner *ds, const char *path)
{
DirectoryEntry de;
DirectoryEntry cachedDE;
Err            result;
Item           dir;
Item           ioreq;
IOInfo         ioInfo;
EntryNode     *en;
char			fullName[FILESYSTEM_MAX_NAME_LEN];

    ResetDirScanner(ds);

	ds->ds_IsInCopyList = NameIsInCopyList(ds->ds_UserData, path);

    dir = result = OpenFile(path);
    if (dir >= 0)
    {
        ioreq = result = CreateIOReq(NULL, 0, dir, 0);
        if (ioreq >= 0)
        {
            memset(&ioInfo, 0, sizeof(IOInfo));

            ioInfo.ioi_Command         = FILECMD_READDIR;
            ioInfo.ioi_Recv.iob_Buffer = &de;
            ioInfo.ioi_Recv.iob_Len    = sizeof(DirectoryEntry);
            ioInfo.ioi_Offset          = 1;

            result = SendIO(ioreq, &ioInfo);

            if (result >= 0)
            {
                while (TRUE)
                {
                    ioInfo.ioi_Offset++;

                    result = (* ds->ds_Progress)(ds->ds_UserData, path);
                    if (result)
                        break;

                    result = WaitIO(ioreq);
                    if (result < 0)
                    {
                        if (result == FILE_ERR_NOFILE)
                        {
                            result = 0;
                            break;
                        }

                        if ((result & 0x1ff) == ER_DeviceOffline)
                            result = (* ds->ds_NeedMedia)(ds->ds_UserData, path);
                        else
                            result = (* ds->ds_Error)(ds->ds_UserData, path, result);

                        if (result)
                            break;
                    }

                    cachedDE = de;

                    result = SendIO(ioreq, &ioInfo);
                    if (result < 0)
                        break;

                    if (ds->ds_Requester->sr_FilterFunc)
                    {
                        if (((* ds->ds_Requester->sr_FilterFunc)(path, &cachedDE)) == FALSE)
                            continue;
                    }

                    result = CreateEntry(ds, &en, &cachedDE, path[1] == 0);
                    if (result < 0)
                        break;

                    if (en)
                    {
						strcpy(fullName, path);
						AppendPath(fullName, en->en_Name, sizeof(fullName));
						if (ds->ds_IsInCopyList)				 	/* if whole dir is in list, file is is list */
							en->en_Flags |= ENTRY_IS_IN_COPYLIST;
						else										/* else check the file specifically... */
							en->en_Flags |= NameIsInCopyList(ds->ds_UserData, fullName);
                        InsertNodeAlpha(&ds->ds_Entries, (Node *)en);
                        ds->ds_NumEntries++;
                    }
                }
            }
            DeleteIOReq(ioreq);
        }
        CloseFile(dir);
    }

    return result;
}
