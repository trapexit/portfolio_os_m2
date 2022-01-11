/*
 * This file will actually tokenise all the data.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "parserfunctions.h"
#include "parsererrors.h"
#include "parsertypes.h"
#include "sdftokens.h"

extern Syntax ClassList;
extern Syntax EnumList;
extern Syntax ArrayList;
extern Syntax InbuiltClassList;
extern int32 *integerResult;
extern float *floatResult;
extern short int *shortResult;
extern bool incBufferPosition;

extern TokenBuffer *tb;

typedef struct stackSizes
{
    char *reference;
    char *buffPos;
} stackSizes;

Err CheckUniqueName(Syntax *syn, char *word, uint32 charCount, SyntaxItem **found);
Err CheckUniqueSymbol(Symbol *sym, char *word, uint32 charCount, Symbol **found);
char *GetCurrentFileName(void);
Err InitTokenBuffer(void);
void FreeTokenBuffer(void);
Err PopSizePointer(stackSizes **position);

SyntaxItem *foundType;
SyntaxItem *foundInstanceType;
SyntaxItem *useObjectType;
SyntaxItem *useObjectSymbol;

Symbol *symbols = NULL;   /* Store all the symbols off here */

int32 resultI;
float resultF;
short int resultSI;

extern SyntaxItem ValueItem[];
extern Syntax BuildObject;
extern Syntax BuildArray;
extern SyntaxItem BuildObjectItem[];
extern SyntaxItem BuildArrayItem[];

/**************************************************************************************/

/* CheckUniqueSymbol() returns DUPLICATE_NAME if the name was found in the current list.
 * This is a case sensitive search.
 */
Err CheckUniqueSymbol(Symbol *sym, char *word, uint32 charCount, Symbol **found)
{    
    while (sym)
    {
        if ((charCount == sym->stringLen) &&
            (strncmp(word, sym->symbolString, charCount) == 0))
        {
            if (found)
            {
                *found = sym;
            }
            return(DUPLICATE_NAME);
        }
        sym = sym->next;
    }
    return(NO_ERROR);
}

/**************************************************************************************/

extern SyntaxItem NumberItem[];
extern Syntax Integer;
Err WriteStringToTokens(char *wordIn, uint32 charCount);
Symbol *symbolNameBuffer;

Err FoundNewSym(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Symbol *sym;

    if (CheckUniqueSymbol(symbols, wordIn, charCount, NULL) == DUPLICATE_NAME)
    {
        return(DUPLICATE_NAME);
    }
    sym = (Symbol *)mymalloc(sizeof(Symbol) + charCount + 1);
    if (sym == NULL)
    {
        return(NO_MEMORY);
    }
    sym->symbolString = (char *)((uint32)sym + sizeof(Symbol));
    sym->stringLen = charCount;
    strncpy(sym->symbolString, wordIn, charCount);
    sym->symbolString[charCount] = '\0';
    sym->type = foundType;   /* This is the type of data this symbol represents */
    /* Set the pResult to point to this entry in the token buffer. We need -sizeof(uimt32)
     * because we have already tokenised the data type, and we are now pointing at the
     * "size" of this data block.
     */
    sym->data.u.pResult = (void *)(tb->position - sizeof(uint32));
    sym->next = symbols;
    symbols = sym;
    
    DPRT(("CREATING SYMBOL %s of type %s \n", sym->symbolString, sym->type->syntaxString));
    DTOKEN(("Setting symbol %s to token position 0x%lx\n", sym->symbolString,
            ((uint32)sym->data.u.pResult - (uint32)tb->buffer)));

        /* Put the symbol name in the token buffer, after the "{" is found.
         * If we were to insert the symbol name at this point in the buffer, then
         * the size field would not be found until after the string.
         */
    symbolNameBuffer = sym;
    
    /* Replace these with the latest position in the token buffer -- SAS */
    integerResult = &resultI;
    floatResult = &resultF;
    shortResult = &resultSI;
    
    /* We want to parse through the found type */
    NumberItem[0].nextLevel = foundType->nextLevel;
    
    return(NO_ERROR);
}

