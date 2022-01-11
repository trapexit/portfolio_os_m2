#ifndef __AUDIO_COBJ_H
#define __AUDIO_COBJ_H


/****************************************************************************
**
**  @(#) cobj.h 96/01/03 1.7
**  $Id: cobj.h,v 1.12 1994/09/10 00:17:48 peabody Exp $
**
**  CObject support
**
****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#define VALID_OBJECT_KEY  (0xABCD4321)
#define COBJ_ERR_NO_MEM (-1)
#define COBJ_ERR_NO_METHOD (-2)
#define COBJ_ERR_DATA_SIZE (-3)
#define COBJ_ERR_NULL_OBJECT (-4)

typedef struct COBClass
{
	struct  COBClass *Super;		/* Superclass */
	int32    DataSize;				/* Size of an object of this class */
	int32    (*Init)( void * );
	int32    (*Term)( void * );
	int32    (*Print)( void * );
	int32    (*SetInfo)( void *, TagArg * );
	int32    (*GetInfo)( void *, TagArg * );
	int32    (*Alloc)( void *, int32 );
	int32    (*Free)( void * );
	int32    (*Add)( void *, void *, int32 );
	int32    (*Clear)( void * );
	int32    (*GetNthFrom)( void *, int32, void ** );
	int32    (*RemoveNthFrom)( void *, int32 );
	int32    (*Start)( void *, uint32, int32, void * );
	int32    (*Stop)( void *, uint32 );
	int32    (*Bump)( void * );
	int32    (*Rewind)( void *, uint32 );
	int32    (*Pause)( void *, uint32 );
	int32    (*Unpause)( void *, uint32 );
	int32    (*Abort)( void *, uint32 );
	int32    (*Finish)( void *, uint32 );
	int32    (*Done)( void *, uint32, void * );
} COBClass;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PrintObject(obj) obj->Class->Print(obj)
#define SetObjectInfo(obj,tags) obj->Class->SetInfo(obj,tags)
#define GetObjectInfo(obj,tags) obj->Class->GetInfo(obj,tags)
#define StartObject(obj,time,nrep,par) obj->Class->Start(obj,time,nrep,par)
#define StopObject(obj,time) obj->Class->Stop(obj,time)
#define AbortObject(obj,time) obj->Class->Abort(obj,time)
#define AllocObject(obj,n) obj->Class->Alloc(obj,n)
#define FreeObject(obj) obj->Class->Free(obj)
#define GetNthFromObject(obj,n,ptr) obj->Class->GetNthFrom(obj,n,ptr)
#define RemoveNthFromObject(obj,n) obj->Class->RemoveNthFrom(obj,n)

#define COBObjectIV \
	Node      COBNode; \
	COBClass  *Class;  \
	uint32    cob_ValidationKey

typedef struct COBObject
{
	COBObjectIV;
} COBObject;

int32 DefineClass( COBClass *Class, COBClass *SuperClass, int32 DataSize);
COBObject *CreateObject( COBClass *Class);
int32 DestroyObject( COBObject *Object );
int32 ValidateObject( COBObject *cob );

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AUDIO_COBJ_H */
