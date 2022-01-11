/* @(#) main_loop.c 96/10/30 1.12 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/list.h>
#include <kernel/time.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fsutils.h>
#include <ui/icon.h>
#include <ui/requester.h>
#include <graphics/font.h>
#include <graphics/frame2d/spriteobj.h>
#include <international/intl.h>
#include <misc/date.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "eventmgr.h"
#include "dirscan.h"
#include "framebuf.h"
#include "msgstrings.h"
#include "utils.h"
#include "listviews.h"
#include "highlight.h"
#include "bg.h"
#include "animlists.h"
#include "overlays.h"
#include "controls.h"
#include "ioserver.h"
#include "hierarchy.h"
#include "boxes.h"
#include "sound.h"
#include "eventloops.h"

/*****************************************************************************/


typedef enum
{
    AREA_HIERARCHY,
    AREA_LISTING,
    AREA_CONTROLS
} Areas;


/*****************************************************************************/


static Control *FindControl(List *controlList, uint32 id)
{
Control *ctrl;
MinNode *n;

    ScanList(controlList, n, MinNode)
    {
        ctrl = Control_Addr(n);
        if (ctrl->ctrl_Type == id)
            return ctrl;
    }

    return NULL;
}


/*****************************************************************************/


static void RenderEntry(int16 x, int16 y, int16 width, int16 height, int16 entryNum, StorageReq *req)
{
EntryNode    *en;
char         *line;
char         *size;
PenInfo       pi;
StringExtent  se;
int32         textWidth;

    TOUCH(width);
    TOUCH(height);

    ScanList(req->sr_DirEntries, en, EntryNode)
    {
        if (entryNum == 0)
        {
            pi.pen_X        = x + 20;
            pi.pen_Y        = y + req->sr_SampleChar.cd_Ascent;
            pi.pen_FgColor  = (en->en_Flags & ENTRY_IS_IN_COPYLIST) ? TEXT_COLOR_VIEWLIST_COPIED : TEXT_COLOR_VIEWLIST_NORMAL;
            pi.pen_BgColor  = 0;
            pi.pen_XScale   = 1.0;
            pi.pen_YScale   = 1.0;
            pi.pen_Flags    = 0;
            pi.pen_reserved = 0;

            line = en->en_Name;

            if (EntryType(en) == ET_DIRECTORY)
            {
                size = NULL;
            }
            else if (EntryType(en) == ET_FILESYSTEM)
            {
                size = ((FileSysEntry *)en)->fs_SizeString;
            }
            else
            {
                size = ((FileEntry *)en)->fe_SizeString;
            }

            /* Draw the name */
            DrawString(req->sr_GState, req->sr_Font, &pi, line, strlen(line));

            /* If it's a file, draw it's size */
            if (size)
            {
                GetStringExtent(&se, req->sr_Font, &pi, size, strlen(size));
                textWidth = se.se_TopRight.x - se.se_TopLeft.x + 1;
                pi.pen_X = x + width - textWidth;
                DrawString(req->sr_GState, req->sr_Font, &pi, size, strlen(size));
            }
            break;
        }
        entryNum--;
    }
}


/*****************************************************************************/


static void AdjustControls(StorageReq *req, List *controlList)
{
EntryNode    *en;
int32         entry;

    DisableControl(req, FindControl(controlList, CTRL_TYPE_OK));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_LOAD));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_SAVE));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_DELETE));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_COPY));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_MOVE));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_RENAME));
    DisableControl(req, FindControl(controlList, CTRL_TYPE_CREATEDIR));

    if ((entry = GetSelectedEntry(&req->sr_ListView)) >= 0)
    {
        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, entry);

		if (!(en->en_Flags & ENTRY_IS_IN_COPYLIST))
		{
			EnableControl(req, FindControl(controlList, CTRL_TYPE_DELETE));
    	    EnableControl(req, FindControl(controlList, CTRL_TYPE_RENAME));
			if (!req->sr_CopyPending)
			{
				EnableControl(req, FindControl(controlList, CTRL_TYPE_MOVE));
			}
		
			if (!req->sr_MovePending)
			{
	        	EnableControl(req, FindControl(controlList, CTRL_TYPE_COPY));
			}
		}
		
        if (req->sr_Hierarchy.h_NumEntries > 1)
        {
			EnableControl(req, FindControl(controlList, CTRL_TYPE_OK));
			EnableControl(req, FindControl(controlList, CTRL_TYPE_LOAD));
			EnableControl(req, FindControl(controlList, CTRL_TYPE_SAVE));
        }
    }

    if (req->sr_Hierarchy.h_NumEntries > 1 && !req->sr_DirScanner->ds_IsInCopyList)
	{
        EnableControl(req, FindControl(controlList, CTRL_TYPE_CREATEDIR));
	}
}


/*****************************************************************************/


