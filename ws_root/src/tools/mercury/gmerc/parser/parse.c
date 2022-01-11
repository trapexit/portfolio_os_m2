/*
 * recursive parser code.
 */
#include "parsertypes.h"
#include "parsererrors.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TOKENISE	0

#define DBUG_WS 0
#define DBUG_TRACE 0
#if (DBUG_WS)
#define DPRTWS(x) printf x;
#else
#define DPRTWS(x)
#endif

#define DPRTT(x) {if (printParse) {printf x;}}

bool printDebug;
bool printParse;
bool printDefinitions;
bool printTokenising;
int32 recursionDepth;
int32 errorDepth;
int32 lineNumber;

#define MATCHES \
     /* If the si->syntaxString is NULL, then we have reached the default   \
      * type (ie this word is unrecognised at this level), so we need  to set  \
      * buff back to its orignal position so that we can parse the same data \
      * again at the next level. \
      */ \
(((si->syntaxString == NULL) && \
  ((((si->nextLevel && !(si->flags & SKIP_MATCHED)) || (si->flags & KEEP_WORD)) \
    && (*buff -= charCount)) || TRUE)) || \
     /* This string must much exactly */ \
 ((charCount == si->stringLen) && \
  (strncmp(word, si->syntaxString, charCount) == 0)) || \
     /* Can we match just the first few characters? */ \
 ((si->flags & MATCH_LEFT) && \
  (strncmp(word, si->syntaxString, si->stringLen) == 0)) || \
     /* How about the last few characters? */ \
  ((si->flags & MATCH_RIGHT) && \
   (strncmp((word + charCount - si->stringLen), si->syntaxString, si->stringLen) == 0)))

static void CheckNewLine(char c);
static void SetStringCase(SyntaxItem *si, char *word, uint32 charCount);
static Err SkipWholeLine(char **buff);
static Err SkipWhiteSpace(char **buff);
static Err GetNextWord(char **buff, char **word, uint32 *charCount, uint32 flags);
Err LookForSyntax(Syntax *syn, char **buff);

Err HandleEOF(char **buffer);
char *GetCurrentFileName(void);
Err IncludeFile(char *filename, char *currentBuff, char **newBuffer, FILE **newFile);
Err InitTokenBuffer(void);
void FreeTokenBuffer(void);
Err WriteBlockSize(void);
void InitMemHeader(void);
void FreeAll(void);


static void CheckNewLine(char c)
{
    if ((c == '\n') || (c == '\r'))
    {
        lineNumber++;
    }
}

static void SetStringCase(SyntaxItem *si, char *word, uint32 charCount)
{
    if (!(si->flags & CASE_SENSITIVE))
    {
        int32 i;
        
        /* make all lowercase */
        for (i = 0; i < charCount; i++)
        {
            if ((word[i] >= 'A') && (word[i] <= 'Z'))
            {
                word[i] += ('a' - 'A');
            }
        }
    }
}

static Err SkipWholeLine(char **buff)
{
    char *b = *buff;
    char c;
    uint32 count = 0;
    Err err;
    
    DPRTWS(("SkipWholeLine\n"));
    
    do
    {
        c = *b;
        count++;
        DPRTWS(("%c", c));
        if (c == EOF_MARKER)
        {
            err = HandleEOF(buff);
            if (err == NO_MORE_DATA)
            {
                return(err);
            }
            b = *buff;
            c = *b;
            /* HandleEOF() copies the last few bytes from the end of the buffer to
             * the front, and sets *buff to the continuation point. We need to set it
             * back to the point we came in at.
             */
            *buff -= count;
            DPRTWS(("-> %c", c));
        }
    }
    while (((c != '\n') && (c != '\r')) && b++);
    
    *buff = b;
    DPRTWS(("\n"));
    return(NO_ERROR);
}       
   
static Err SkipWhiteSpace(char **buff)
{
    char *b = *buff;
    char c;
    uint32 count = 0;
    Err err;

    DPRTWS(("SkipWhiteSpace [%d]\n", recursionDepth));
    
    do
    {
        c = *b;
        count++;
        CheckNewLine(c);
        DPRTWS(("%c", c));
        if (c == COMMENT_CHAR)
        {
            *buff = b;
            err = SkipWholeLine(buff);
            if (err < NO_ERROR)
            {
                return(err);
            }
            b = *buff;
            c = *b;
            CheckNewLine(c);
            DPRTWS(("-> %c", c));
        }
        if (c == EOF_MARKER)
        {
            err = HandleEOF(buff);
            if (err == NO_MORE_DATA)
            {
                return(err);
            }
            b = *buff;
            c = *b;
            /* HandleEOF() copies the last few bytes from the end of the buffer to
             * the front, and sets *buff to the continuation point. We need to set it
             * back to the point we came in at.
             */
            *buff -= count;
            CheckNewLine(c);
            DPRTWS(("-> %c", c));
        }
    }
    while ((c <= ' ') && b++);

    *buff = b;
    DPRTWS(("\n"));
    return(NO_ERROR);
}       