extern SyntaxItem BitmaskItem[];
int32 bitValue;
int32 bitMaskValue;

Err BuildBitMask(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    if (sr->type != SERESULT)
    {
        printf("***WARNING: Shouldn't see this!!\n");
        return(SYNTAX_ERROR);
    }
    
    DPRT(("BUILDING BITMASK FOR %s\n", sr->u.seResult->name));
    BitmaskItem[2].nextLevel = sr->u.seResult; /*foundType->nextLevel;*/
    integerResult = &bitValue;
    bitMaskValue = 0;
    incBufferPosition = FALSE; /* else we will inc the token position the first time
                                * we enumerate a value. We want to inc the position only
                                * when the last bit has been ORred.
                                */

    return(NO_ERROR);
}
    
Err OrBitValue(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    bitMaskValue |= bitValue;
    DPRT(("BITMASK VALUE = 0x%lx\n", bitMaskValue));
    return(NO_ERROR);
}

Err EndBitMask(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    bitMaskValue |= bitValue;
    *(uint32 *)tb->position = bitMaskValue;
    INC_BUFFER_POS(uint32);
    
    DPRT(("BITMASK FINAL VALUE = 0x%lx\n", bitMaskValue));
    return(NO_ERROR);
}

Err SetEnumDestination(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DPRT(("SETENUMDESTINATION\n"));
    
    WRITE_TOKEN(uint32, sr->id);
    
    integerResult = (int32 *)tb->position;
    incBufferPosition = TRUE;
    return(NO_ERROR);
}

Err FoundInstanceArrayType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    DPRT(("SET DESTINATION FOR ARRAY DATA\n"));

    /* We don't know what type of data we are dealing with, so set up all the
     * possible pointers.
     * Setting incBufferPosition TRUE will force the base type code to increment the
     * token position pointer by the appropriate size.
     */
    integerResult = (int32 *)tb->position;
    floatResult = (float *)tb->position;
    shortResult = (short int *)tb->position;
    incBufferPosition = TRUE;
    
    return(NO_ERROR);
}

bool matchedSymbol;

Err UseObjectType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err = NO_ERROR;

    matchedSymbol = FALSE;
    if ((CheckUniqueName(&InbuiltClassList, wordIn, charCount, &useObjectType) != DUPLICATE_NAME) &&
        (CheckUniqueName(&EnumList, wordIn, charCount, &useObjectType) != DUPLICATE_NAME) &&
        (CheckUniqueName(&ArrayList, wordIn, charCount, &useObjectType) != DUPLICATE_NAME) &&
        (CheckUniqueName(&ClassList, wordIn, charCount, &useObjectType) != DUPLICATE_NAME))
    {
        /* Maybe it's a symbol. This would be the case if we had the form:
         * datatype "use" symbol.
         * However, we don't know at this point if we have that syntax, so we just
         * assume that if we find the symbol we do.
         */
        useObjectType = NULL;
        err = UseObjectSymbol(buff, wordIn, charCount, sr);
        if (err < NO_ERROR)
        {
            /* Nope, it`s not a symbol either */
            err = UNKNOWN_TYPE;
        }
        else
        {
            /* The parse tree will still call UseObjectSymbol(), so we disable it... */
            matchedSymbol = TRUE;
        }
        
        return(err);
    }
    else
    {
        DPRT(("USING AN OBJECT OF TYPE %s\n", useObjectType->syntaxString));
    }
    return(NO_ERROR);
}

Err UseObjectSymbol(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Symbol *sym;
    Err err;
    
    if (matchedSymbol)
    {
        /* The symbol was matched by UseObjectType() */
        *buff -= charCount; /* El Hackipoo. We are here because of
                             * rule UseObjectItem[1], but the symbol was really found
                             * in UseObjectType() called from rule UseObjectItem[0].
                             */
        return(NO_ERROR);
    }
    
    if (CheckUniqueSymbol(symbols, wordIn, charCount, &sym) != DUPLICATE_NAME)
    {
        return(UNKNOWN_SYMBOL);
    }

    /* useObjectType can be NULL if we were called from UseObjectType() */
    if (useObjectType && ((uint32)sym->type != (uint32)useObjectType))
    {
        return(SYMBOL_TYPE_MISMATCH);
    }

    DPRT(("USING SYMBOL %s, at address 0x%lx (type 0x%lx)\n", sym->symbolString,
          sym->data.u.pResult, *(uint32 *)sym->data.u.pResult));
    WRITE_TOKEN(uint32, USE_SYMBOL);
    err = WriteTokenAddress(tb->position, sym->data.u.pResult);
    
    return(err);
}

