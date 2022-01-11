/* @(#) msgstrings.h 96/10/30 1.6 */

#ifndef __MSGSTRINGS_H
#define __MSGSTRINGS_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


typedef enum
{
    MSG_OK_NUM,
    MSG_LOAD_NUM,
    MSG_SAVE_NUM,
    MSG_CANCEL_NUM,
    MSG_EXIT_NUM,
    MSG_QUIT_NUM,
    MSG_DELETE_NUM,
    MSG_COPY_NUM,
    MSG_MOVE_NUM,
    MSG_RENAME_NUM,
    MSG_CREATEDIR_NUM,
    MSG_FORMAT_NUM,

    MSG_OVL_DELETE_NUM,
    MSG_OVL_FORMAT_NUM,
    MSG_OVL_CANCEL_NUM,
    MSG_OVL_STOP_NUM,
    MSG_OVL_OK_NUM,

    MSG_OVL_CONFIRMDELETEFILE_NUM,
    MSG_OVL_CONFIRMDELETEDIR_NUM,
    MSG_OVL_CONFIRMDELETEFSYS_NUM,
    MSG_OVL_CONFIRMFORMAT_NUM,
    MSG_OVL_ERROR_NUM,
    MSG_OVL_INFO_NUM,
    MSG_OVL_DUPLICATE_NUM,
    MSG_OVL_MEDIAFULL_NUM,
    MSG_OVL_MEDIAPROTECTED_NUM,
    MSG_OVL_MEDIAREQUEST_NUM,
    MSG_OVL_WORKING_NUM,

    MSG_OVL_CONFIRMDELETE_TITLE_NUM,
    MSG_OVL_CONFIRMFORMAT_TITLE_NUM,
    MSG_OVL_ERROR_TITLE_NUM,
    MSG_OVL_INFO_TITLE_NUM,
    MSG_OVL_DUPLICATE_TITLE_NUM,
    MSG_OVL_MEDIAFULL_TITLE_NUM,
    MSG_OVL_MEDIAPROTECTED_TITLE_NUM,
    MSG_OVL_MEDIAREQUEST_TITLE_NUM,
    MSG_OVL_WORKING_TITLE_NUM,

    MSG_DEFAULT_FS_NAME_NUM,

    MSG_INFO_NAME_NUM,
    MSG_INFO_SIZE_NUM,
    MSG_INFO_CREATED_BY_NUM,
    MSG_INFO_LAST_MODIFIED_NUM,
    MSG_INFO_VERSION_NUM,
    MSG_INFO_CREATED_NUM,
    MSG_INFO_STATUS_NUM,
    MSG_INFO_CAPACITY_NUM,
    MSG_INFO_WRITABLE_NUM,
    MSG_INFO_READ_ONLY_NUM,
    MSG_INFO_AMOUNT_NUM,

    MSG_TEXT_SHIFT_NUM,
    MSG_TEXT_CLEAR_NUM,
    MSG_TEXT_BACKSPACE_NUM,

	MSG_OVL_CONFIRMCOPY_TITLE_NUM,
	MSG_OVL_CONFIRMCOPY_NUM,
	MSG_OVL_HIERARCHY_COPY_TITLE_NUM,
	MSG_OVL_HIERARCHY_COPY_NUM,
	
    /* must always be last */
    NUM_STRINGS
} MsgStrings;


#define MSG_OK                       GetString(req, MSG_OK_NUM)
#define MSG_LOAD                     GetString(req, MSG_LOAD_NUM)
#define MSG_SAVE                     GetString(req, MSG_SAVE_NUM)
#define MSG_CANCEL                   GetString(req, MSG_CANCEL_NUM)
#define MSG_EXIT                     GetString(req, MSG_EXIT_NUM)
#define MSG_QUIT                     GetString(req, MSG_QUIT_NUM)
#define MSG_DELETE                   GetString(req, MSG_DELETE_NUM)
#define MSG_COPY                     GetString(req, MSG_COPY_NUM)
#define MSG_MOVE                     GetString(req, MSG_MOVE_NUM)
#define MSG_RENAME                   GetString(req, MSG_RENAME_NUM)
#define MSG_CREATEDIR                GetString(req, MSG_CREATEDIR_NUM)
#define MSG_FORMAT                   GetString(req, MSG_FORMAT_NUM)

#define MSG_OVL_DELETE               GetString(req, MSG_OVL_DELETE_NUM)
#define MSG_OVL_CREATE               GetString(req, MSG_OVL_CREATE_NUM)
#define MSG_OVL_RENAME               GetString(req, MSG_OVL_RENAME_NUM)
#define MSG_OVL_FORMAT               GetString(req, MSG_OVL_FORMAT_NUM)
#define MSG_OVL_CANCEL               GetString(req, MSG_OVL_CANCEL_NUM)
#define MSG_OVL_STOP                 GetString(req, MSG_OVL_STOP_NUM)
#define MSG_OVL_OK                   GetString(req, MSG_OVL_OK_NUM)