static Err GetNextWord(char **buff, char **word, uint32 *charCount, uint32 flags)
{
    char *b = *buff;
    char c;
    uint32 count = 0;
    Err err;

    DPRTWS(("GetNextWord [%d]\n", recursionDepth));
    /*
     * Keep moving over the characters until we find either a whitespace character
     * or a special character. A special character is one that does not need to be
     * surrounded by whitespace.
     */
    do
    {
        c = *b;
        DPRTWS(("%c", c));
        if (c == EOF_MARKER)
        {
            err = HandleEOF(buff);
            if (err == NO_MORE_DATA)
            {
                return(err);
            }
            b = *buff;
            c = *b;
            /* HandleEOF() copies the last few bytes from the end of the buffer to
             * the front, and sets *buff to the continuation point. We need to set it
             * back to the beginning of the word that is being isoloated.
             */
            *buff -= count;
            DPRTWS(("-> %c", c));
        }
    }
    while (((flags & TO_QUOTE) && ++count && ++b && (c != '"')) ||
           ((!(flags & TO_QUOTE)) && (c > ' ') && ++count && ++b && (strchr(SPECIAL_CHARS, c) == NULL)));

    /* Did we go too far? */
    if ((count > 1) && (strchr(SPECIAL_CHARS_EXCLUDE, c)))
    {
        --count;
        --b;
    }
    
    *word = *buff;
    *buff = b;
    *charCount = count;
    DPRTWS(("\n"));
    return(NO_ERROR);
}

