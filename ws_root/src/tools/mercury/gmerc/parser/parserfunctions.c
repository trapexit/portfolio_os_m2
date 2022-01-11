/*
 * Load of functions called by the parser.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "parser.h"
#include "parserfunctions.h"
#include "parsererrors.h"
#include "sdftokens.h"

#define STRLEN 16

int32 *integerResult = NULL;
float *floatResult = NULL;
short int *shortResult = NULL;
bool incBufferPosition = FALSE;

extern TokenBuffer *tb;
extern TokenBuffer *firstTB;

char *GetCurrentFileName(void);
Err IncludeFile(char *filename, char *currentBuff, char **newBuffer, FILE **newFile);
Err LookForSyntax(Syntax *syn, char **buff);
Err WriteStringToTokens(char *wordIn, uint32 charCount);

Err ConvertShortInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    char *endp;
    short int result;
    Err err;
    
    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    endp = NULL;

    result = (short int)strtol(word, &endp, 0);
    DPRT(("FOUND short string %s\n", word));
    if ((endp) && (((uint32)endp - (uint32)word) != charCount))
    {
        DPRT(("BAD SHORT NUMBER\n"));
        return(MALFORMED_INTEGER);
    }
    DPRT(("\t(%d)\n", result));

    if (shortResult)
    {
        *shortResult = result;
        shortResult = NULL;    /* Save potential bugs later */
        if (incBufferPosition)
        {
            INC_BUFFER_POS(short int);
            incBufferPosition = FALSE;
        }
    }
    
    return(NO_ERROR);
}

Err ConvertHexInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    char *endp;
    uint32 hex;
    Err err;
    
    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    endp = NULL;

    hex = strtol(word, &endp, 16);
    DPRT(("FOUND HEX string %s\n", word));
    if ((endp) && (((uint32)endp - (uint32)word) != charCount))
    {
        DPRT(("BAD HEX NUMBER\n"));
        return(MALFORMED_INTEGER);
    }
    DPRT(("\t(0x%lx)\n", hex));

    if (integerResult)
    {
        *integerResult = hex;
        integerResult = NULL;    /* Save potential bugs later */
        if (incBufferPosition)
        {
            INC_BUFFER_POS(uint32);
            incBufferPosition = FALSE;
        }
    }
    
    return(NO_ERROR);
}

Err ConvertOctInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    char *endp;
    uint32 oct;
    Err err;
    
    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    endp = NULL;

    oct = strtol(word, &endp, 8);
    DPRT(("FOUND OCT string %s\n", word));
    if ((endp) && (((uint32)endp - (uint32)word) != charCount))
    {
        DPRT(("BAD OCT NUMBER\n"));
        return(MALFORMED_INTEGER);
    }
    DPRT(("\t(0x%lx)\n", oct));

    if (integerResult)
    {
        *integerResult = oct;
        integerResult = NULL;    /* Save potential bugs later */
        if (incBufferPosition)
        {
            INC_BUFFER_POS(uint32);
            incBufferPosition = FALSE;
        }
    }
    
    return(NO_ERROR);
}

Err ConvertDecInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    char *endp;
    uint32 dec;
    Err err;
    
    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    endp = NULL;

    dec = strtol(word, &endp, 10);
    DPRT(("FOUND DEC string %s\n", word));
    if ((endp) && (((uint32)endp - (uint32)word) != charCount))
    {
        DPRT(("BAD DEC NUMBER\n"));
        return(MALFORMED_INTEGER);
    }
    DPRT(("\t(%ld)\n", dec));

    if (integerResult)
    {
        *integerResult = dec;
        integerResult = NULL;    /* Save potential bugs later */
        if (incBufferPosition)
        {
            INC_BUFFER_POS(uint32);
            incBufferPosition = FALSE;
        }
    }
    
    return(NO_ERROR);
}

Err ConvertFloat(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    char *endp;
    double f;
    Err err;
    
    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    endp = NULL;

    f = (double)strtod(word, &endp);
    DPRT(("FOUND FLOAT string %s\n", word));
    if ((endp) && (((uint32)endp - (uint32)word) != charCount))
    {
        DPRT(("BAD FLOAT NUMBER\n"));
        return(MALFORMED_FLOAT);
    }
    
    DPRT(("\t(%f)\n", f));

    if (floatResult)
    {
        *floatResult = f;
        integerResult = NULL;    /* Save potential bugs later */
        if (incBufferPosition)
        {
            INC_BUFFER_POS(float);
            incBufferPosition = FALSE;
        }
    }

    return(NO_ERROR);
}

Err ConvertString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];

    /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';

    DPRT(("FOUND STRING %s\n", word));
    return(NO_ERROR);
}

Err FoundMeters(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND METERS\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_METERS);
    return(NO_ERROR);
}

Err FoundKMeters(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND KMETERS\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_KMETERS);
    return(NO_ERROR);
}

Err FoundFeet(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND FEET\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_FEET);
    return(NO_ERROR);
}

Err FoundInches(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND INCHES\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_INCHES);
    return(NO_ERROR);
}

Err FoundNautMiles(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND NAUTMILES\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_NAUTMILES);
    return(NO_ERROR);
}

Err FoundUnits(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    DPRT(("FOUND UNITS\n"));
    return(NO_ERROR);
}

Err FoundUnitsInt(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("FOUND UNITS OF TYPE INTEGER\n"));
    WRITE_TOKEN(uint32, SDF_UNITS);
    WRITE_TOKEN(uint32, ENUM_UNITS_INTEGER);
    integerResult = (int32 *)tb->position;
    incBufferPosition = TRUE;
    return(NO_ERROR);
}

Err FoundSDFVersion(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    TokenHead *th;
    
    DPRT(("FOUND SDFVERSION\n"));
    th = (TokenHead *)firstTB->buffer;
    floatResult = &th->version;
    return(NO_ERROR);
}

extern FILE *file;
extern struct Syntax File;               /* This is the start of every definition */
Err HandleInclude(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];
    Err err;
    
        /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';

    DPRT(("INCLUDING %s\n", word));
    err = IncludeFile(word, *buff, buff, &file);
    if (err < 0)
    {
        return(err);
    }
    do
    {
        err = LookForSyntax(&File, buff);
    }
    while (err == NO_ERROR);
    return(err);
}

Err WriteStringToTokens(char *wordIn, uint32 charCount)
{
    uint32 round;
    char *word;
    Err err;
    
    /* round the size up to the nearest 4 bytes, so that all subsequent
     * numbers are aligned.
     */
    round = ((charCount + 3) & 0xfffffffc);
    
        /* NB - not checking if string fits */
    WRITE_TOKEN(uint32, round);
    word = tb->position;
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';
    /* Need buffer stitching here. */
    tb->position += round;
    DTOKEN(("Writing string %s\n", word));
    return(NO_ERROR);
}

Err HandleFiles(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    err = WriteStringToTokens(wordIn, charCount);
    return(err);
}

Err WarnGarbage(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    char word[STRLEN];

        /* NB - not checking if string fits */
    strncpy(word, wordIn, charCount);
    word[charCount] = '\0';

#ifdef KF_OBJECT
    printf("*** Warning: [%s:%d] Found %s\n",
           GetCurrentFileName(), lineNumber, word);
#endif
    return(NO_ERROR);
}
