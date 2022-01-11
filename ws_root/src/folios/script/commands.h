/* @(#) commands.h 96/04/19 1.2 */

#ifndef __COMMANDS_H
#define __COMMANDS_H


/****************************************************************************/


#include <misc/script.h>


/*****************************************************************************/


typedef struct ScriptCommand
{
    char  *sc_Name;
    Err  (*sc_Command)(ScriptContext *sc, char *args);
} ScriptCommand;

extern const ScriptCommand builtIns[];


/****************************************************************************/


#endif /* __COMMANDS_H */