Err LookForSyntax(Syntax *syn, char **buff)
{
    uint32 i;
    Err err;
    char *word;
    char *origWord;
    SyntaxItem *si;
    uint32 charCount;
    bool found;
    bool foundAlt = TRUE;
    bool doneAlt = FALSE;
    bool doneReq = FALSE;
    
    recursionDepth++;
    PRTDEPTH;
    DPRT(("\t RULE %s\n", syn->name ? syn->name : "??"));
    
    origWord = (char *)malloc(16);
    
    do
    {
        found = FALSE;

        /* Are there any strings that *must* be at this level? */
        if ((si = syn->required) &&
            (!doneReq ||
             (doneReq && (!(syn->repeatFlags & REPEAT_ONLY_ALT)))))
        {
            doneReq = TRUE;
            for (i = 0; i < syn->countReq; si = si->nextSI, i++)
            {
                if (si->flags & SKIP_THIS)
                {
                    continue;
                }
                if (!(si->flags & KEEP_WORD))
                {
                        /* Skip over white space */
                    err = SkipWhiteSpace(buff);
                    UNWIND_ON_ERR(err);
    
                    err = GetNextWord(buff, &word, &charCount, (si->flags & TO_QUOTE));
                    UNWIND_ON_ERR(err);
                }

                /* Repeating until a close-brace is found? */ 
                if ((syn->repeatFlags & REPEAT_UNTIL_BRACE) && (*word == '}'))
                {
                    DPRTT(("REPEATED UNTIL BRACE FOUND [rule %s line %d depth %d].\n", syn->name, lineNumber, recursionDepth));
                    if (syn->repeatFlags & REPEAT_WORD)
                    {
                        DPRTT(("*** putback.\n"));
                        *buff -= charCount;
                    }
                    break;
                }

                SetStringCase(si, word, charCount);
                DPRTT(("Must have %s at level %d [%s %d/%d]\n",
                       (si->syntaxString ? si->syntaxString : "<default>"), recursionDepth,
                       syn->name, (i + 1), syn->countReq));
                if (!MATCHES)
                {
                    if (si->flags & CONTINUE_ON_ERR)
                    {
                        DPRTT(("----> continue on err\n"));
                        *buff -= charCount;
                        continue;
                    }
                    err = SYNTAX_ERROR;
                    UNWIND_ON_ERR(err);
                }
                DPRTT(("Match\n"));
                found = TRUE;
                
                /* Should we alter the string before calling the function? */
                if (si->flags & SKIP_MATCHED)
                {
                    if (si->flags & MATCH_LEFT)
                    {
                        word += si->stringLen;
                    }
                    charCount -= si->stringLen;
                }
                
                if (si->syntaxFn)
                {
                    err = (si->syntaxFn)(buff, word, charCount,
                                         ((si->flags & RESULT_VALID) ? &si->result : NULL));
                    UNWIND_ON_ERR(err);
                }
                
                if (si->flags & REPEAT_WORD)
                {
                    *buff -= charCount;
                }

                if (si->nextLevel)
                {
                        /* Recurse down one more level */
                    err = LookForSyntax(si->nextLevel, buff);
                    UNWIND_ON_ERR(err);
                }

                if (si->flags & RETURN)
                {
                    break;   
                }
            }
        }
        if (si && si->flags & RETURN)
        {
            /* break again in case there were alternatives after the required tokens */
            break;   
        }
        
            /* Look for all the alternative syntax at this level */
        if ((si = syn->alternatives) &&
            (!doneAlt ||
             (doneAlt && (!(syn->repeatFlags & REPEAT_ONLY_REQ)))))
        {
            doneAlt = TRUE;
            foundAlt = FALSE;
            
                /* Skip over white space */
            err = SkipWhiteSpace(buff);
            UNWIND_ON_ERR(err);
            
            err = GetNextWord(buff, &word, &charCount, (si->flags & TO_QUOTE));
            UNWIND_ON_ERR(err);
                /* Repeating until a close-brace is found? */
            if ((syn->repeatFlags & REPEAT_UNTIL_BRACE) && (*word == '}'))
            {
                DPRTT(("REPEATED UNTIL BRACE FOUND [rule %s line %d depth %d]\n", syn->name, lineNumber, recursionDepth));
                if (syn->repeatFlags & REPEAT_WORD)
                {
                    DPRTT(("*** putback\n"));
                    *buff -= charCount;
                }
                break;
            }

            origWord = (char *)realloc(origWord, charCount + 1);
            if (origWord == NULL)
            {
                return(NO_MEMORY);
            }
            /* Make a copy of the string, in case the aternatives mix case sensitivity */
            strncpy(origWord, word, charCount);
            origWord[charCount] = '\0';
            DPRTT(("Found word %s at line %d\n", origWord, lineNumber));
            
            for (i = 0; i < syn->countAlt; si = si->nextSI, i++)
            {
                if (si->flags & SKIP_THIS)
                {
                    continue;
                }
                /* Copy the original string back into word, in case the last time through
                 * changed the string case and this time is case sensitive. Must use
                 * strncpy() to avoid writing the \0 character back into the file buff,
                 * as this looks like an EOF marker!
                 */
                strncpy(word, origWord, charCount);
                SetStringCase(si, word, charCount);
                
                DPRTT(("Do we have %s at level %d [%s %d/%d]?\n",
                       (si->syntaxString ? si->syntaxString : "<default>"), recursionDepth,
                       syn->name, (i + 1), syn->countAlt));
                if (MATCHES)
                {
                        /* We have a match!! */
                    DPRTT(("Match\n"));
                    found = TRUE;
                    
                    if (si->syntaxFn)
                    {
                        err = (si->syntaxFn)(buff, word, charCount,
                                             ((si->flags & RESULT_VALID) ? &si->result : NULL));
                        UNWIND_ON_ERR(err);
                    }
                    if (si->nextLevel)
                    {
                            /* Recurse down one more level */
                        err = LookForSyntax(si->nextLevel, buff);
                        if (!(si->flags & CONTINUE_ON_ERR))
                        {
                            UNWIND_ON_ERR(err);
                        }
                        else if (err < NO_ERROR)
                        {
                            found = FALSE;
                            continue;
                        }
                    }
                    if (si->flags & RETURN)
                    {
                        DPRTT(("Forced return\n"));
                        found = FALSE;
                    }
                    if (si->flags & REPEAT_WORD)
                    {
                        *buff -= charCount;
                    }
                    
                    break;
                }
            }
            if (!foundAlt && (syn->repeatFlags & KEEP_UNMATCHED))
            {
                DPRT(("**** Keep unmatched\n"));
                *buff -= charCount;
            }
        }
    } while ((syn->repeatFlags & (REPEAT | REPEAT_UNTIL_BRACE)) && found);

    free(origWord);
    recursionDepth--;
    if (!found && (syn->repeatFlags & DONT_REPEAT))
    {
        DPRT(("SYNTAX_ERR0R\n"));
        return(SYNTAX_ERROR);
    }
    
    return(err);
}

/*************************************************/
FILE *builtinFile = NULL;
char *builtinBuffer = NULL;
extern int32 fileDepth;
extern int32 sizeDepth;
extern Symbol *symbolNameBuffer;
#define DBG_BUILTIN FALSE