static Err GetNewDirectory(StorageReq *req, List *controlList)
{
Err			 status;
ServerPacket packet;
uint32       oldNum;

    oldNum = req->sr_Hierarchy.h_NumEntries;

    status = GetFullPath(req->sr_DirectoryBuffer, sizeof(req->sr_DirectoryBuffer));
	if (status < 0) 
	{
		return status;
	}
	
    GetHierarchy(req, &req->sr_Hierarchy, req->sr_DirectoryBuffer);

    if (req->sr_Options & STORREQ_OPTION_CHANGEDIR)
    {
        if (oldNum < req->sr_Hierarchy.h_NumEntries)
        {
            AddBoxes(req, &req->sr_Boxes, req->sr_Hierarchy.h_NumEntries - oldNum);
        }
        else if (oldNum > req->sr_Hierarchy.h_NumEntries)
        {
            RemoveBoxes(req, &req->sr_Boxes, oldNum - req->sr_Hierarchy.h_NumEntries);
        }
    }

    DisableHighlight(req, &req->sr_Highlight);

    packet.sp_Action     = SERVER_SCAN;
    packet.sp_Path       = req->sr_DirectoryBuffer;
    packet.sp_StatusPort = req->sr_IOStatusPort;
    IOLoop(req, &packet, NULL);

    EnableHighlight(req, &req->sr_Highlight);

    SetListViewNumEntries(req, &req->sr_ListView, req->sr_DirScanner->ds_NumEntries);
    AdjustControls(req, controlList);

    if (req->sr_Options & STORREQ_OPTION_CHANGEDIR)
        MakeBoxVisible(req, &req->sr_Boxes, req->sr_Hierarchy.h_NumEntries - 1);

    return 0;
}


/*****************************************************************************/


static Err CmdDelete(StorageReq *req, List *controlList)
{
EntryNode    *en;
int32         entry;
char          path[FILESYSTEM_MAX_PATH_LEN+1];
Err           result;
ServerPacket  packet;

    result = 0;

    entry = GetSelectedEntry(&req->sr_ListView);
    if (entry >= 0)
    {
        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, entry);

        strcpy(path, req->sr_DirectoryBuffer);
        AppendPath(path, en->en_Name, sizeof(path));

        DisableHighlight(req, &req->sr_Highlight);

        result = DeleteLoop(req, en, FindControl(controlList, CTRL_TYPE_DELETE));
        if (result >= 0)
        {
            packet.sp_Action     = SERVER_DELETE;
            packet.sp_Path       = path;
            packet.sp_StatusPort = req->sr_IOStatusPort;

            result = IOLoop(req, &packet, FindControl(controlList, CTRL_TYPE_DELETE));
            if (result == 0)
            {
                if (!req->sr_DeletedTextAnimation)
                {
                    PlaySound(req, SOUND_DELETED);
                    if (PrepMovingText(req, &req->sr_DeletedText, TEXT_DELETED, en->en_Name) >= 0)
                    {
                        InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_DeletedText);
                        req->sr_DeletedTextAnimation = TRUE;
                    }
                }

				if (EntryType(en) != ET_FILESYSTEM) 
				{
	                RemoveEntry(req->sr_DirScanner, entry);
    	            RemoveListViewEntry(req, &req->sr_ListView, entry);
				}
            }
        }

        EnableHighlight(req, &req->sr_Highlight);
    }

    AdjustControls(req, controlList);

    return result;
}


/*****************************************************************************/


static Err CmdStartCopy(StorageReq *req, bool deleteOriginal, const Control *ctrl, List *controlList)
{
EntryNode   *en;
int32        entry;
Err          result;
ServerPacket packet;
CopyStats	  copyStats;

    result = 0;

    entry = GetSelectedEntry(&req->sr_ListView);

    if (entry >= 0)
    {
        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, entry);

		if (en->en_Flags & ENTRY_IS_IN_COPYLIST)
			return 0;	/* it's already in there: no-op (maybe we should whine?) */

        if (req->sr_CopiedTextAnimation)				
        {												
            RemNode((Node *)&req->sr_CopiedText);		
            UnprepMovingText(req, &req->sr_CopiedText);
            req->sr_CopiedTextAnimation = FALSE;
        }

        UnhighlightControl(req, &req->sr_Suitcase);	/* open the suitcase */

        PlaySound(req, SOUND_TOSUITCASE);
        if (PrepMovingText(req, &req->sr_CopiedText, TEXT_COPIED, en->en_Name) >= 0)
        {
            InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_CopiedText);
            req->sr_CopiedTextAnimation = TRUE;
        }

        packet.sp_Action     = SERVER_START_COPY;
        packet.sp_Path       = req->sr_DirectoryBuffer;
        packet.sp_CopyName   = en->en_Name;
        packet.sp_StatusPort = req->sr_IOStatusPort;

        DisableHighlight(req, &req->sr_Highlight);
        result = IOLoop(req, &packet, ctrl);
		EnableHighlight(req, &req->sr_Highlight);

        if (result == 0)
        {
			en->en_Flags |= ENTRY_IS_IN_COPYLIST;
			if (deleteOriginal) 
			{
				req->sr_MovePending	= TRUE;
			}
			else
			{
				req->sr_CopyPending	= TRUE;
			}
		}

		
		packet.sp_Action     = SERVER_QUERY_COPY;
		packet.sp_StatusPort = req->sr_IOStatusPort;
		packet.sp_data.sp_CopyStats = &copyStats;
		IOLoop(req, &packet, ctrl);	/* this one pretty much can't fail */
	
		if (copyStats.dirCount == 0 && copyStats.fileCount == 0) 
		{
			req->sr_MovePending = FALSE;
			req->sr_CopyPending = FALSE;
		}

        ScrollListView(req, &req->sr_ListView, entry + 1);
   		AdjustControls(req, controlList);
    }

    return result;
}


