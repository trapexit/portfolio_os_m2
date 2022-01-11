/* @(#) ioserver.h 96/10/30 1.4 */

#ifndef __IOSERVER_H
#define __IOSERVER_H


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __DIRSCAN_H
#include "dirscan.h"
#endif


/*****************************************************************************/


typedef struct IOServer IOServer;

/* things the server can do */
typedef enum
{
    SERVER_SCAN,
    SERVER_START_COPY,
    SERVER_END_COPY,
    SERVER_DELETE,
    SERVER_RENAME,
    SERVER_CREATEDIR,
    SERVER_FORMAT,
    SERVER_GETINFO,
	SERVER_CANCEL_COPY,
	SERVER_QUERY_COPY
} ServerCommands;

/* info obtained from a CopyObj */

typedef struct 
{
	uint32	readIsComplete;
	uint32	dirCount;
	uint32	fileCount;
	uint32	fileBytes;
} CopyStats;

/* argument block passed to the server to get it to do something */
typedef struct
{
    ServerCommands  sp_Action;
    char           *sp_Path;
    union
    {
        char       *sp_copyname;
        EntryInfo  *sp_info;
		CopyStats  *sp_CopyStats;
		bool		sp_DeleteAfterCopy;
    } sp_data;
    Item            sp_StatusPort;
} ServerPacket;

#define sp_CopyName sp_data.sp_copyname
#define sp_Info     sp_data.sp_info

/* state of the server */
typedef enum
{
    SS_DONE,
    SS_DIRSCANNED,
    SS_ERROR,
    SS_NEED_MEDIA,
    SS_MEDIA_PROTECTED,
    SS_MEDIA_FULL,
    SS_DUPLICATE_FILE,
	SS_HIERARCHY_COPY	/* attempt to copy item into its child hierarchy */
} ServerStates;

/* information supplied to the status port by the server as it does I/O */
typedef struct
{
    ServerStates ss_State;

    union
    {
        Err         ss_ErrorCode;
        const char *ss_MediaSought;
        const char *ss_MediaProtected;
        const char *ss_MediaFull;
        const char *ss_DuplicateFile;
		const char *ss_HierarchyName;
        DirScanner *ss_DirScanner;
    } ss_Info;
} ServerStatus;


#define SERVER_ABORTED 2

/*****************************************************************************/


Err  CreateIOServer(IOServer **ios, struct StorageReq *req);
Err  DeleteIOServer(IOServer *ios);
Err  StartIOService(IOServer *ios, const ServerPacket *packet);
Err  StopIOService(IOServer *ios);
bool NameIsInCopyList(IOServer *ios, const char *name);

/*****************************************************************************/


#endif /* __IOSERVER_H */