/* InitParser() initialises the parser global variables */
Err InitParser(Syntax *start)
{
    Err err;
	char *tmpbuff;
    
    recursionDepth = 0;
    errorDepth = -1;
    fileDepth = 0;
    lineNumber = 1;
#ifdef TOKENISE
    symbolNameBuffer = NULL;
#endif

    InitMemHeader();
        
#ifdef TOKENISE
    err = InitTokenBuffer();
    if (err < NO_ERROR)
    {
        return(err);
    }
#endif
    
    printDebug = DBG_BUILTIN;
    err = IncludeFile(BUILTIN_FILENAME, NULL, &builtinBuffer, &builtinFile);
	tmpbuff = builtinBuffer;
    if (err < NO_ERROR)
    {
       printf("Error parsing builtinclasses.sdf\n");
       ExplainError(err, NULL, NULL, 0);
       return(err);
    }
    while (err == NO_ERROR)
    {
        err = LookForSyntax(start, &builtinBuffer);
    }

    /* free(builtinBuffer); */
    free(tmpbuff);
    
    printDebug = FALSE;
    printParse = FALSE;
    printDefinitions = FALSE;
    printTokenising = FALSE;
    
    return(NO_ERROR);
}

/*************************************************/

/* DeletelParser() frees up all the parser resources */
extern Symbol *symbols;
extern Syntax EnumList;
extern Syntax ClassList;
extern Syntax ArrayList;
#ifdef TOKENISE
extern TokenBuffer *firstTB;
#endif    

void DeleteParser(void)
{
    int32 i, j;
    SyntaxItem *si, *si1, *next;
    Syntax *syn;
    Symbol *nextSym;

    nextSym = symbols;
    while (nextSym)
    {
        Symbol *tmp = nextSym->next;
        PRTDEF(("Symbol %s of type %s\n", nextSym->symbolString, nextSym->type->syntaxString));
        nextSym = tmp;
    }
    PRTDEF(("\n"));
    
    if (printDefinitions)
    {
        for (i = 0, si = ClassList.alternatives; i < ClassList.countAlt; i++)
        {
            if (si->nextLevel == NULL)
            {
                continue;
            }
            PRTDEF(("Class %s\n{\n", si->syntaxString));
            for (j = 0, si1 = si->nextLevel->alternatives; j < si->nextLevel->countAlt; j++)
            {
                next = si1->nextSI;
                if (si1->flags & RESULT_VALID)
                {
                    PRTDEF(("\t%s\t%s\n", si1->result.u.taResult->syntaxString, si1->syntaxString));
                }
                si1 = next;
            }
        
            PRTDEF(("\t[Pad %s %d]\n}\n", (si->flags & RESULT_VALID) ? "VALID" : "INVALID",
                    (si->flags & RESULT_VALID) ? si->result.u.iResult : 0));
        
            next = si->nextSI;
            si = next;
        }
        PRTDEF(("\n"));
    
        for (i = 0, si = ArrayList.alternatives; i < ArrayList.countAlt; i++)
        {
            PRTDEF(("Array %s of type %s\n", si->syntaxString, si->result.u.taResult->syntaxString));
            next = si->nextSI;
            si = next;
        }
        PRTDEF(("\n"));

        for (i = 0, si = EnumList.alternatives; i < EnumList.countAlt; i++)
        {
            if (si->nextLevel == NULL)
            {
                continue;
            }
            PRTDEF(("Enum %s\n{\n", si->syntaxString));        
            for (j = 0, si1 = si->nextLevel->alternatives; j < si->nextLevel->countAlt; j++)
            {
                next = si1->nextSI;
                if (si1->syntaxString[0] != '(')
                {
                    PRTDEF(("\t%s\t= %d\n", si1->syntaxString, si1->result.u.iResult));
                }
                si1 = next;
            }
            PRTDEF(("}\n"));
        
            next = si->nextSI;
            si = next;
        }
    }

    FreeAll();
    
#ifdef TOKENISE
    /* Write the size of the entire tokenised buffer */
    WriteBlockSize();
    FreeTokenBuffer();
#endif
}

FILE *file;
Err StartParser(char *filename, Syntax *start, char **tokens)
{
    Err err;
    char *rootBuffer = NULL;
    char *_rootBuffer;
    
    err  = IncludeFile(filename, NULL, &rootBuffer, &file);
    if (err < 0)
    {
        printf("Error opening the file %s\n", filename);
        return(err);
    }

        /* Save rootBuffer, as LookForSyntax() will change it. */
    _rootBuffer = rootBuffer;
    do
    {
        err = LookForSyntax(start, &rootBuffer);
    }
    while (err == NO_ERROR);

#ifdef TOKENISE
    *tokens = firstTB->buffer;
#else
    *tokens = NULL;
#endif
    
    fclose(file);
    free(_rootBuffer);

    return(err);
}