/*****************************************************************************/


static Err CmdEndCopy(StorageReq *req, List *controlList)
{
Err           result;
ServerPacket  packet;
Control      *ctrl;
CopyStats	  copyStats;

    if (req->sr_MovePending)
        ctrl = FindControl(controlList, CTRL_TYPE_MOVE);
    else
        ctrl = FindControl(controlList, CTRL_TYPE_COPY);

	packet.sp_Action     = SERVER_QUERY_COPY;
	packet.sp_StatusPort = req->sr_IOStatusPort;
	packet.sp_data.sp_CopyStats = &copyStats;
	IOLoop(req, &packet, ctrl);	/* this one pretty much can't fail */

	result = CopyLoop(req, &copyStats, ctrl);
	
	if (result != 0) /* user wants to cancel, IE, empty the suitcase */
	{
		packet.sp_Action     = SERVER_CANCEL_COPY;
		packet.sp_StatusPort = req->sr_IOStatusPort;
		IOLoop(req, &packet, ctrl);
		if (req->sr_CopiedTextAnimation)
		{
			RemNode((Node *)&req->sr_CopiedText);
			UnprepMovingText(req, &req->sr_CopiedText);
			req->sr_CopiedTextAnimation = FALSE;
		}
		UnhighlightControl(req, &req->sr_Suitcase);
		req->sr_MovePending	 = FALSE;
		req->sr_CopyPending	 = FALSE;
	}
	else
	{
		packet.sp_Action     = SERVER_END_COPY;
		packet.sp_Path       = req->sr_DirectoryBuffer;
		packet.sp_StatusPort = req->sr_IOStatusPort;
		packet.sp_data.sp_DeleteAfterCopy = req->sr_MovePending;
		
		DisableHighlight(req, &req->sr_Highlight);
		result = IOLoop(req, &packet, ctrl);
		EnableHighlight(req, &req->sr_Highlight);
	
		if (result == 0)
		{
			UnhighlightControl(req, &req->sr_Suitcase);
			req->sr_MovePending	 = FALSE;
			req->sr_CopyPending	 = FALSE;
		}
	}
	
	GetNewDirectory(req, controlList);	/* refresh view window with results of copy/move */
   	AdjustControls(req, controlList);

    return result;
}



/*****************************************************************************/


static Err CmdCreateDir(StorageReq *req, List *controlList)
{
char         path[FILESYSTEM_MAX_PATH_LEN];
Err          result;
char         newDir[FILESYSTEM_MAX_NAME_LEN];
ServerPacket packet;

    result = 0;

    newDir[0] = 0;
    DisableHighlight(req, &req->sr_Highlight);

    if (TextLoop(req, newDir, sizeof(newDir)) > 0)
    {
        strcpy(path, req->sr_DirectoryBuffer);
        AppendPath(path, newDir, sizeof(path));

        packet.sp_Action     = SERVER_CREATEDIR;
        packet.sp_Path       = path;
        packet.sp_StatusPort = req->sr_IOStatusPort;

        result = IOLoop(req, &packet, FindControl(controlList, CTRL_TYPE_CREATEDIR));
        if (result >= 0)
        {
            result = AddEntry(req->sr_DirScanner, path);
            if (result >= 0)
            {
                AddListViewEntry(req, &req->sr_ListView, result);
                ScrollListView(req, &req->sr_ListView, result);
            }
        }
    }
    EnableHighlight(req, &req->sr_Highlight);

    AdjustControls(req, controlList);

    return result;
}


/*****************************************************************************/


static Err CmdRename(StorageReq *req, List *controlList)
{
char         path[FILESYSTEM_MAX_PATH_LEN];
Err          result;
char         newName[FILESYSTEM_MAX_NAME_LEN];
EntryNode   *en;
int32        entry;
ServerPacket packet;

    result = 0;

    entry = GetSelectedEntry(&req->sr_ListView);
    if (entry >= 0)
    {
        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, entry);

        strcpy(newName, en->en_Name);

        DisableHighlight(req, &req->sr_Highlight);

        if (TextLoop(req, newName, sizeof(newName)) > 0)
        {
			if (strcmp(newName, en->en_Name) != 0)
			{	/* only do rename if user actually changed name during text dialog... */
				strcpy(path, req->sr_DirectoryBuffer);
				AppendPath(path, en->en_Name, sizeof(path));
	
				packet.sp_Action     = SERVER_RENAME;
				packet.sp_Path       = path;
				packet.sp_CopyName   = newName;
				packet.sp_StatusPort = req->sr_IOStatusPort;
	
				result = IOLoop(req, &packet, FindControl(controlList, CTRL_TYPE_RENAME));
				if (result == 0)
				{
					RemoveEntry(req->sr_DirScanner, entry);
	
					strcpy(path, req->sr_DirectoryBuffer);
					AppendPath(path, newName, sizeof(path));
	
					result = AddEntry(req->sr_DirScanner, path);
					if (result >= 0)
						ScrollListView(req, &req->sr_ListView, result);
				}
			}
        }

        EnableHighlight(req, &req->sr_Highlight);
    }

    AdjustControls(req, controlList);

    return result;
}


