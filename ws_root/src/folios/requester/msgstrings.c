/* @(#) msgstrings.c 96/10/30 1.6 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <international/intl.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "msgstrings.h"


/*****************************************************************************/


static const char * const defaultStrings[] =
{
    "OK",                                                   /* MSG_ start here */
    "Load",
    "Save",
    "Cancel",
    "Exit",
    "Quit",
    "Remove",
    "Copy",
    "Move",
    "Rename",
    "New Box",
    "Format",

    "Delete",                                               /* MSG_OVL_ start here */
    "Format",
    "Cancel",
    "Stop",
    "OK",
    "File: %s\n\nDo you really want to\nremove this file?",
    "Box: %s\n\nDo you really want to\nremove this box and\neverything it contains?",
    "Card: %s\n\nDo you really want to\nremove EVERYTHING\nfrom this card?",
    "Do you really want to format\n%s?",
    "The operation could not\nbe completed because an\nerror occured.",
    "%s",
    "The operation could not\nbe completed because there\nis already an entry called\n%s.",
    "The operation could not\nbe completed because\n%s\nis full.",
    "The operation could not\nproceed because\n%s\nis locked.",
    "Please Insert\n%s",
    "Please Wait...",
    "Please Confirm...",                                            /* delete */
    "Please Confirm...",                                            /* format */
    "Error Detected",
    "Information...",
    "Duplicate Names",
    "Media Full",
    "Media Locked",
    "Media Needed",
    "Working...",

    "Card",

    /* info display */
    "Name",
    "Size",
    "Created By",
    "Last Modified",
    "Version",
    "Created",
    "Status",
    "Capacity",
    "Writable",
    "Read-Only",
    "%uK of %uK used (%u%%)",

    "Shift",
    "Clear",
    "Backspace",
	
	"Please Confirm...",											/* copy/move title */
	"%s %d boxes, %d files\ntotaling %d bytes\nto this location?",	/* copy/move stats */
	
	"Invalid destination",
	"You cannot copy entry\n%s\ninto itself or into a\nbox within itself.",
};


/*****************************************************************************/


static const TagArg stringSearchTags[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

void LoadStrings(StorageReq *req)
{
Locale   *loc;
uint32    i;
char      buffer[80];
char      path[80];
FileInfo  fileInfo;
RawFile  *file;
char     *ptr;

    for (i = 0; i < NUM_STRINGS; i++)
        req->sr_MsgStrings[i] = (char *)defaultStrings[i];

    loc = intlLookupLocale(req->sr_Locale);
    if (loc && (loc->loc_Language != INTL_LANG_ENGLISH))
    {
        sprintf(buffer,"System.m2/Requester/Strings/%c%c.strings",
                       (loc->loc_Language >> 24) & 0xff,
                       (loc->loc_Language >> 16) & 0xff);

        if (FindFileAndIdentify(path, sizeof(path), buffer, stringSearchTags) >= 0)
        {
            if (OpenRawFile(&file, path, FILEOPEN_READ) >= 0)
            {
                GetRawFileInfo(file, &fileInfo, sizeof(fileInfo));

                req->sr_StringBlock = AllocMem(fileInfo.fi_ByteCount, MEMTYPE_TRACKSIZE);
                if (req->sr_StringBlock)
                {
                    if (ReadRawFile(file, req->sr_StringBlock, fileInfo.fi_ByteCount) == fileInfo.fi_ByteCount)
                    {
                        ptr = (char *)req->sr_StringBlock;
                        for (i = 0; i < NUM_STRINGS; i++)
                        {
                            req->sr_MsgStrings[i] = ptr;
                            while ((*ptr != '\n') && (*ptr != '\r'))
                                ptr++;

                            *ptr++ = 0;
                        }
                    }
                    else
                    {
                        FreeMem(req->sr_StringBlock, TRACKED_SIZE);
                    }
                }
                CloseRawFile(file);
            }
        }
    }
}


/*****************************************************************************/


void UnloadStrings(StorageReq *req)
{
    FreeMem(req->sr_StringBlock, TRACKED_SIZE);
    req->sr_StringBlock = NULL;
}


/*****************************************************************************/


char *GetString(StorageReq *req, uint32 stringNum)
{
#ifdef BUILD_PARANOIA
    if (stringNum >= NUM_STRINGS)
    {
        printf("ERROR: GetString asked for string %d\n",stringNum);
        return NULL;
    }
#endif

    return req->sr_MsgStrings[stringNum];
}