#define MSG_OVL_CONFIRMDELETEFILE    GetString(req, MSG_OVL_CONFIRMDELETEFILE_NUM)
#define MSG_OVL_CONFIRMDELETEDIR     GetString(req, MSG_OVL_CONFIRMDELETEDIR_NUM)
#define MSG_OVL_CONFIRMDELETEFSYS    GetString(req, MSG_OVL_CONFIRMDELETEFSYS_NUM)
#define MSG_OVL_CONFIRMFORMAT        GetString(req, MSG_OVL_CONFIRMFORMAT_NUM)
#define MSG_OVL_ERROR                GetString(req, MSG_OVL_ERROR_NUM)
#define MSG_OVL_INFO                 GetString(req, MSG_OVL_INFO_NUM)
#define MSG_OVL_DUPLICATE            GetString(req, MSG_OVL_DUPLICATE_NUM)
#define MSG_OVL_MEDIAFULL            GetString(req, MSG_OVL_MEDIAFULL_NUM)
#define MSG_OVL_MEDIAPROTECTED       GetString(req, MSG_OVL_MEDIAPROTECTED_NUM)
#define MSG_OVL_MEDIAREQUEST         GetString(req, MSG_OVL_MEDIAREQUEST_NUM)
#define MSG_OVL_WORKING              GetString(req, MSG_OVL_WORKING_NUM)

#define MSG_OVL_CONFIRMDELETE_TITLE  GetString(req, MSG_OVL_CONFIRMDELETE_TITLE_NUM)
#define MSG_OVL_CONFIRMFORMAT_TITLE  GetString(req, MSG_OVL_CONFIRMFORMAT_TITLE_NUM)
#define MSG_OVL_ERROR_TITLE          GetString(req, MSG_OVL_ERROR_TITLE_NUM)
#define MSG_OVL_INFO_TITLE           GetString(req, MSG_OVL_INFO_TITLE_NUM)
#define MSG_OVL_DUPLICATE_TITLE      GetString(req, MSG_OVL_DUPLICATE_TITLE_NUM)
#define MSG_OVL_MEDIAFULL_TITLE      GetString(req, MSG_OVL_MEDIAFULL_TITLE_NUM)
#define MSG_OVL_MEDIAPROTECTED_TITLE GetString(req, MSG_OVL_MEDIAPROTECTED_TITLE_NUM)
#define MSG_OVL_MEDIAREQUEST_TITLE   GetString(req, MSG_OVL_MEDIAREQUEST_TITLE_NUM)
#define MSG_OVL_WORKING_TITLE        GetString(req, MSG_OVL_WORKING_TITLE_NUM)

#define MSG_DEFAULT_FS_NAME          GetString(req, MSG_DEFAULT_FS_NAME_NUM)

#define MSG_INFO_NAME                GetString(req, MSG_INFO_NAME_NUM)
#define MSG_INFO_SIZE                GetString(req, MSG_INFO_SIZE_NUM)
#define MSG_INFO_CREATED_BY          GetString(req, MSG_INFO_CREATED_BY_NUM)
#define MSG_INFO_LAST_MODIFIED       GetString(req, MSG_INFO_LAST_MODIFIED_NUM)
#define MSG_INFO_VERSION             GetString(req, MSG_INFO_VERSION_NUM)
#define MSG_INFO_CREATED             GetString(req, MSG_INFO_CREATED_NUM)
#define MSG_INFO_STATUS              GetString(req, MSG_INFO_STATUS_NUM)
#define MSG_INFO_CAPACITY            GetString(req, MSG_INFO_CAPACITY_NUM)
#define MSG_INFO_WRITABLE            GetString(req, MSG_INFO_WRITABLE_NUM)
#define MSG_INFO_READ_ONLY           GetString(req, MSG_INFO_READ_ONLY_NUM)
#define MSG_INFO_AMOUNT              GetString(req, MSG_INFO_AMOUNT_NUM)

#define MSG_TEXT_SHIFT               GetString(req, MSG_TEXT_SHIFT_NUM)
#define MSG_TEXT_CLEAR               GetString(req, MSG_TEXT_CLEAR_NUM)
#define MSG_TEXT_BACKSPACE           GetString(req, MSG_TEXT_BACKSPACE_NUM)

#define MSG_OVL_CONFIRMCOPY_TITLE    GetString(req, MSG_OVL_CONFIRMCOPY_TITLE_NUM)
#define MSG_OVL_CONFIRMCOPY          GetString(req, MSG_OVL_CONFIRMCOPY_NUM)

#define MSG_OVL_HIERARCHY_COPY_TITLE GetString(req, MSG_OVL_HIERARCHY_COPY_TITLE_NUM)
#define MSG_OVL_HIERARCHY_COPY		 GetString(req, MSG_OVL_HIERARCHY_COPY_NUM)

/*****************************************************************************/


void  LoadStrings(struct StorageReq *req);
void  UnloadStrings(struct StorageReq *req);
char *GetString(struct StorageReq *req, uint32 stringNum);


/*****************************************************************************/


#endif /* __MSGSTRINGS_H */