/*****************************************************************************/


static void GetDateStr(Item locale, const TimeVal *tv,
                       char *result, uint32 resultSize)
{
GregorianDate  gd;
unichar        unibuffer[80];
Locale        *loc;

    loc = intlLookupLocale(locale);
    ConvertTimeValToGregorian(tv, &gd);

    intlFormatDate(loc->loc.n_Item, loc->loc_Date, &gd, unibuffer, sizeof(unibuffer));
    intlTransliterateString(unibuffer, INTL_CS_UNICODE, result, INTL_CS_ASCII, resultSize, ' ');
}


/*****************************************************************************/


static void AddPair(List *l, const char *label, const char *info)
{
InfoPair *ip;

    ip = AllocMem(sizeof(InfoPair), MEMTYPE_NORMAL);
    if (ip)
    {
        stccpy(ip->ip_Label, label, sizeof(ip->ip_Label));
        stccpy(ip->ip_Info, info, sizeof(ip->ip_Info));
        AddTail(l, (Node *)ip);
    }
}


/*****************************************************************************/


static Err CmdInfo(StorageReq *req)
{
EntryNode     *en;
int32          entry;
FileSysEntry  *fs;
char           path[FILESYSTEM_MAX_PATH_LEN];
Err            result;
ServerPacket   packet;
EntryInfo      ei;
List           list;
InfoPair      *ip;
char           buffer[128];
uint32         total;
uint32         free;
uint32         percentage;

    result = 0;

    entry = GetSelectedEntry(&req->sr_ListView);
    if (entry >= 0)
    {
        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, entry);

        strcpy(path, req->sr_DirectoryBuffer);
        AppendPath(path, en->en_Name, sizeof(path));

        DisableHighlight(req, &req->sr_Highlight);

        packet.sp_Action     = SERVER_GETINFO;
        packet.sp_Path       = path;
        packet.sp_Info       = &ei;
        packet.sp_StatusPort = req->sr_IOStatusPort;

        result = IOLoop(req, &packet, NULL);
        if (result >= 0)
        {
            PrepList(&list);
            AddPair(&list, MSG_INFO_NAME, en->en_Name);

            if (ei.ei_FileSystem)
            {
                fs         = (FileSysEntry *)en;
                total      = fs->fs_Stat.fst_Size * fs->fs_Stat.fst_BlockSize;
                free       = fs->fs_Stat.fst_Free * fs->fs_Stat.fst_BlockSize;
                percentage = (fs->fs_Stat.fst_Used*100) / fs->fs_Stat.fst_Size;

                sprintf(buffer, MSG_INFO_AMOUNT, (total - free) / 1024, total / 1024, percentage);
                AddPair(&list, MSG_INFO_CAPACITY, buffer);

                if (ei.ei_DirEntry.de_Flags & FILE_IS_READONLY)
                    AddPair(&list, MSG_INFO_STATUS, MSG_INFO_READ_ONLY);
                else
                    AddPair(&list, MSG_INFO_STATUS, MSG_INFO_WRITABLE);

                if (fs->fs_Stat.fst_BitMap & FSSTAT_CREATETIME)
                {
                    GetDateStr(req->sr_Locale, &fs->fs_Stat.fst_CreateTime, buffer, sizeof(buffer));
                    AddPair(&list, MSG_INFO_CREATED, buffer);
                }
            }
            else if (ei.ei_DirEntry.de_Flags & FILE_IS_DIRECTORY)
            {
            }
            else
            {
                AddPair(&list, MSG_INFO_SIZE, ((FileEntry *)en)->fe_SizeString);

                if (ei.ei_Creator[0])
                    AddPair(&list, MSG_INFO_CREATED_BY, ei.ei_Creator);
            }

            if (ei.ei_DirEntry.de_Date.tv_Seconds || ei.ei_DirEntry.de_Date.tv_Microseconds)
            {
                GetDateStr(req->sr_Locale, &ei.ei_DirEntry.de_Date, buffer, sizeof(buffer));
                AddPair(&list, MSG_INFO_LAST_MODIFIED, buffer);
            }

            if (!ei.ei_FileSystem)  /* no room on FS display */
            {
                if (ei.ei_DirEntry.de_Version || ei.ei_DirEntry.de_Revision)
                {
                    sprintf(buffer, "%u.%u", ei.ei_DirEntry.de_Version, ei.ei_DirEntry.de_Revision);
                    AddPair(&list, MSG_INFO_VERSION, buffer);
                }
            }

            InfoLoop(req, &list);

            while (ip = (InfoPair *)RemHead(&list))
                FreeMem(ip, sizeof(InfoPair));
        }
        EnableHighlight(req, &req->sr_Highlight);
    }

    return result;
}


/*****************************************************************************/