Err UseArraySymbol(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Symbol *sym;
    Err err;
    
    if (CheckUniqueSymbol(symbols, wordIn, charCount, &sym) != DUPLICATE_NAME)
    {
        return(UNKNOWN_SYMBOL);
    }
    
     /* NB - there is no type checking. If I have the symbol, I know it's type. However,
      * I don't know what type of array I'm parsing, so I have nothing to compare against.
      * It would be far better if the "use" syntax in the array were the same as the "use"
      * syntax in the class.
      * ie: for class:  "use" datatype symbol
      *    for array   "use" symbol
      */

    DPRT(("USING SYMBOL %s OF TYPE %s IN THE ARRAY\n",
          sym->symbolString, sym->type->syntaxString));
     
    WRITE_TOKEN(uint32, USE_SYMBOL);
    err = WriteTokenAddress(tb->position, sym->data.u.pResult);

    return(err);
}

Err WriteDataType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DTOKEN(("WRITE DATA TYPE\n"));
    WRITE_TOKEN(uint32, sr->id);
    integerResult = (int32 *)tb->position;
    floatResult = (float *)tb->position;
    shortResult = (short int *)tb->position;
    incBufferPosition = TRUE;
    
    return(NO_ERROR);
}

Err WriteDataID(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DTOKEN(("WRITE DATA ID\n"));
    WRITE_TOKEN(uint32, sr->id);
    return(NO_ERROR);
}

Err OpenBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;

    DTOKEN(("OpenBrace [%s %d]\n", GetCurrentFileName(), lineNumber));
    err = PushSizePointer();

    if (symbolNameBuffer)
    {
        WRITE_TOKEN(uint32, SYMBOL_NAME);
        WriteStringToTokens(symbolNameBuffer->symbolString, symbolNameBuffer->stringLen);
        symbolNameBuffer = NULL;
    }
    return(err);
}

Err CloseBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    DTOKEN(("CloseBrace [%s %d]\n", GetCurrentFileName(), lineNumber));
    err = WriteBlockSize();
    return(err);
}

/***********************************************************************************************/

#define WRITE_TOKEN_BUFFER TRUE

#define BRACE_DEPTH 256
/* Store pointers to size fiilds off this stack. BRACE_DEPTH should be enough... */

stackSizes sizeStack[BRACE_DEPTH];
int32 sizeDepth;

TokenBuffer *tb;
TokenBuffer *firstTB;

Err InitTokenBuffer(void)
{
    TokenHead *th;
    sizeDepth = 0;
    sizeStack[0].reference = sizeStack[0].buffPos = NULL;

    firstTB = (TokenBuffer *)malloc(sizeof(TokenBuffer));
    if (firstTB == NULL)
    {
        return(NO_MEMORY);
    }
    tb = firstTB;
    memset(tb, 0, sizeof(TokenBuffer));    

    /* This first buffer will contain the TokenHead. Point the position to the
     * field position of the TokenHead.
     */
    th = (TokenHead *)tb->buffer;
    tb->position = (char *)&th->size;
    /*th->flags = TH_ADDR_DELTA;*/
    
    PushSizePointer();
    
    return(NO_ERROR);
}

void FreeTokenBuffer(void)
{
    TokenBuffer *next;
    
    tb = firstTB;
    
#if WRITE_TOKEN_BUFFER
    {
        FILE *out;
        out = fopen("tokenbuffer", "w");
        if (out)
        {
            while (tb)
            {
                fwrite(tb->buffer, sizeof(char), TOKEN_BUFFER_SIZE, out);
                tb = tb->next;
            }

            fclose(out);
        }
    }
#endif
    
    while (tb)
    {
        next = tb->next;
        free(tb);
        tb = next;
    }
}

