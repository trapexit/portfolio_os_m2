/*
 * file.c -- handles the file stack
 */
#include "parsertypes.h"
#include "parsererrors.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 16384
#define FILE_DEPTH 16
#define FILENAME_SIZE 32
#define KNIT_OVERLAP 64

#define DBUG_FS 0
#if (DBUG_FS)
#define DPRTFS(x) printf x;
#else
#define DPRTFS(x)
#endif

typedef struct fileStack
{
    char filename[FILENAME_SIZE];
    FILE *file;
    char *buffer;
    char *readPos;
    uint32 lineNumber;
} fileStack;

fileStack filestack[FILE_DEPTH];
int32 fileDepth;
Err IncludeFile(char *filename, char *currentBuff, char **newBuffer, FILE **newFile);
Err HandleEOF(char **buffer);
char *GetCurrentFileName(void);
extern char builtinClasses[];

Err IncludeFile(char *filename, char *currentBuff, char **newBuffer, FILE **newFile)
{
    char *buffer;
    FILE *file;
    size_t bytesRead;
    
    if (fileDepth == (FILE_DEPTH - 1))
    {
        return(TOO_MANY_INCLUDES);
    }
    buffer = (char *)malloc(BUFFSIZE);
    if (buffer == NULL)
    {
        return(NO_MEMORY);
    }

        /* Hack. If this is the builtinclasses file, then read it from RAM. */
    if (strcmp(filename, BUILTIN_FILENAME) == 0)
    {
        bytesRead = strlen(builtinClasses);
        memcpy(buffer, builtinClasses, bytesRead);
        file = 0;
    }
    else
    {
        file = fopen(filename, "r");
        if (file == NULL)
        {
            free(buffer);
            return(NO_INCLUDE);
        }

        bytesRead = fread((void *)buffer, 1, (BUFFSIZE - 1), file);
        DPRTFS(("Read %d bytes\n", bytesRead));
        if (bytesRead == 0)
        {
            fclose(file);
            free(buffer);
            return(NO_INCLUDE);
        }
    }
    
    if (bytesRead < (BUFFSIZE - 1))
    {
        DPRTFS(("feof() returns %ld\n", feof(file)));
        DPRTFS(("ferror() returns %ld\n", ferror(file)));
        buffer[bytesRead] = EOF_MARKER;
    }
    else
    {
        /* Stick the EOF_MARKER at the end of the buffer so the
         * parser doesn't overrun it.
         */
        buffer[BUFFSIZE - 1] = EOF_MARKER;
    }

    /* Store the info on the stack */
    DPRTFS(("pushing %s at depth %d: 0x%lx, 0x%lx, 0x%lx\n", filename, fileDepth, file, buffer, currentBuff));
    strncpy(filestack[fileDepth].filename, filename, FILENAME_SIZE);
    filestack[fileDepth].file = file;
    filestack[fileDepth].buffer = buffer;
    filestack[fileDepth].readPos = currentBuff;
    filestack[fileDepth].lineNumber = lineNumber;
    lineNumber = 1;
        
    /* and return the new pointers */
    if (newBuffer)
    {
        *newBuffer = buffer;
    }
    if (newFile)
    {
        *newFile = file;
    }
    
    fileDepth++;
    
    return(NO_ERROR);
}

Err HandleEOF(char **buffer)
{
    char *buff;
    size_t bytesRead;

    if (filestack[fileDepth - 1].file == 0)
    {
            /* Must be the pseudo builtinclasses file. */
        bytesRead = 0;
    }
    else
    {
            /* Read more data from the large file.
             * Knit the two reads together by copying the last KNIT_OVERLAP
             * characters to the beginning of the buffer, and reading more bytes from
             * the file after.
             */
        buff = filestack[fileDepth - 1].buffer;
        memcpy(buff, (buff + BUFFSIZE - KNIT_OVERLAP - 1), KNIT_OVERLAP);
        bytesRead =fread((buff + KNIT_OVERLAP), 1,
                         (BUFFSIZE - KNIT_OVERLAP - 1), filestack[fileDepth - 1].file);
    }

    if (bytesRead == 0)
    {
            /* Must have been the end of the file */
        DPRTFS(("Nothing to knit together.\n"));
    
            /* Pop down one level */
        fileDepth--;

        DPRTFS(("Handle EOF. depth = %ld\n", fileDepth));
        if (fileDepth <= 0)
        {
            fileDepth = 0;
            return(NO_MORE_DATA);
        }
    
        DPRTFS(("popping %s at depth %d: 0x%lx, 0x%lx, 0x%lx\n", filestack[fileDepth].filename, fileDepth, filestack[fileDepth].file,
                filestack[fileDepth].buffer, filestack[fileDepth].readPos));
        *buffer = filestack[fileDepth].readPos;
        fclose(filestack[fileDepth].file);
        lineNumber = filestack[fileDepth].lineNumber;
    }
    else
    {
        buff[bytesRead + KNIT_OVERLAP] = EOF_MARKER;
        *buffer = (buff + KNIT_OVERLAP);
    }
    return(NO_ERROR);
}

char *GetCurrentFileName(void)
{
    return(filestack[fileDepth - 1].filename);
}

