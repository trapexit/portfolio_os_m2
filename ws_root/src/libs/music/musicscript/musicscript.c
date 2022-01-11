/******************************************************************************
**
**  @(#) musicscript.c 95/11/27 1.14
**
**  Private script parser.
**
**  This module is the core part of the musicscript parser. The rest is in
**  other modules.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  951024 WJB  Created.
**  951025 WJB  Improved end of line handling to ignore comments that overrun line buffer.
**  951127 WJB  Made placement of diagnostic error messages consistent.
**  951127 WJB  Fragmented.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/musicerror.h>   /* error codes */
#include <file/fileio.h>        /* raw file functions */
#include <kernel/mem.h>
#include <ctype.h>              /* isspace() */
#include <string.h>

#include "musicscript_internal.h"


/* -------------------- local functions */

static int32 GetStreamLine (RawFile *, char *buf, int32 bufsize);
static char *TrimLine (char *);
static int32 GetArgs (char *, char *argv[], int32 argvnelts);


/* -------------------- functions */

/**
|||	AUTODOC -private -class libmusic -group MusicScript -name OpenMusicScript
|||	Opens a text file for parsing using a MusicScriptParser.
|||
|||	  Synopsis
|||
|||	    Err OpenMusicScript (MusicScriptParser **resultmsp, const char *fileName,
|||	                         int32 lineBufferSize, int32 argArrayElts)
|||
|||	  Description
|||
|||	    Opens a text file for parsing using a MusicScriptParser. Allocates a line
|||	    buffer and argument array. Call ReadMusicScriptCmd() to read lines from the
|||	    script.
|||
|||	  Arguments
|||
|||	    resultmsp
|||	        Buffer in which to write resulting MusicScriptParser pointer on success.
|||
|||	    fileName
|||	        Name of file to parse.
|||
|||	    lineBufferSize
|||	        The size of the line buffer to allocate. The maximum legal line limit
|||	        after EOL and comment stripping is lineBufferSize - 2.
|||
|||	    argArrayElts
|||	        Number of elements to allocate for the argument array. When filled out,
|||	        this array is NULL terminated, so the maximum number of arguments for a
|||	        script command is argArrayElts - 1.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, Err code on failure.
|||
|||	    *msp is set to pointer of allocated MusicScriptParser on success, NULL on
|||	    failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
|||
|||	  See Also
|||
|||	    ReadMusicScriptCmd(), ProcessMusicScriptCmd(), CloseMusicScript(),
|||	    ParseMusicScript()
**/
Err OpenMusicScript (MusicScriptParser **resultmsp, const char *fileName, int32 lineBufferSize, int32 argArrayElts)
{
    MusicScriptParser *msp;
    Err errcode;

        /* init result */
    *resultmsp = NULL;

        /* alloc/init MusicScriptParser + arg buffer + line buffer */
    if (!(msp = (MusicScriptParser *)AllocMem (sizeof *msp + argArrayElts * sizeof (char *) + lineBufferSize,
        MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {

        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    msp->msp_FileName = fileName;
    msp->msp_LineBufferSize = lineBufferSize;
    msp->msp_ArgArrayElts = argArrayElts;
    msp->msp_ArgArray = (char **)(msp + 1);
    msp->msp_LineBuffer = (char *)(msp->msp_ArgArray + argArrayElts);

        /* open file */
    if ((errcode = OpenRawFile (&msp->msp_File, fileName, FILEOPEN_READ)) < 0) goto clean;

        /* success: set result */
    *resultmsp = msp;
    return 0;

clean:
    CloseMusicScript (msp);
    return errcode;
}

/**
|||	AUTODOC -private -class libmusic -group MusicScript -name CloseMusicScript
|||	Closes script opened by OpenMusicScript().
|||
|||	  Synopsis
|||
|||	    void CloseMusicScript (MusicScriptParser *msp)
|||
|||	  Description
|||
|||	    Closes file and disposes of MusicScriptParser.
|||
|||	  Arguments
|||
|||	    msp
|||	        MusicScriptParser to dispose of. Can be NULL.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
|||
|||	  See Also
|||
|||	    OpenMusicScript()
**/
void CloseMusicScript (MusicScriptParser *msp)
{
    if (msp) {
        CloseRawFile (msp->msp_File);
        FreeMem (msp, TRACKED_SIZE);
    }
}

/**
|||	AUTODOC -private -class libmusic -group MusicScript -name ReadMusicScriptCmd
|||	Reads next non-trivial command from script.
|||
|||	  Synopsis
|||
|||	    int32 ReadMusicScriptCmd (MusicScriptParser *msp)
|||
|||	  Description
|||
|||	    Reads next non-trivial command from script. A non-trivial command is one
|||	    which has at least one argument (the command itself). Skips blank lines, and
|||	    lines which consist only of white space or comment.
|||
|||	    The resulting command is stored in the MusicScriptParser's msp_ArgArray[].
|||	    The contents of this array may be passed to ProcessMusicScriptCmd(), or
|||	    processed locally.
|||
|||	    This function prints its own error messages.
|||
|||	  Arguments
|||
|||	    msp
|||	        MusicScriptParser associated with script to read from.
|||
|||	  Return Value
|||
|||	    0
|||	        End of file. Nothing in msp_ArgArray.
|||
|||	    >0
|||	        msp_ArgArray has next command. This value is the same as msp_NumArgs.
|||
|||	    <0
|||	        Error code.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Caveats
|||
|||	    !!! MusicScriptParser structures is currently private, so there isn't any
|||	    way to get at msp_ArgArray.
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
|||
|||	  See Also
|||
|||	    OpenMusicScript(), ProcessMusicScriptCmd()
**/
int32 ReadMusicScriptCmd (MusicScriptParser *msp)
{
    int32 result;

        /* loop on lines until we get a command with >0 args */
    do {
            /* get next line */
        const int32 linelen = GetStreamLine (msp->msp_File, msp->msp_LineBuffer, msp->msp_LineBufferSize);

            /* bump line count if not EOF */
        if (linelen != 0) msp->msp_LineNum++;

            /* if EOF or error, return */
        if ((result = linelen) <= 0) goto clean;

            /* trim line at comment. also discards trailing EOL character, if there is one. */
        TrimLine (msp->msp_LineBuffer);

      #if DEBUG_Parser
        printf ("%d: %4d '%s'\n", msp->msp_LineNum, linelen, msp->msp_LineBuffer);
      #endif

            /*
                Trap line too long: if length of buffered line after trimming
                is maximum length (it's OK to truncate a comment, but not bona
                fide script text).

                @@@ this means that the maximum legal line length is 1 less than can fit in
                    the buffer. Think of it as reserving one byte for the '\0' and one
                    byte for the comment character or new line character.

                @@@ if the line has nothing but white space before EOL or
                    comment, but EOL or comment char didn't fit in buffer, this
                    is still considered line too long. Would need to be a lot
                    smarter about handling comment characters outside of buffer
                    to be able to handle this more gracefully.
            */
        if (strlen (msp->msp_LineBuffer) == msp->msp_LineBufferSize-1) {
            result = ML_ERR_LINE_TOO_LONG;
            goto clean;
        }

        /* break into words, break if non-zero result from GetArgs() (either num args or error code) */
    } while (!(result = msp->msp_NumArgs = GetArgs (msp->msp_LineBuffer, msp->msp_ArgArray, msp->msp_ArgArrayElts)));

clean:
        /* print error message if result < 0 */
    if (result < 0) MSERR((msp, result, NULL));
    return result;
}


/* -------------------- RawFile processing */

static int32 GetStreamChar (RawFile *, char *s);

/*
    Reads next line from RawFile until EOL or EOF is found. Stores as many
    characters as will fit into the buffer, leaving room to '\0' terminate the
    buffer. Any characters that overflow the buffer are discarded. The '\n', if
    there is one before EOF, is stored in the buffer if there is room.

    '\r' is automatically translated to '\n'.

    Arguments
        stream
            file created by OpenRawFile().

        buf
            Pointer to client buffer.

        bufsize
            sizeof client buffer. This size includes the NULL termination.

    Results
        >0
            number of characters in line including '\n' character if there was
            one. This indicates the true length of the line and can be >=
            bufsize. No more than bufsize-1 characters, plus '\0' are written
            to buf, however. The difference in lengths is an indication that
            a line was truncated.

        0
            end of file was reached (there wasn't a single character left
            in the file to place in the buffer).

        <0
            error code
*/
static int32 GetStreamLine (RawFile *stream, char *buf, int32 bufsize)
{
    int32 nchars = 0;
    char *s = buf;
    int32 result;
    char c;

    for (;;) {
            /* get next character from file, return on error */
        if ((result = GetStreamChar (stream, &c)) < 0) return result;

            /* on EOF, simply leave loop */
        if (!result) break;

            /* increment # of characters read. store in buffer if there's
               enough room (leaving space for '\0') */
        if (nchars++ < bufsize-1) *s++ = c;

            /* leave loop on EOL */
        if (c == '\n') break;
    }
    *s = '\0';                  /* terminate string */

    return nchars;
}

/*
    Read next character from RawFile. Does cheap mac EOL character translation.

    Arguments
        stream
            RawFile to skip characters on.

        s
            Buffer to write character.

    Results
        >0
            Read a character.

        0
            EOF. No character read.

        <0
            Err code on failure.
*/
static int32 GetStreamChar (RawFile *stream, char *s)
{
    char c;
    int32 actual;

    if ((actual = ReadRawFile (stream, &c, 1)) > 0) {
        if (c == '\r') c = '\n';    /* cheap conversion of Mac EOL (@@@ doesn't do PC EOL) */
        *s = c;
    }
    return actual;
}


/* -------------------- command line arg processing */

static char *breakword (char *s, char **newp);
static char *skipspace (char *s);
static char *skipnspace (char *s);

/*
    Break string at first instance of new line, pound sign, or semicolon
    by writing '\0' there. Returns original buffer pointer.
*/
static char *TrimLine (char *buf)
{
    char *s;

        /* replace first occurance of EOL or comment char w/ '\0' */
    if ((s = strpbrk(buf, ";#\n")) != NULL) *s = '\0';

    return buf;
}

/*
    Build NULL-terminated argv[] from source string. Breaks original string bufffer.

    Arguments
        s
            String to parse

        argv
            Arg array buffer to fill out

        argvnelts
            Number of elements in argv, including space for NULL termination.

    Results
        Returns number of args (not including NULL termination) or error code
        if too many args.

        Overwrites white space in original string with '\0' to make separate
        strings.

        argv[] and original string contents undefined when this function
        returns an error.
*/
static int32 GetArgs (char *s, char *argv[], int32 argvnelts)
{
    char ** const endargp = &argv[argvnelts];   /* pointer to just past end of array */
    char **argp;

        /* loop until space exhausted or no more words in s */
    for (argp = argv; argp < endargp; argp++) {
        if (!(*argp = breakword (s, &s))) return argp - argv;
    }
    return ML_ERR_TOO_MANY_ARGS;
}

/*
    break line into words

    Arguments
        s
            Pointer to character to begin string search. Can point to any
            character in a string including the null termination. Must not be
            NULL.

        newp
            Pointer to variable to store value of s for next pass. Can point to
            same variable containing s. The result is never NULL, but could
            point to white space or the end of the string.

    Results
        Pointer to beginning of a word or NULL if no more found. Never returns
        a pointer to the end of the string or white space.

    @@@ could be extended to support quotes (although, comment
        location would then have to occur here instead of separate)
*/
static char *breakword (char *s, char **newp)
{
    char *wordp;

        /* skip leading white space */
    s = skipspace(s);

        /* get word pointer, if there is one */
    wordp = *s ? s : NULL;

        /* locate end of word (does nothing if already at end of string) */
    s = skipnspace(s);

        /* if not at end of string, break string and get pointer to next character */
    if (*s) *s++ = '\0';

        /* set pointer to current string location for next call to breakword(). */
    *newp = s;

    return wordp;
}

/*
    Return pointer to first non-whitespace character in string,
    or end of string if none found.
*/
static char *skipspace (char *s)
{
    while (*s && isspace(*s)) s++;
    return s;
}

/*
    Return pointer to first whitespace character in string,
    or end of string if none found.
*/
static char *skipnspace (char *s)
{
    while (*s && !isspace(*s)) s++;
    return s;
}