static void HandleUnformattedMedia(StorageReq *req, List *controlList)
{
List        *l;
DeviceStack *ds;
ServerPacket packet;
Item         device;
Item         ior;
bool         ok;
IOInfo       ioInfo;
DeviceStatus status;

    DisableHighlight(req, &req->sr_Highlight);

    if (GetUnmountedList(&l) >= 0)
    {
        ScanList(l, ds, DeviceStack)
        {
            ok = FALSE;

            device = OpenDeviceStack(ds);
            if (device >= 0)
            {
                ior = CreateIOReq(NULL, 0, device, 0);
                if (ior >= 0)
                {
                    memset(&ioInfo, 0, sizeof(IOInfo));
                    ioInfo.ioi_Command         = CMD_STATUS;
                    ioInfo.ioi_Recv.iob_Buffer = &status;
                    ioInfo.ioi_Recv.iob_Len    = sizeof(status);
                    if (DoIO(ior, &ioInfo) >= 0)
                        ok = TRUE;

                    DeleteIOReq(ior);
                }

                if (!ok || ((status.ds_DeviceUsageFlags & DS_USAGE_TRUEROM) == 0))
                {
                    /* this would be where to ask the user whether to format or not */

                    packet.sp_Action     = SERVER_FORMAT;
                    packet.sp_Path       = (char *)ds;
                    packet.sp_StatusPort = req->sr_IOStatusPort;

                    IOLoop(req, &packet, NULL);
                }

                CloseDeviceStack(device);
            }
        }

        DeleteUnmountedList(l);
    }

    EnableHighlight(req, &req->sr_Highlight);
    AdjustControls(req, controlList);
}


/*****************************************************************************/


static void SelectDir(StorageReq *req, HierarchyEntry *dir)
{
int32  boxNum;
Corner corners[NUM_HIGHLIGHT_CORNERS];

    boxNum = GetNodePosFromHead(&req->sr_Hierarchy.h_Entries, (Node *)dir);
    MakeBoxVisible(req, &req->sr_Boxes, boxNum);
    GetBoxCorners(req, &req->sr_Boxes, boxNum, corners);
    MoveHighlight(req, &req->sr_Highlight, corners);
}


/*****************************************************************************/


typedef struct
{
    uint32     fm_Option;
    ControlTypes fm_ControlType;
} FlagMap;

static const FlagMap flagMap[] =
{
    {STORREQ_OPTION_OK,         CTRL_TYPE_OK},
    {STORREQ_OPTION_LOAD,       CTRL_TYPE_LOAD},
    {STORREQ_OPTION_SAVE,       CTRL_TYPE_SAVE},
    {STORREQ_OPTION_CANCEL,     CTRL_TYPE_CANCEL},
    {STORREQ_OPTION_EXIT,       CTRL_TYPE_EXIT},
    {STORREQ_OPTION_QUIT,       CTRL_TYPE_QUIT},
    {STORREQ_OPTION_DELETE,     CTRL_TYPE_DELETE},
    {STORREQ_OPTION_COPY,       CTRL_TYPE_COPY},
    {STORREQ_OPTION_MOVE,       CTRL_TYPE_MOVE},
    {STORREQ_OPTION_RENAME,     CTRL_TYPE_RENAME},
    {STORREQ_OPTION_CREATEDIR,  CTRL_TYPE_CREATEDIR},
    {0,0}
};


/*****************************************************************************/


