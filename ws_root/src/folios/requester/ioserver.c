/* @(#) ioserver.c 96/10/30 1.8 */

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/msgport.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fsutils.h>
#include <ui/requester.h>
#include <string.h>
#include "dirscan.h"
#include "req.h"
#include "ioserver.h"
#include "utils.h"

/*****************************************************************************/

typedef struct {
	MinNode		ci_Link;
	CopyObj *	ci_CopyObj;
	int32		ci_FullNameLen;
	char		ci_FullName[1];
} CopyInfo;


struct IOServer
{
    Item          ios_ServerThread;
    Item          ios_ServerPort;
    int32         ios_ServerAbortSig;
    Item          ios_ServerStatusMsg;

    Item          ios_ClientMsg;
    Message      *ios_ClientMsgPtr;
    Item          ios_ClientPort;
    ServerPacket *ios_ClientPacket;

    DirScanner    ios_DirScanner[2];
    uint32        ios_CurrentDirScanner;
	List		  ios_CopyList;
	CopyStats	  ios_CopyStats;
    StorageReq   *ios_Requester;
};


/*****************************************************************************/


static CopyInfo * CopyListItemInPath(IOServer *ios, const char *path)
{
CopyInfo *	info;
int32		pathLen;

	pathLen = strlen(path);

	SCANLIST(&ios->ios_CopyList, info, CopyInfo)
	{
		if (0 == strncmp(info->ci_FullName, path, pathLen))
		{
			return info;
		}
	}
	
	return NULL;
}



/*****************************************************************************/


bool NameIsInCopyList(IOServer *ios, const char *name)
{
CopyInfo *	info;

	SCANLIST(&ios->ios_CopyList, info, CopyInfo)
	{
		if (0 == strncmp(name, info->ci_FullName, info->ci_FullNameLen))
		{
			return TRUE;
		}
	}
	
	return FALSE;
}


/*****************************************************************************/


static Err CheckAbort(IOServer *ios)
{
    if (GetCurrentSignals() & ios->ios_ServerAbortSig)
    {
        ClearCurrentSignals(ios->ios_ServerAbortSig);
        return SERVER_ABORTED;
    }
	
	return 0;
}

/*****************************************************************************/


static Err ClientSend(IOServer *ios, ServerStatus *status)
{
Err 	result;
Message *msg;

    result = SendMsg(ios->ios_ClientPacket->sp_StatusPort,
                     ios->ios_ServerStatusMsg, status, sizeof(ServerStatus));
    if (result < 0)
		return result;

	msg = MESSAGE(WaitPort(ios->ios_ServerPort, ios->ios_ServerStatusMsg));
	
	result = CheckAbort(ios);
	
	if (result < 0)
		return result;
	else
		return msg->msg_Result;
}


/*****************************************************************************/


Err CopyProgressCallBack(IOServer *ios, const char *path)
{
    TOUCH(path);

	return CheckAbort(ios);
}


/*****************************************************************************/


