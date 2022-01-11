#ifndef __MUSICSCRIPT_INTERNAL_H
#define __MUSICSCRIPT_INTERNAL_H


/****************************************************************************
**
**  @(#) musicscript_internal.h 95/11/27 1.2
**
**  musicscript internals
**
****************************************************************************/


#ifndef __AUDIO_MUSICSCRIPT_H
#include <audio/musicscript.h>  /* 'public' part */
#endif

#ifndef __FILE_FILEIO_H
#include <file/fileio.h>        /* RawFile */
#endif


/* -------------------- Debug control */

#define DEBUG_Parser 0  /* debug workings of Music Script Parser */

#if DEBUG_Parser
#include <stdio.h>
#endif


/* -------------------- Internal structures */

struct MusicScriptParser {
        /* file */
    const char *msp_FileName;       /* file name */
    RawFile *msp_File;              /* open file handle */

        /* line buffer */
    int32   msp_LineBufferSize;
    char   *msp_LineBuffer;
    int32   msp_LineNum;            /* line number of last line read by ReadParserFileLine(). Set to 0 by OpenParserFile(). */

        /* arg array */
    int32   msp_ArgArrayElts;       /* number of elements in msp_ArgArray */
    char  **msp_ArgArray;
    int32   msp_NumArgs;            /* number of args currently in ArgArray */
};


/*****************************************************************************/


#endif /* __MUSICSCRIPT_INTERNAL_H */