Err MainLoop(StorageReq *req)
{
bool            exit;
uint32          i;
uint32          buttonY;
List            controlList;
Item            msg;
Event           event;
Corner          corners[NUM_HIGHLIGHT_CORNERS];
EntryNode      *en;
Areas           currentArea;
Control        *currentControl;
Control        *shortcutExitControl;
bool            currentlyOnSuitcase;
Err             result;
HierarchyEntry *currentDir;
int32           sigs;
Control        *ctrl;
uint16          widest;
MinNode        *n;
bool            suitcaseAdded;
bool            navigate;

    SampleSystemTimeTV(&req->sr_CurrentTime);

    req->sr_DeletedTextAnimation = FALSE;
    req->sr_CopiedTextAnimation  = FALSE;

    result              = 0;
    suitcaseAdded       = FALSE;
    currentlyOnSuitcase = TRUE;
	shortcutExitControl	= NULL;
	
    PrepList(&controlList);
    PrepAnimList(req, &req->sr_AnimList);

    buttonY = BUTTON_Y;
    i       = 0;
    widest  = 0;

    navigate = FALSE;
    if (req->sr_Options & STORREQ_OPTION_CHANGEDIR)
    {
        navigate            = TRUE;
        currentlyOnSuitcase = FALSE;
    }

    while (flagMap[i].fm_Option)
    {
        if (flagMap[i].fm_Option & (req->sr_Options | 0x80000000))
        {
            ctrl = AllocMem(sizeof(Control), MEMTYPE_NORMAL);

            if (ctrl)
            {
                PrepControl(req, ctrl, 0, buttonY, flagMap[i].fm_ControlType);

                if (ctrl->ctrl_TotalWidth > widest)
                    widest = ctrl->ctrl_TotalWidth;

                AddTail(&controlList, (Node *)&ctrl->ctrl_Link);
                InsertAnimNode(&req->sr_AnimList, (AnimNode *)ctrl);
                buttonY += ctrl->ctrl_TotalHeight + BUTTON_SPACING;
				
				if (flagMap[i].fm_Option & (STORREQ_OPTION_CANCEL | STORREQ_OPTION_EXIT | STORREQ_OPTION_QUIT))
				{
					shortcutExitControl = ctrl;
				}
            }
            else
            {
                result = REQ_ERR_NOMEM;
                break;
            }
        }
        i++;
    }

    if (result >= 0)
    {
        ScanList(&controlList, n, MinNode)                      /* center all the controls */
        {
            ctrl = Control_Addr(n);
            PositionControl(req, ctrl, BUTTON_X + ((widest - ctrl->ctrl_Width) / 2), ctrl->ctrl_TotalY);
        }

        if (req->sr_Options & (STORREQ_OPTION_MOVE | STORREQ_OPTION_COPY))
        {
            PrepControl(req, &req->sr_Suitcase, SUITCASE_X, SUITCASE_Y, CTRL_TYPE_END_COPY);
            req->sr_Suitcase.ctrl.an.n_Priority = 101;
            InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_Suitcase);
            suitcaseAdded = TRUE;
        }
    }

    if (navigate)
    {
        PrepBoxes(req, &req->sr_Boxes, BOXES_X, BOXES_Y, BOXES_WIDTH, BOXES_HEIGHT);
        InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_Boxes);
    }

    PrepHighlight(req, &req->sr_Highlight);
    InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_Highlight);

    PrepBackground(req, &req->sr_Bg, req->sr_BgSlices, BG_DRAWBAR | BG_DRAWPROMPT);

    InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_Bg);

    {
    ListViewArgs lva;

        lva.lva_X           = LISTVIEW_X;
        lva.lva_Y           = LISTVIEW_Y;
        lva.lva_Width       = LISTVIEW_WIDTH;
        lva.lva_Height      = LISTVIEW_HEIGHT;
        lva.lva_EntryHeight = (req->sr_SampleChar.cd_CharHeight + req->sr_SampleChar.cd_Leading);
        lva.lva_RenderFunc  = (RenderEntryFunc)RenderEntry;
        lva.lva_UserData    = req;

        PrepListView(req, &req->sr_ListView, &lva);
        InsertAnimNode(&req->sr_AnimList, (AnimNode *)&req->sr_ListView);
    }

    PrepHierarchy(&req->sr_Hierarchy);

    if (result >= 0)
    {
        GetListViewCorners(&req->sr_ListView, corners);
        MoveHighlightNow(req, &req->sr_Highlight, corners);
        DisableHighlightNow(req, &req->sr_Highlight);

        result = GetNewDirectory(req, &controlList);
        if (result >= 0)
        {
            currentDir  = (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries);
            currentArea = AREA_LISTING;

            if (IsListEmpty(&controlList))
                currentControl = NULL;
            else
                currentControl = Control_Addr(FirstNode(&controlList));

            HandleUnformattedMedia(req, &controlList);

            exit = FALSE;
            while (!exit)
            {
				if (req->sr_FSChanged)
				{
					req->sr_FSChanged = FALSE;
					HandleUnformattedMedia(req, &controlList);
					if (GetNewDirectory(req, &controlList) < 0)
					{
						strcpy(req->sr_DirectoryBuffer, "/");
						GetNewDirectory(req, &controlList);
						currentDir  = (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries);
					}
					else
					{
						BlinkFirstBox(req, &req->sr_Boxes);
					}
				}
				
                sigs = WaitSignal(req->sr_RenderSig | MSGPORT(req->sr_EventPort)->mp_Signal);
                if (sigs & req->sr_RenderSig)
                {
                    DoNextFrame(req, &req->sr_AnimList);

                    if (req->sr_DeletedTextAnimation)
                    {
                        if (req->sr_DeletedText.mt_Stabilized)
                        {
                            RemNode((Node *)&req->sr_DeletedText);
                            UnprepMovingText(req, &req->sr_DeletedText);
                            req->sr_DeletedTextAnimation = FALSE;
                        }
                    }

                    if (req->sr_CopiedTextAnimation)
                    {
                        if (req->sr_CopiedText.mt_Stabilized)
                        {
                         	RemNode((Node *)&req->sr_CopiedText);
                         	UnprepMovingText(req, &req->sr_CopiedText);
                        	req->sr_CopiedTextAnimation = FALSE;
							if (req->sr_CopyPending || req->sr_MovePending)
							{
         						HighlightControl(req, &req->sr_Suitcase);	/* close the suitcase */
							} 
							else 
							{
         						UnhighlightControl(req, &req->sr_Suitcase);	/* open the suitcase */
							}
                       }
                    }
                }
                while (!exit)
                {
                    msg = GetMsg(req->sr_EventPort);
                    if (msg <= 0)
                        break;

                    event = *(Event *)(MESSAGE(msg)->msg_DataPtr);
                    ReplyMsg(msg,0,NULL,0);

                    switch (event.ev_Type)
                    {
                        case EVENT_TYPE_RELATIVE_MOVE:
                        {
                            while (!exit)
                            {
                                switch (currentArea)
                                {
                                    case AREA_HIERARCHY:
                                    {
                                        if (event.ev_X > 0)
                                        {
                                            currentArea = AREA_LISTING;
                                            GetListViewCorners(&req->sr_ListView, corners);
                                            MoveHighlight(req, &req->sr_Highlight, corners);
                                            event.ev_X = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y > 0)
                                        {
                                            if (currentDir != (HierarchyEntry *)FirstNode(&req->sr_Hierarchy.h_Entries))
                                            {
                                                currentDir = (HierarchyEntry *)PrevNode(currentDir);
                                                SelectDir(req, currentDir);
                                            }
                                            else if (!currentlyOnSuitcase && suitcaseAdded)
                                            {
                                                currentlyOnSuitcase = TRUE;
                                                GetControlCorners(&req->sr_Suitcase, corners);
                                                MoveHighlight(req, &req->sr_Highlight, corners);
                                            }

                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y < 0)
                                        {
                                            if (currentlyOnSuitcase)
                                            {
                                                if (navigate)
                                                {
                                                    currentlyOnSuitcase = FALSE;
                                                    currentDir          = (HierarchyEntry *)FirstNode(&req->sr_Hierarchy.h_Entries);
                                                    SelectDir(req, currentDir);
                                                }
                                            }
                                            else if (currentDir != (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries))
                                            {
                                                currentDir = (HierarchyEntry *)NextNode(currentDir);
                                                SelectDir(req, currentDir);
                                            }
                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        break;
                                    }

                                    case AREA_LISTING:
                                    {
                                        if (event.ev_X < 0)
                                        {
                                            if (navigate || suitcaseAdded)
                                            {
                                                currentArea = AREA_HIERARCHY;
                                                event.ev_X  = 0;

                                                if (currentlyOnSuitcase)
                                                {
                                                    GetControlCorners(&req->sr_Suitcase, corners);
                                                    MoveHighlight(req, &req->sr_Highlight, corners);
                                                }
                                                else
                                                {
                                                    SelectDir(req, currentDir);
                                                }
                                                continue;
                                            }
                                        }
                                        else if (event.ev_X > 0)
                                        {
                                            if (currentControl)
                                            {
                                                currentArea = AREA_CONTROLS;
                                                GetControlCorners(currentControl, corners);
                                                MoveHighlight(req, &req->sr_Highlight, corners);

                                                if (!currentControl->ctrl_Disabled)
                                                    HighlightControl(req, currentControl);
                                            }

                                            event.ev_X = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y > 0)
                                        {
                                            ScrollListView(req, &req->sr_ListView, GetSelectedEntry(&req->sr_ListView) + 1);
										    AdjustControls(req, &controlList);
                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y < 0)
                                        {
                                            ScrollListView(req, &req->sr_ListView, GetSelectedEntry(&req->sr_ListView) - 1);
										    AdjustControls(req, &controlList);
                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        break;
                                    }

                                    case AREA_CONTROLS:
                                    {
                                        UnselectControl(req, currentControl);
                                        if (event.ev_X < 0)
                                        {
                                            currentArea = AREA_LISTING;
                                            GetListViewCorners(&req->sr_ListView, corners);
                                            MoveHighlight(req, &req->sr_Highlight, corners);
                                            UnhighlightControl(req, currentControl);
                                            event.ev_X = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y > 0)
                                        {
                                            if (currentControl != Control_Addr(LastNode(&controlList)))
                                            {
                                                UnhighlightControl(req, currentControl);
                                                currentControl = Control_Addr(NextNode(&currentControl->ctrl_Link));
                                                GetControlCorners(currentControl, corners);
                                                MoveHighlight(req, &req->sr_Highlight, corners);

                                                if (!currentControl->ctrl_Disabled)
                                                    HighlightControl(req, currentControl);
                                            }
                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        else if (event.ev_Y < 0)
                                        {
                                            if (currentControl != Control_Addr(FirstNode(&controlList)))
                                            {
                                                UnhighlightControl(req, currentControl);
                                                currentControl = Control_Addr(PrevNode(&currentControl->ctrl_Link));
                                                GetControlCorners(currentControl, corners);
                                                MoveHighlight(req, &req->sr_Highlight, corners);

                                                if (!currentControl->ctrl_Disabled)
                                                    HighlightControl(req, currentControl);
                                            }
                                            event.ev_Y = 0;
                                            continue;
                                        }
                                        break;
                                    }

                                    default:
                                    {
                                        break;
                                    }
                                }
                                break;
                            }
                            break;
                        }

                        case EVENT_TYPE_BUTTON_DOWN:
                        {
                            if (event.ev_Button == EVENT_BUTTON_INFO)
                            {
                                CmdInfo(req);
                                break;
                            }

                            if (event.ev_Button == EVENT_BUTTON_STOP && shortcutExitControl != NULL)
                            {
								UnhighlightControl(req, currentControl);
								currentArea 	= AREA_CONTROLS;
								currentControl	= shortcutExitControl;
								GetControlCorners(currentControl, corners);
								MoveHighlightFast(req, &req->sr_Highlight, corners);
								while (!IsMoveHighlightDone(req, &req->sr_Highlight))
								{
									WaitSignal(req->sr_RenderSig);
									DoNextFrame(req, &req->sr_AnimList);
								}
                                HighlightControl(req, currentControl);
                                SelectControl(req, currentControl);
                                break;
                            }

                            switch (currentArea)
                            {
                                case AREA_HIERARCHY:
                                {
                                    if (event.ev_Button == EVENT_BUTTON_SELECT)
                                    {
                                        if (currentlyOnSuitcase)
                                        {
                                            if (req->sr_Hierarchy.h_NumEntries > 1)
                                            {
                                                if (req->sr_CopyPending || req->sr_MovePending)
                                                {
                                                    result = CmdEndCopy(req, &controlList);

                                                    currentArea = AREA_LISTING;
                                                    if (req->sr_MovePending)
                                                        currentControl = FindControl(&controlList, CTRL_TYPE_MOVE);
                                                    else
                                                        currentControl = FindControl(&controlList, CTRL_TYPE_COPY);

                                                    GetListViewCorners(&req->sr_ListView, corners);
                                                    MoveHighlight(req, &req->sr_Highlight, corners);
                                                }
                                                else
                                                {
                                                    PlaySound(req, SOUND_UNAVAILABLE);
                                                }
                                            }
                                        }
                                        else if (currentDir != (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries))
                                        {
                                            char prevdir[80];
                                            HierarchyEntry *he;
											int32 offcount=0;
											EntryNode   *en;

											strcpy(prevdir, req->sr_DirectoryBuffer);
											req->sr_DirectoryBuffer[0] = 0;

											he = (HierarchyEntry *)FirstNode(&req->sr_Hierarchy.h_Entries);
											for (;;)
											{
												AppendPath(req->sr_DirectoryBuffer, he->he.n_Name, sizeof(req->sr_DirectoryBuffer));
												if (he == currentDir)
													break;
												he = (HierarchyEntry *)NextNode(he);
											}

                                            GetNewDirectory(req, &controlList);
           									currentDir  = (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries);

											ScanList(req->sr_DirEntries, en, EntryNode)
											{
												if (strstr(prevdir, en->en_Name))
												{
													ScrollListView(req, &req->sr_ListView, offcount);
												    AdjustControls(req, &controlList);
													break;
												}
												offcount++;
											}

                                            GetListViewCorners(&req->sr_ListView, corners);
                                            MoveHighlight(req, &req->sr_Highlight, corners);
                                            currentArea = AREA_LISTING;
                                        }
                                    }
                                    break;
                                }

                                case AREA_LISTING:
                                {
                                    if (event.ev_Button == EVENT_BUTTON_SELECT)
                                    {
                                        en = (EntryNode *)FindNodeFromHead(&req->sr_DirScanner->ds_Entries, GetSelectedEntry(&req->sr_ListView));
                                        if (navigate && en && (EntryType(en) != ET_FILE))
                                        {
											AppendPath(req->sr_DirectoryBuffer, en->en_Name, sizeof(req->sr_DirectoryBuffer));
                                            GetNewDirectory(req, &controlList);
           									currentDir  = (HierarchyEntry *)LastNode(&req->sr_Hierarchy.h_Entries);
                                        }
                                    }
                                    break;
                                }

                                case AREA_CONTROLS:
                                {
                                    if (event.ev_Button == EVENT_BUTTON_SELECT)
                                        SelectControl(req, currentControl);
                                    break;
                                }
                            }
                            break;
                        }

                        case EVENT_TYPE_BUTTON_UP:
                        {
                            if (event.ev_Button == EVENT_BUTTON_SELECT || (event.ev_Button == EVENT_BUTTON_STOP && shortcutExitControl != NULL))
                            {
                                if (currentArea == AREA_CONTROLS)
                                {
                                    if (currentControl->ctrl_Selected)
                                    {
                                        UnselectControl(req, currentControl);

                                        switch (currentControl->ctrl_Type)
                                        {
                                            case CTRL_TYPE_CANCEL:
                                            case CTRL_TYPE_EXIT:
                                            case CTRL_TYPE_QUIT:
                                            {
                                                exit   = TRUE;
                                                result = STORREQ_CANCEL;
                                                break;
                                            }

                                            case CTRL_TYPE_OK:
                                            case CTRL_TYPE_LOAD:
                                            case CTRL_TYPE_SAVE:
                                            {
                                                exit   = TRUE;
                                                result = STORREQ_OK;
                                                break;
                                            }

                                            case CTRL_TYPE_DELETE:
                                            {
                                                result = CmdDelete(req, &controlList);
                                                break;
                                            }

                                            case CTRL_TYPE_COPY:
                                            case CTRL_TYPE_MOVE:
                                            {
                                                result = CmdStartCopy(req, (currentControl->ctrl_Type == CTRL_TYPE_MOVE), currentControl, &controlList);
                                                break;
                                            }

                                            case CTRL_TYPE_RENAME:
                                            {
                                                result = CmdRename(req, &controlList);
                                                break;
                                            }

                                            case CTRL_TYPE_CREATEDIR:
                                            {
                                                result = CmdCreateDir(req, &controlList);
                                                break;
                                            }
                                        }
                                        HandleUnformattedMedia(req, &controlList); /* FIXME: what is this doing here? */
                                    }
                                    else
                                    {
                                        PlaySound(req, SOUND_UNAVAILABLE);
                                    }
                                }
                            }
                            break;
                        }

                        case EVENT_TYPE_FSCHANGE:
                        {
							req->sr_FSChanged = TRUE;
                            break;
                        }
                    }
                }
            }
        }
    }

    while (ctrl = (Control *)RemHead(&controlList))
    {
        RemNode((Node *)Control_Addr(ctrl));
        UnprepControl(req, Control_Addr(ctrl));
        FreeMem(Control_Addr(ctrl), sizeof(Control));
    }

    UnprepAnimList(req, &req->sr_AnimList);
    UnprepHierarchy(&req->sr_Hierarchy);

    return result;
}
