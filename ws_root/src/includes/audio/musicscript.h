#ifndef __AUDIO_MUSICSCRIPT_H
#define __AUDIO_MUSICSCRIPT_H


/****************************************************************************
**
**  @(#) musicscript.h 96/02/13 1.4
**
**  Private script parser used by a number of audio things.
**
****************************************************************************/


#ifdef EXTERNAL_RELEASE
#error This file may not be used in externally released source code.
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* -------------------- Types */

typedef struct MusicScriptParser MusicScriptParser;
typedef Err (*MusicScriptCmdCallback) (const MusicScriptParser *, void *userData, int argc, const char * const argv[]);

typedef struct MusicScriptCmdInfo {
    const char *msci_Cmd;                   /* command name (note: case insensitive) */
    int32       msci_MinNumArgs;            /* min argc value (includes command name) */
    MusicScriptCmdCallback msci_Callback;   /* callback to implement the command */
} MusicScriptCmdInfo;


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


Err OpenMusicScript (MusicScriptParser **resultmsp, const char *fileName, int32 lineBufferSize, int32 argArrayElts);
void CloseMusicScript (MusicScriptParser *);
int32 ReadMusicScriptCmd (MusicScriptParser *);
Err ProcessMusicScriptCmd (const MusicScriptParser *, const MusicScriptCmdInfo *cmdTable, void *userData);

    /* convenience */
Err ParseMusicScript (const char *fileName, int32 lineBufferSize, int32 argArrayElts, const MusicScriptCmdInfo *cmdTable, void *userData);

    /* diagnostics */
#ifdef BUILD_STRINGS
    void PrintMusicScriptError (const MusicScriptParser *, Err errcode, const char *msgfmt, ...);
    #define MSERR(x) PrintMusicScriptError x
#else
    #define MSERR(x)
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_MUSICSCRIPT_H */