Err PushSizePointer(void)
{
    if (sizeDepth == BRACE_DEPTH)
    {
        return(TOO_MANY_BRACES);
    }
    
    sizeStack[sizeDepth].reference =  sizeStack[sizeDepth].buffPos = tb->position;
    *(uint32 *)tb->position = 0;
    tb->position += (sizeof(uint32));
    sizeDepth++;
    return(NO_ERROR);
}

Err PopSizePointer(stackSizes **position)
{
    if (sizeDepth == 0)
    {
        return(TOO_MANY_CLOSE_BRACES);
    }
    
    sizeDepth--;
    *position = &sizeStack[sizeDepth];
    return(NO_ERROR);
}

Err WriteBlockSize(void)
{
    stackSizes *pos;
    Err err;
    int32 size;
    
    err = PopSizePointer(&pos);
    if (err < NO_ERROR)
    {
        return(err);
    }

    /* The size of this block is the current buffer position - the buffer position of
     * the size field itself, exclusive.
     */
    size = ((uint32)tb->position - (uint32)pos->reference - sizeof(uint32));
    *(int32 *)pos->buffPos += size;
    DTOKEN(("SIZE OF BLOCK = %d (0x%lx)\n", *(int32 *)pos->buffPos,  *(int32 *)pos->buffPos));
        /* Next block must start on a 32 bit boundary. */
    tb->position = (char *)((uint32)(tb->position + 3) & ~0x3);
    err = CheckTokenBufferOverflow();
    return(err);
}

Err WriteTokenAddress(char *pos, void *addr)
{
    TokenHead *th = (TokenHead *)firstTB->buffer;
    uint32 *p;
    Err err;

    p = (uint32 *)pos;
    if (th->flags & TH_ADDR_DELTA)
    {
        *p = ((uint32)addr - (uint32)p);
    }
    else
    {
        *p = (uint32)addr;
    }
    INC_BUFFER_POS(uint32);
}

#define END_SIZE (sizeof(uint32) * 2)

Err CheckTokenBufferOverflow(void)
{
    TokenBuffer *new;
    TokenHead *th = (TokenHead *)firstTB->buffer;
    uint32 *p;
    int32 i;
    
    
        /* See if the current position in the token buffer is near the end.
         * if it is, then allocate a new buffer and join the two together.
         */
    if (((uint32)tb->position - (uint32)&tb->buffer[0]) >= (TOKEN_BUFFER_SIZE - END_SIZE))
    {
        new = (TokenBuffer *)malloc(sizeof(TokenBuffer));
        if (new == NULL)
        {
            return(NO_MEMORY);
        }
        memset(new, 0, sizeof(TokenBuffer));
        new->prev = tb;
        new->position = &new->buffer[0];
        tb->next = new;

        DTOKEN(("Stitching a new token buffer in.\n"));

        /* We can't reuse the WRITE_TOKEN macro or WriteTokenAddress() here as that
         * would lead to CHeckTokenBufferOverflow() being recursively called.
         */
        *(uint32 *)tb->position = NEXT_BUFFER;
        tb->position += sizeof(uint32);

        p = (uint32 *)tb->position;
        if (th->flags & TH_ADDR_DELTA)
        {
            *p = ((uint32)&new->buffer[0] - (uint32)p);
        }
        else
        {
            *p = (uint32)&new->buffer[0];
        }
        tb->position += sizeof(uint32);

        /* Now go through our stack of sizes, and set them to the size of their block so
         * far (ie the size from their starting position in the buffer to the end of the buffer).
         * Then point them at the start of the new buffer. When the end of their blok is found,
         * and their true size is calculated, we can add the difference to get an accurate
         * size value.
         */
        for (i = (sizeDepth - 1); i >= 0; --i)
        {
            int32 size;
            stackSizes *pos = &sizeStack[i];
            
            size = ((uint32)tb->position - (uint32)pos->reference);
            *(int32 *)pos->buffPos += size;
            pos->reference = new->position;
        }
        
        /* Now, make this new buffer the current one to use. */
        tb = new;
    }

    return(NO_ERROR);
}
