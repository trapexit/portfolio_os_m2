#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "parserfunctions.h"
#include "parsererrors.h"
#include "parsertypes.h"

#define ARRAY_INC 0x01000000
uint32 arrayCount = 0x03000000;

extern Syntax ClassList;
extern Syntax EnumList;
extern Syntax ArrayList;
extern Syntax InbuiltClassList;
extern Syntax GetArraySymbol;
extern SyntaxItem GetArraySymbolItem[];
extern SyntaxItem GetArraySymbol2Item[];
extern Syntax ParseArrayData;
extern SyntaxItem ParseArrayDataItem[];
extern Syntax ParseArrayArray;
extern SyntaxItem ParseArrayArrayDataItem[];

extern int32 *integerResult;
extern SyntaxItem *foundType;

extern TokenBuffer *tb;

char *GetCurrentFileName(void);
Err CheckUniqueName(Syntax *syn, char *word, uint32 charCount, SyntaxItem **found);

Err ParseArray(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    SyntaxItem *base = NULL;
    
    err = CheckUniqueName(&ArrayList, wordIn, charCount, &foundType);
    if (err != DUPLICATE_NAME)
    {
        /* Then this was called as a result of recognising the array declarer in
         * a class. (see HandleClassString() in class.c, where the si->syntaxFn
         * is set).
         *
         * Find the base array type to set the data type to parse.
         */
        foundType = sr->u.taResult;
        base = foundType;
        while (base->flags & RESULT_VALID)
        {
            base = base->result.u.taResult;
        }
        DPRT(("BUILDING ARRAY OF TYPE %s (%d) [= %s = (%d)].\n",foundType->syntaxString,
              foundType->result.type, base->syntaxString, base->result.type));
    }
    else
    {
        DPRT(("BUILDING ARRAY OF TYPE %s (%d) [= %s = (%d)]\n", foundType->syntaxString,
              foundType->result.type, foundType->result.u.taResult->syntaxString, foundType->result.u.taResult->result.type));
        base = foundType->result.u.taResult;
    }

    WRITE_TOKEN(uint32, sr->id);
    
    /* Each instance needs to be parsed by the found type */

    /* Set GetArraySymbolItem[2].nextLevel to the next syntax to parse, depending on
     * the datatype.
     */
    switch (foundType->result.type)
    {
        case (BASE_TYPE):  /* one of the base classes eg float. integer */
        {
            GetArraySymbolItem[2].nextLevel = &ParseArrayData;
            GetArraySymbol2Item[1].nextLevel = &ParseArrayData;
            ParseArrayDataItem[2].nextLevel = base->nextLevel;
            break;
        }
        case (CLASS_TYPE):
        case (ARRAY_TYPE):
        {
            GetArraySymbolItem[2].nextLevel = &ParseArrayArray;
            GetArraySymbol2Item[1].nextLevel = &ParseArrayArray;
            ParseArrayArrayDataItem[4].nextLevel = (base ?
                                                    base->nextLevel : foundType->result.u.taResult->nextLevel);
            break;
        }
        default: break;
    }
        
    return(NO_ERROR);
}

Err FoundDefineArray(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *first;
    Err err;

    /* See if this name is already defined */
    if (((err = CheckUniqueName(&EnumList, wordIn, charCount, NULL)) < NO_ERROR) ||
        ((err = CheckUniqueName(&ClassList, wordIn, charCount, NULL)) < NO_ERROR) ||
        ((err = CheckUniqueName(&ArrayList, wordIn, charCount, NULL)) < NO_ERROR) ||
       ((err = CheckUniqueName(&InbuiltClassList, wordIn, charCount, NULL)) < NO_ERROR))
    {
        return(err);
    }

    si = (SyntaxItem *)mymalloc(sizeof(SyntaxItem) + charCount + 1);
    if (si == NULL)
    {
        return(NO_MEMORY);
    }
    si->syntaxString = (char *)((uint32)si + sizeof(SyntaxItem));
    strncpy(si->syntaxString, wordIn, charCount);
    si->syntaxString[charCount] = '\0';
    si->stringLen = charCount;
    si->flags = ID_VALID;
    si->syntaxFn = ParseArray;
    si->nextLevel = &GetArraySymbol;
    arrayCount += ARRAY_INC;
    si->result.id = arrayCount;

    /* Now add this to the list */
    first = ArrayList.alternatives;
    ArrayList.alternatives = si;
    si->nextSI = first;
    ArrayList.countAlt++;

    DPRT(("FOUND DEFINE ARRAY %s\n", si->syntaxString));
    
    return(NO_ERROR);
}
    
/*
 * FoundArrayType() will hang this string off of the ArrayList we are building,
 * and then point down to the found type.
 */
#define ARRAY_NAME " Array Types"
Err FoundArrayType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *arrayType;
    uint32 foundArrayType;
    Err err;

    /* Do we know this type? */
    if (((foundArrayType = BASE_TYPE) && (CheckUniqueName(&InbuiltClassList, wordIn, charCount, &arrayType) != DUPLICATE_NAME)) &&
        ((foundArrayType = ARRAY_TYPE) && (CheckUniqueName(&ArrayList, wordIn, charCount, &arrayType) != DUPLICATE_NAME)) &&
        ((foundArrayType = CLASS_TYPE) && (CheckUniqueName(&ClassList, wordIn, charCount, &arrayType) != DUPLICATE_NAME)))
    {
        /* We don't know what this is!! */
        return(UNKNOWN_TYPE);
    }

    /* Note the array type in the SyntaxItem we just built. */
    si = ArrayList.alternatives;
    si->flags |= RESULT_VALID;
    si->result.type = foundArrayType;
    si->result.u.taResult = arrayType;
    
    DPRT(("DEFINED ARRAY %s OF TYPE %s\n", si->syntaxString, si->result.u.taResult->syntaxString));
    
    return(NO_ERROR);
}

