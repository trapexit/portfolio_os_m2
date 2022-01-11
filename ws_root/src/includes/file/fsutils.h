#ifndef __FILE_FSUTILS_H
#define __FILE_FSUTILS_H


/******************************************************************************
**
**  @(#) fsutils.h 96/10/28 1.5
**
**  File system utility routines.
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


/*****************************************************************************/


/* a file copy object */
typedef struct CopyObj CopyObj;

/* definition of callback functions used by the copier */
typedef Err (* CopyProgressFunc)(void *userData, const char *path);
typedef Err (* CopyNeedSourceFunc)(void *userData, const char *fsName);
typedef Err (* CopyNeedDestinationFunc)(void *userData, const char *fsName);
typedef Err (* CopyDuplicateFunc)(void *userData, const char *path);
typedef Err (* CopyDestinationFullFunc)(void *userData, const char *fsName);
typedef Err (* CopyDestinationProtectedFunc)(void *userData, const char *fsName);
typedef Err (* CopyReadErrorFunc)(void *userData, const char *path, Err error);
typedef Err (* CopyWriteErrorFunc)(void *userData, const char *path, Err error);

typedef enum CopierTags
{
    COPIER_TAG_USERDATA = TAG_ITEM_LAST+1,
    COPIER_TAG_MEMORYTHRESHOLD,
    COPIER_TAG_ASYNCHRONOUS,

    COPIER_TAG_PROGRESSFUNC,
    COPIER_TAG_NEEDSOURCEFUNC,
    COPIER_TAG_NEEDDESTINATIONFUNC,
    COPIER_TAG_DUPLICATEFUNC,
    COPIER_TAG_DESTINATIONFULLFUNC,
    COPIER_TAG_DESTINATIONPROTECTEDFUNC,
    COPIER_TAG_READERRORFUNC,
    COPIER_TAG_WRITEERRORFUNC,
	
	COPIER_TAG_DIRECTORYCOUNT,	/* tags for query... */
	COPIER_TAG_FILECOUNT,
	COPIER_TAG_FILEBYTES,
	COPIER_TAG_ISREADCOMPLETE
} CopierTags;


/*****************************************************************************/


/* definition of callback functions used by the deleter */
typedef Err (* DeleteProgressFunc)(void *userData, const char *path);
typedef Err (* DeleteNeedMediaFunc)(void *userData, const char *fsName);
typedef Err (* DeleteMediaProtectedFunc)(void *userData, const char *fsName);
typedef Err (* DeleteErrorFunc)(void *userData, const char *path, Err error);

typedef enum DeleterTags
{
    DELETER_TAG_USERDATA = TAG_ITEM_LAST+1,
    DELETER_TAG_PROGRESSFUNC,
    DELETER_TAG_NEEDMEDIAFUNC,
    DELETER_TAG_MEDIAPROTECTEDFUNC,
    DELETER_TAG_ERRORFUNC
} DeleterTags;


/*****************************************************************************/


/* Error codes */
#define MakeFSUtilsErr(svr,class,err) MakeErr(ER_FOLI,ER_FSUTILS,svr,ER_E_SSTM,class,err)

/* No memory */
#define FSUTILS_ERR_NOMEM    MakeFSUtilsErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* Unknown tag supplied */
#define FSUTILS_ERR_BADTAG   MakeFSUtilsErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* Bad pointer supplied */
#define FSUTILS_ERR_BADPTR   MakeFSUtilsErr(ER_SEVERE,ER_C_STND,ER_BadPtr)

/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* folio management */
Err OpenFSUtilsFolio(void);
Err CloseFSUtilsFolio(void);

/* directory tree copy engine */
Err CreateCopyObj(CopyObj **co, const char *sourceDir, const char *objName, const TagArg *tags);
Err CreateCopyObjVA(CopyObj **co, const char *sourceDir, const char *objName, uint32 tag, ...);
Err DeleteCopyObj(CopyObj *co);
Err PerformCopy(CopyObj *co, const char *destinationDir);
Err QueryCopyObj(CopyObj *co, const TagArg *tags);
Err QueryCopyObjVA(CopyObj *co, uint32 tag, ...);

/* directory tree deletion engine */
Err DeleteTree(const char *path, const TagArg *tags);
Err DeleteTreeVA(const char *path, uint32 tag, ...);

/* pathname utilities */
Err AppendPath(char *path, const char *append, uint32 numBytes);
char *FindFinalComponent(const char *path);
Err GetPath(Item file, char *path, uint32 numBytes);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* FILE_FSUTILS_H */
