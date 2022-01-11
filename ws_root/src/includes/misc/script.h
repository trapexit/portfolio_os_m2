#ifndef __MISC_SCRIPT_H
#define __MISC_SCRIPT_H


/******************************************************************************
**
**  @(#) script.h 95/10/20 1.6
**
**  Script execution management.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


/*****************************************************************************/


/* kernel interface definitions */
#define SCRIPT_FOLIONAME "script"


/*****************************************************************************/


#ifdef EXTERNAL_RELEASE
typedef struct ScriptContext ScriptContext;
#else
typedef struct ScriptContext
{
    bool   sc_DefaultBackgroundMode;
    bool   sc_LumberjackCreated;
    bool   sc_MustBeSigned;
    uint32 sc_EventsLogged;
} ScriptContext;
#endif


/*****************************************************************************/


/* controls the execution environment of scripts */
typedef enum ScriptTags
{
    SCRIPT_TAG_CONTEXT = TAG_ITEM_LAST+1,
    SCRIPT_TAG_BACKGROUND_MODE,
    SCRIPT_TAG_MUST_BE_SIGNED,
} ScriptTags;


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* folio management */
Err OpenScriptFolio(void);
Err CloseScriptFolio(void);

Err CreateScriptContext(ScriptContext **sc, const TagArg *tags);
Err DeleteScriptContext(ScriptContext *sc);
Err ExecuteCmdLine(const char *scriptName, int32 *pStatus, const TagArg *tags);

/* varargs variants of some of the above */
Err CreateScriptContextVA(ScriptContext **sc, uint32 tag, ...);
Err ExecuteCmdLineVA(const char *scriptName, int32 *pStatus, uint32 tag, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __MISC_SCRIPT_H */