static Err CopyNeedSourceCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State               = SS_NEED_MEDIA;
    status.ss_Info.ss_MediaSought = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyNeedDestinationCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State               = SS_NEED_MEDIA;
    status.ss_Info.ss_MediaSought = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyDuplicateCallBack(IOServer *ios, const char *path)
{
ServerStatus status;

    status.ss_State                 = SS_DUPLICATE_FILE;
    status.ss_Info.ss_DuplicateFile = path;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyDestinationFullCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State             = SS_MEDIA_FULL;
    status.ss_Info.ss_MediaFull = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyDestinationProtectedCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State                  = SS_MEDIA_PROTECTED;
    status.ss_Info.ss_MediaProtected = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyReadErrorCallBack(IOServer *ios, const char *path, Err error)
{
ServerStatus status;

    TOUCH(path);

    status.ss_State             = SS_ERROR;
    status.ss_Info.ss_ErrorCode = error;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err CopyWriteErrorCallBack(IOServer *ios, const char *path, Err error)
{
ServerStatus status;

    TOUCH(path);

    status.ss_State             = SS_ERROR;
    status.ss_Info.ss_ErrorCode = error;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err ScanProgressCallBack(IOServer *ios, const char *path)
{
    TOUCH(path);
	
	return CheckAbort(ios);
}


/*****************************************************************************/


static Err ScanNeedMediaCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State               = SS_NEED_MEDIA;
    status.ss_Info.ss_MediaSought = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err ScanErrorCallBack(IOServer *ios, const char *path, Err error)
{
ServerStatus status;

    TOUCH(path);

    status.ss_State             = SS_ERROR;
    status.ss_Info.ss_ErrorCode = error;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err DeleteProgressCallBack(IOServer *ios, const char *fileName)
{
    TOUCH(fileName);

	return CheckAbort(ios);
}


/*****************************************************************************/


static Err DeleteNeedMediaCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State               = SS_NEED_MEDIA;
    status.ss_Info.ss_MediaSought = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err DeleteMediaProtectedCallBack(IOServer *ios, const char *fsName)
{
ServerStatus status;

    status.ss_State                  = SS_MEDIA_PROTECTED;
    status.ss_Info.ss_MediaProtected = fsName;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static Err DeleteErrorCallBack(IOServer *ios, const char *path, Err error)
{
ServerStatus status;

	/* If the error is FilesystemBusy and the path is in fact a filesystem 		*/
	/* (IE, the only slash is the first one), we're trying to delete everything	*/
	/* in the card, but we don't want to delete the filesystem itself (we can't)*/
	/* so just return a 1 to tell the deleter engine to quietly skip this item.	*/

	if (error == FILE_ERR_BUSY && IsFilesystemName(path))
	{
		return 1;
	}

    status.ss_State             = SS_ERROR;
    status.ss_Info.ss_ErrorCode = error;
    return ClientSend(ios, &status);
}


/*****************************************************************************/


static void GetFSName(const char *path, char *fsName)
{
uint32 i;

    fsName[0] = 0;
    if (path[0])
    {
        i = 1;
        while ((path[i] != '/') && path[i])
        {
            fsName[i-1] = path[i];
            i++;
        }
        fsName[i-1] = 0;
    }
}


/*****************************************************************************/


static Err DoRename(IOServer *ios, const char *path, const char *newName)
{
Err          result;
ServerStatus status;
char         fsName[FILESYSTEM_MAX_NAME_LEN];

    GetFSName(path, fsName);

    while (TRUE)
    {
        result = Rename(path, newName);
        if (result >= 0)
        {
            return result;
        }
        else if (result == FILE_ERR_READONLY)
        {
            status.ss_State                  = SS_MEDIA_PROTECTED;
            status.ss_Info.ss_MediaProtected = fsName;
            result = ClientSend(ios, &status);
        }
        else if (result == FILE_ERR_DUPLICATEFILE)
        {
            status.ss_State                 = SS_DUPLICATE_FILE;
            status.ss_Info.ss_DuplicateFile = newName;
            result = ClientSend(ios, &status);
        }
        else if (((result & 0x1ff) == ER_DeviceOffline)
             ||  (result == FILE_ERR_NOFILESYSTEM))
        {
            status.ss_State               = SS_NEED_MEDIA;
            status.ss_Info.ss_MediaSought = fsName;
            result = ClientSend(ios, &status);
        }
        else
        {
            status.ss_State             = SS_ERROR;
            status.ss_Info.ss_ErrorCode = result;
            result = ClientSend(ios, &status);
        }

        if (result)
            return result;
    }
}


/*****************************************************************************/


static Err DoCreateDir(IOServer *ios, const char *path)
{
Err          result;
ServerStatus status;
char         fsName[FILESYSTEM_MAX_NAME_LEN];

    GetFSName(path, fsName);

    while (TRUE)
    {
        result = CreateDirectory(path);
        if (result >= 0)
        {
            return result;
        }
        else if (result == FILE_ERR_READONLY)
        {
            status.ss_State                  = SS_MEDIA_PROTECTED;
            status.ss_Info.ss_MediaProtected = fsName;
            result = ClientSend(ios, &status);
        }
        else if (result == FILE_ERR_DUPLICATEFILE)
        {
            status.ss_State                 = SS_DUPLICATE_FILE;
            status.ss_Info.ss_DuplicateFile = path;
            result = ClientSend(ios, &status);
        }
        else if (((result & 0x1ff) == ER_DeviceOffline)
             ||  (result == FILE_ERR_NOFILESYSTEM))
        {
            status.ss_State               = SS_NEED_MEDIA;
            status.ss_Info.ss_MediaSought = fsName;
            result = ClientSend(ios, &status);
        }
        else
        {
            status.ss_State             = SS_ERROR;
            status.ss_Info.ss_ErrorCode = result;
            result = ClientSend(ios, &status);
        }

        if (result)
            return result;
    }
}


/*****************************************************************************/


static Err DoFormat(IOServer *ios, const DeviceStack *ds)
{
Item         device;
Err          result;
ServerStatus status;

    device = OpenDeviceStack(ds);
    if (device < 0)
        return device;

    while (TRUE)
    {
        result = FormatFileSystem(device, "foo", NULL);
        if (result >= 0)
        {
            return result;
        }
        else if (result == FILE_ERR_READONLY)
        {
            status.ss_State                  = SS_MEDIA_PROTECTED;
            status.ss_Info.ss_MediaProtected = "foo"; /* FIXME */
            result = ClientSend(ios, &status);
        }
        else
        {
            status.ss_State             = SS_ERROR;
            status.ss_Info.ss_ErrorCode = result;
            result = ClientSend(ios, &status);
        }

        if (result)
            return result;
    }
}


/*****************************************************************************/


static Err DoInfo(IOServer *ios, const char *path, EntryInfo *info)
{
Err          result;
ServerStatus status;
char         fsName[FILESYSTEM_MAX_NAME_LEN];

    GetFSName(path, fsName);

    while (TRUE)
    {
        result = GetEntryInfo(path, info);
        if (result >= 0)
        {
            return result;
        }
        else if (((result & 0x1ff) == ER_DeviceOffline)
             ||  (result == FILE_ERR_NOFILESYSTEM))
        {
            status.ss_State               = SS_NEED_MEDIA;
            status.ss_Info.ss_MediaSought = fsName;
            result = ClientSend(ios, &status);
        }
        else
        {
            status.ss_State             = SS_ERROR;
            status.ss_Info.ss_ErrorCode = result;
            result = ClientSend(ios, &status);
        }

        if (result)
            return result;
    }
}



/*****************************************************************************/


static Err DoDelete(IOServer *ios, const char *path)
{
 	return DeleteTreeVA(path,
					  DELETER_TAG_USERDATA,           ios,
					  DELETER_TAG_PROGRESSFUNC,       DeleteProgressCallBack,
					  DELETER_TAG_NEEDMEDIAFUNC,      DeleteNeedMediaCallBack,
					  DELETER_TAG_MEDIAPROTECTEDFUNC, DeleteMediaProtectedCallBack,
					  DELETER_TAG_ERRORFUNC,          DeleteErrorCallBack,
					  TAG_END);
}

								  
/*****************************************************************************/


static Err DoCleanupCopy(IOServer *ios)
{
CopyInfo *	info;

	while ((info = (CopyInfo *)RemHead(&ios->ios_CopyList)) != NULL) 
	{
		DeleteCopyObj(info->ci_CopyObj);
		FreeMemTrack(info);
	}
	
	memset(&ios->ios_CopyStats, 0, sizeof(ios->ios_CopyStats));
	
	return 0;
}


/*****************************************************************************/


static Err DoEndCopy(IOServer *ios, const char *path, bool deleteAfterCopy)
{
Err			result = 0;
CopyInfo *	info;
ServerStatus status;

	if (NameIsInCopyList(ios, path))
	{
		status.ss_State                 = SS_HIERARCHY_COPY;
		status.ss_Info.ss_HierarchyName = path;
		return ClientSend(ios, &status);
	}
	
	SCANLIST(&ios->ios_CopyList, info, CopyInfo)
	{
		result = PerformCopy(info->ci_CopyObj, path);
		if (result != 0) 
			return result;
	}
	
	if (deleteAfterCopy)
	{
		SCANLIST(&ios->ios_CopyList, info, CopyInfo)
		{
			result = DoDelete(ios, info->ci_FullName);
			if (result != 0)
				break; /* don't return, we should still do the cleanup */
		}
	} else {
		result = 0;
	}
	
	DoCleanupCopy(ios);

	return result;
}


/*****************************************************************************/


static Err DoStartCopy(IOServer *ios, const char *path, const char *name)
{
Err			result;
CopyInfo *	info;
uint32		readIsComplete;
uint32		dirCount;
uint32		fileCount;
uint32		fileBytes;
int32		fullNameLen;
char		fullName[FILESYSTEM_MAX_NAME_LEN];

	strcpy(fullName, path);
	AppendPath(fullName, name, sizeof(fullName));
	fullNameLen = strlen(fullName);
	
	while ((info = CopyListItemInPath(ios, fullName)) != NULL) {
		result = QueryCopyObjVA(info->ci_CopyObj, 
			COPIER_TAG_DIRECTORYCOUNT,	&dirCount,
			COPIER_TAG_FILECOUNT,		&fileCount,
			COPIER_TAG_FILEBYTES,		&fileBytes,
			TAG_END);
		if (result == 0)
		{	
			ios->ios_CopyStats.dirCount	 -= dirCount;
			ios->ios_CopyStats.fileCount -= fileCount;
			ios->ios_CopyStats.fileBytes -= fileBytes;
		}
		RemNode((Node *)info);
		DeleteCopyObj(info->ci_CopyObj);
		FreeMemTrack(info);
	}
	
	if ((info = AllocMem(sizeof(CopyInfo) + fullNameLen, MEMTYPE_FILL|MEMTYPE_TRACKSIZE)) == NULL)
		return REQ_ERR_NOMEM;

	result = CreateCopyObjVA(&info->ci_CopyObj, path, name,
	   COPIER_TAG_USERDATA,                 ios,
	   COPIER_TAG_PROGRESSFUNC,             CopyProgressCallBack,
	   COPIER_TAG_NEEDSOURCEFUNC,           CopyNeedSourceCallBack,
	   COPIER_TAG_NEEDDESTINATIONFUNC,      CopyNeedDestinationCallBack,
	   COPIER_TAG_DUPLICATEFUNC,            CopyDuplicateCallBack,
	   COPIER_TAG_DESTINATIONFULLFUNC,      CopyDestinationFullCallBack,
	   COPIER_TAG_DESTINATIONPROTECTEDFUNC, CopyDestinationProtectedCallBack,
	   COPIER_TAG_READERRORFUNC,            CopyReadErrorCallBack,
	   COPIER_TAG_WRITEERRORFUNC,           CopyWriteErrorCallBack,
	   TAG_END);
	
	if (result == 0) 
	{
		result = QueryCopyObjVA(info->ci_CopyObj, 
			COPIER_TAG_ISREADCOMPLETE,	&readIsComplete,
			COPIER_TAG_DIRECTORYCOUNT,	&dirCount,
			COPIER_TAG_FILECOUNT,		&fileCount,
			COPIER_TAG_FILEBYTES,		&fileBytes,
			TAG_END);
		if (result == 0)
		{	
			ios->ios_CopyStats.readIsComplete	 = readIsComplete;
			ios->ios_CopyStats.dirCount			+= dirCount;
			ios->ios_CopyStats.fileCount		+= fileCount;
			ios->ios_CopyStats.fileBytes		+= fileBytes;
		}
	}
	
	if (result != 0) 
	{
		FreeMemTrack(info);
		return result;
	}
	
	strcpy(info->ci_FullName, fullName);
	info->ci_FullNameLen = fullNameLen;
	
	AddTail(&ios->ios_CopyList, (Node *)info);
	
	return 0;
}


/*****************************************************************************/


static void IOServerDaemon(IOServer *ios)
{
Err           result;
Item          msg;
ServerPacket *packet;
ServerStatus  status;

    msg = WaitPort(CURRENTTASK->t_DefaultMsgPort, 0);

    ios->ios_ServerAbortSig  = AllocSignal(0);
    ios->ios_ServerStatusMsg = result = CreateMsg(NULL, 0, CURRENTTASK->t_DefaultMsgPort);

    ReplyMsg(msg, result, NULL, 0);
    if (result < 0)
        return;

    PrepDirScanner(&ios->ios_DirScanner[0], (ScanProgressFunc)ScanProgressCallBack,
                                            (ScanNeedMediaFunc)ScanNeedMediaCallBack,
                                            (ScanErrorFunc)ScanErrorCallBack,
                                            ios->ios_Requester,
                                            ios);

    PrepDirScanner(&ios->ios_DirScanner[1], (ScanProgressFunc)ScanProgressCallBack,
                                            (ScanNeedMediaFunc)ScanNeedMediaCallBack,
                                            (ScanErrorFunc)ScanErrorCallBack,
                                            ios->ios_Requester,
                                            ios);

    while (TRUE)
    {
        msg    = WaitPort(ios->ios_ServerPort, 0);
        packet = (ServerPacket *)MESSAGE(msg)->msg_DataPtr;
        ReplyMsg(msg, 0, NULL, 0);

        if (!packet)
        {
            /* message without a packet tells us to exit */
            break;
        }

        ios->ios_ClientPacket = packet;

        switch (packet->sp_Action)
        {
            case SERVER_DELETE:
								result = DoDelete(ios, packet->sp_Path);
                                break;

            case SERVER_RENAME: result = DoRename(ios, packet->sp_Path, packet->sp_CopyName);
                                break;

            case SERVER_CREATEDIR: 
								result = DoCreateDir(ios, packet->sp_Path);
                                break;

            case SERVER_FORMAT: result = DoFormat(ios, (DeviceStack *)packet->sp_Path);
                                break;

            case SERVER_GETINFO: result = DoInfo(ios, packet->sp_Path, packet->sp_Info);
                                 break;

            case SERVER_SCAN  : result = ScanDirectory(&ios->ios_DirScanner[ios->ios_CurrentDirScanner],
                                                       packet->sp_Path);
                                status.ss_State              = SS_DIRSCANNED;
                                status.ss_Info.ss_DirScanner = &ios->ios_DirScanner[ios->ios_CurrentDirScanner];
                                ClientSend(ios, &status);

                                ios->ios_CurrentDirScanner = 1 - ios->ios_CurrentDirScanner;
                                ResetDirScanner(&ios->ios_DirScanner[ios->ios_CurrentDirScanner]);
                                break;

            case SERVER_START_COPY:
                                result = DoStartCopy(ios, packet->sp_Path, packet->sp_CopyName);
                                break;

            case SERVER_END_COPY: 
								result = DoEndCopy(ios, packet->sp_Path, packet->sp_data.sp_DeleteAfterCopy);
								break;
								  
            case SERVER_CANCEL_COPY: 
								result = DoCleanupCopy(ios);
                                break;
								
			case SERVER_QUERY_COPY:
								*packet->sp_data.sp_CopyStats = ios->ios_CopyStats;
								result = 0;
								break;
        }

        status.ss_State             = SS_DONE;
        status.ss_Info.ss_ErrorCode = result;
        ClientSend(ios, &status);
    }
    UnprepDirScanner(&ios->ios_DirScanner[0]);
    UnprepDirScanner(&ios->ios_DirScanner[1]);
    DoCleanupCopy(ios);
    DeleteMsg(ios->ios_ServerStatusMsg);
}


/*****************************************************************************/


Err CreateIOServer(IOServer **ios, StorageReq *req)
{
Item thread;
Item port;
Item msg;
Err  result;

    *ios = AllocMem(sizeof(IOServer), MEMTYPE_FILL);
    if (*ios)
    {
        (*ios)->ios_Requester = req;
		
		PrepList(&((*ios)->ios_CopyList));
		
        port = result = CreateMsgPort(NULL,0,0);
        if (port >= 0)
        {
            msg = result = CreateMsg(NULL,0,port);
            if (msg >= 0)
            {
                thread = result = CreateThreadVA(IOServerDaemon, "IO Server Daemon", 99, 4096,
                                      CREATETASK_TAG_ARGC,           *ios,
                                      CREATETASK_TAG_DEFAULTMSGPORT, 0,
                                      TAG_END);

                if (thread >= 0)
                {
                    (*ios)->ios_ServerThread = thread;
                    (*ios)->ios_ServerPort   = THREAD(thread)->t_DefaultMsgPort;
                    (*ios)->ios_ClientPort   = port;
                    (*ios)->ios_ClientMsg    = msg;
                    (*ios)->ios_ClientMsgPtr = MESSAGE(msg);

                    result = SendMsg(THREAD(thread)->t_DefaultMsgPort, msg, 0, 0);
                    if (result >= 0)
                    {
                        WaitPort(port,msg);
                        result = MESSAGE(msg)->msg_Result;
                    }

                    if (result >= 0)
                    {
                        return 0;
                    }
                    DeleteThread(thread);
                }
                DeleteMsg(msg);
            }
            DeleteMsgPort(port);
        }
        FreeMem(*ios, sizeof(IOServer));
    }
    else
    {
        result = REQ_ERR_NOMEM;
    }

    return result;
}


/*****************************************************************************/


Err DeleteIOServer(IOServer *ios)
{
    if (ios)
    {
        SendMsg(ios->ios_ServerPort, ios->ios_ClientMsg, NULL, 0);
        WaitPort(ios->ios_ClientPort, ios->ios_ClientMsg);

        while (LookupItem(ios->ios_ServerThread))
            WaitSignal(SIGF_DEADTASK);

        DeleteMsg(ios->ios_ClientMsg);
        DeleteMsgPort(ios->ios_ClientPort);
        FreeMem(ios, sizeof(IOServer));
    }

    return 0;
}


/*****************************************************************************/


Err StartIOService(IOServer *ios, const ServerPacket *packet)
{
Err result;

    result = SendMsg(ios->ios_ServerPort, ios->ios_ClientMsg, packet, sizeof(ServerPacket));
    if (result >= 0)
    {
        result = WaitPort(ios->ios_ClientPort, ios->ios_ClientMsg);
        if (result >= 0)
        {
            result = ios->ios_ClientMsgPtr->msg_Result;
        }
    }

    return result;
}


/*****************************************************************************/


Err StopIOService(IOServer *ios)
{
    return SendSignal(ios->ios_ServerThread, ios->ios_ServerAbortSig);
}
