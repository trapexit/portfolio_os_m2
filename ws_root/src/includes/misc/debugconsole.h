#ifndef __MISC_DEBUGCONSOLE_H
#define __MISC_DEBUGCONSOLE_H


/******************************************************************************
**
**  @(#) debugconsole.h 95/09/14 1.1
**
**  Definitions for the debugging console library
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


/****************************************************************************/


typedef enum DebugConsoleTags
{
    DEBUGCONSOLE_TAG_HEIGHT = TAG_ITEM_LAST+1,
    DEBUGCONSOLE_TAG_TOP,
    DEBUGCONSOLE_TAG_TYPE
} DebugConsoleTags;


/****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


Err CreateDebugConsole(const TagArg *tags);
Err CreateDebugConsoleVA(uint32 tags, ...);
void DeleteDebugConsole(void);

void DebugConsolePrintf(const char *text, ...);
void DebugConsoleClear(void);
void DebugConsoleMove(uint32 x, uint32 y);


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __MISC_DEBUGCONSOLE_H */
