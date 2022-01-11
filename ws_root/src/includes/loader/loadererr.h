#ifndef __LOADER_LOADERERR_H
#define __LOADER_LOADERERR_H


/******************************************************************************
**
**  @(#) loadererr.h 96/06/06 1.21
**
******************************************************************************/


#ifndef EXTERNAL_RELEASE
#ifdef LOAD_FOR_UNIX
#define MAKELDERR(svr,class,err) -42
#else
#include <kernel/operror.h>
#define MAKELDERR(svr,class,err) MakeLErr(ER_LOADER,svr,class,err)
#endif
#else
#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


#define MAKELDERR(svr,class,err) MakeLErr(ER_LOADER,svr,class,err)
#endif

#define LOADER_ERR_NOMEM      MAKELDERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define LOADER_ERR_BADPRIV    MAKELDERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define LOADER_ERR_NOSUPPORT  MAKELDERR(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define LOADER_ERR_BADTAG     MAKELDERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define LOADER_ERR_BADTAGVAL  MAKELDERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define LOADER_ERR_BADITEM    MAKELDERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define LOADER_ERR_NOTFOUND   MAKELDERR(ER_SEVERE,ER_C_STND,ER_NotFound)
#define LOADER_ERR_BADPTR     MAKELDERR(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define LOADER_ERR_BADNAME    MAKELDERR(ER_SEVERE,ER_C_STND,ER_BadName)
#define LOADER_ERR_BADFILE    MAKELDERR(ER_SEVERE,ER_C_NSTND,1)
#define LOADER_ERR_RELOC      MAKELDERR(ER_SEVERE,ER_C_NSTND,2)
#define LOADER_ERR_SYM        MAKELDERR(ER_SEVERE,ER_C_NSTND,3)
#define LOADER_ERR_RSA        MAKELDERR(ER_SEVERE,ER_C_NSTND,4)
#define LOADER_ERR_BADSECTION MAKELDERR(ER_SEVERE,ER_C_NSTND,5)
#define LOADER_ERR_NOIMPORT   MAKELDERR(ER_SEVERE,ER_C_NSTND,6)
#define LOADER_ERR_IMPRANGE   MAKELDERR(ER_SEVERE,ER_C_NSTND,7)


/*****************************************************************************/


#endif /* __LOADER_LOADERERR_H */
