#ifndef __UI_REQUESTER_H
#define __UI_REQUESTER_H


/******************************************************************************
**
**  @(#) requester.h 96/02/22 1.9
**
**  Definitions for the high-level human interface folio.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __FILE_DIRECTORY_H
#include <file/directory.h>
#endif


/****************************************************************************/


/* kernel interface definitions */
#define REQ_FOLIONAME  "requester"


/****************************************************************************/


/* Error codes */

#define MakeReqErr(svr,class,err) MakeErr(ER_FOLI,ER_REQ,svr,ER_E_SSTM,class,err)

/* Bad pointer passed in */
#define REQ_ERR_BADPTR          MakeReqErr(ER_SEVERE,ER_C_STND,ER_BadPtr)

/* Unknown tag supplied */
#define REQ_ERR_BADTAG          MakeReqErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* No memory */
#define REQ_ERR_NOMEM           MakeReqErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* No signal bits available */
#define REQ_ERR_NOSIGNALS       MakeReqErr(ER_SEVERE,ER_C_STND,ER_NoSignals)

/* Invalid or corrupt requester pointer supplied to a function */
#define REQ_ERR_BADREQ          MakeReqErr(ER_SEVERE,ER_C_NSTND,1)

/* Invalid requester type */
#define REQ_ERR_BADTYPE         MakeReqErr(ER_SEVERE,ER_C_NSTND,2)

/* Ancilliary data file corrupt */
#define REQ_ERR_CORRUPTFILE     MakeReqErr(ER_SEVERE,ER_C_NSTND,3)

/* Bad option supplied */
#define REQ_ERR_BADOPTION       MakeReqErr(ER_SEVERE,ER_C_NSTND,4)

/* Supplied prompt sprite is too large */
#define REQ_ERR_PROMPTTOOBIG    MakeReqErr(ER_SEVERE,ER_C_NSTND,5)


/****************************************************************************/


/* a storage requester object */
typedef struct StorageReq StorageReq;

/* a function to decide whether or not a file/dir should be displayed to the user */
typedef bool (* FileFilterFunc)(const char *path, DirectoryEntry *de);

/* for use with CreateStorageReq(), ModifyStorageReq(), and QueryStorageReq() */
typedef enum StorageReqTags
{
    STORREQ_TAG_VIEW_LIST = TAG_ITEM_LAST+1, /* application view list        */
    STORREQ_TAG_FONT,                        /* font to use for rendering    */
    STORREQ_TAG_LOCALE,                      /* locale to use for rendering  */
    STORREQ_TAG_OPTIONS,                     /* see flag definitions below   */
    STORREQ_TAG_PROMPT,                      /* sprite to use for prompt     */
    STORREQ_TAG_FILTERFUNC,                  /* file filtering function      */

    /* initial values, and where query results are stored */
    STORREQ_TAG_FILE,                   /* file string                       */
    STORREQ_TAG_DIRECTORY               /* path string                       */
} StorageReqTags;


/* option flags for use with the STORREQ_TAG_OPTIONS tag */
#define STORREQ_OPTION_OK        (1 << 0)  /* put up an "OK" button          */
#define STORREQ_OPTION_LOAD      (1 << 1)  /* put up a "Load" button         */
#define STORREQ_OPTION_SAVE      (1 << 2)  /* put up a "Save" button         */
#define STORREQ_OPTION_CANCEL    (1 << 3)  /* put up a "Cancel" button       */
#define STORREQ_OPTION_EXIT      (1 << 4)  /* put up an "Exit" button        */
#define STORREQ_OPTION_QUIT      (1 << 5)  /* put up a "Quit" button         */
#define STORREQ_OPTION_DELETE    (1 << 6)  /* allow the user to delete files */
#define STORREQ_OPTION_MOVE      (1 << 7)  /* allow the user to move files   */
#define STORREQ_OPTION_COPY      (1 << 8)  /* allow the user to copy files   */
#define STORREQ_OPTION_RENAME    (1 << 9)  /* allow the user to rename files */
#define STORREQ_OPTION_CREATEDIR (1 << 10) /* allow the user to create dirs  */
#define STORREQ_OPTION_CHANGEDIR (1 << 11) /* allow the user to navigate     */


/*****************************************************************************/


/* Possible successful return values from DisplayStorageReq() */
typedef enum StorageReqResults
{
    STORREQ_CANCEL,   /* user cancelled   */
    STORREQ_OK        /* user selected ok */
} StorageReqResults;


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* folio management */
Err OpenRequesterFolio(void);
Err CloseRequesterFolio(void);

/* storage requester management */
Err CreateStorageReq(StorageReq **req, const TagArg *tags);
Err DeleteStorageReq(StorageReq *req);
Err DisplayStorageReq(StorageReq *req);
Err QueryStorageReq(StorageReq *req, const TagArg *tags);
Err ModifyStorageReq(StorageReq *req, const TagArg *tags);

/* varargs variants of some of the above */
Err CreateStorageReqVA(StorageReq **req, uint32 tags, ...);
Err QueryStorageReqVA(StorageReq *req, uint32 tags, ...);
Err ModifyStorageReqVA(StorageReq *req, uint32 tags, ...);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* __UI_REQUESTER_H */
