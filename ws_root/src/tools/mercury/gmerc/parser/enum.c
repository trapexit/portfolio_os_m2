#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "parserfunctions.h"
#include "parsererrors.h"
#include "parsertypes.h"

extern Syntax EnumList;
extern Syntax ClassList;
extern Syntax ArrayList;
extern Syntax InbuiltClassList;
extern Syntax Bitmask;

extern int32 *integerResult;
extern bool incBufferPosition;
extern TokenBuffer *tb;
Err CheckUniqueName(Syntax *syn, char *word, uint32 charCount, SyntaxItem **found);
Err ReturnEnumVal(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

/* CheckUniqueName() returns DUPLICATE_NAME if the name was found in the current list.
 * This is a case sensitive search.
 */
Err CheckUniqueName(Syntax *syn, char *word, uint32 charCount, SyntaxItem **found)
{
    int32 i;
    SyntaxItem *si;
    
    for (i = 0, si = syn->alternatives; i < syn->countAlt; i++)
    {
        if ((charCount == si->stringLen) &&
            (strncmp(word, si->syntaxString, charCount) == 0))
        {
            if (found)
            {
                *found = si;
            }
            return(DUPLICATE_NAME);
        }
        si = si->nextSI;
    }
    return(NO_ERROR);
}

/*************************************************/

Err ReturnEnumVal(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;
    
    if (integerResult)
    {
        *integerResult = sr->u.iResult;
        if (incBufferPosition)
        {
            INC_BUFFER_POS(uint32);
            incBufferPosition = FALSE;
        }            
        DPRT(("FOUND ENUMERATED VALUE %d\n", *integerResult));
    }
    else
        DPRT(("**** found enum value %d\n", sr->u.iResult));
    
    return(NO_ERROR);
}

/* This code builds the parser table for all enum definitions.
 * The enum is broken down as follows. For the following example:
 *
 * define enum AnimKind
 * {
 *    Once 0
 *    Cycle 1
 *    Swing 2
 * }
 *
 * the sdf_newclass AnimKind is added to a list of alternatives in EnumList,
 * with the string "AnimKind" being the syntaxString. A parse list is built at the
 * next level down with all the alternative strings "Once", "Cycle" and "Swing" each
 * setting the SyntaxResult to the enumerated integer.
 */
 
/*
 * FoundDefineEnum() will add this named enum to the EnumList definitions
 */
Err FoundDefineEnum(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *first;
    Err err;

    /* See if this name is already defined */
    if (((err = CheckUniqueName(&EnumList, wordIn, charCount, NULL)) < NO_ERROR) ||
        ((err = CheckUniqueName(&ClassList, wordIn, charCount, NULL))  < NO_ERROR) ||
        ((err = CheckUniqueName(&ArrayList, wordIn, charCount, NULL)) < NO_ERROR) ||
        ((err = CheckUniqueName(&InbuiltClassList, wordIn, charCount, NULL))  < NO_ERROR))
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
    si->flags = 0; /*CASE_SENSITIVE;*/
    si->syntaxFn = SetEnumDestination;
    si->nextLevel = NULL;    /* HandleEnumDefinition() will change this */

    /* Now add this to the list */
    first = EnumList.alternatives;
    EnumList.alternatives = si;
    si->nextSI = first;
    EnumList.countAlt++;

    DPRT(("FOUND DEFINE ENUM %s\n", si->syntaxString));
    return(NO_ERROR);
}

/*
 * HandleEnumDefinition() will add this named enum member to the enum definition.
 */
#define ENUM_NAME " Enum Types"
Err HandleEnumDefinition(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *first;
    Syntax *syn;
    Err err;

    syn = EnumList.alternatives->nextLevel;  /* This is the list we're building on */
    if (syn == NULL)
    {
        /* Must be the first one */
        syn = (Syntax *)mymalloc(sizeof(Syntax) + (strlen(ENUM_NAME) + 1) +
                               EnumList.alternatives->stringLen);
        if (syn == NULL)
        {
            return(NO_MEMORY);
        }
        syn->countReq = syn->countAlt = 0;
        syn->repeatFlags = DONT_REPEAT;
        syn->required = syn->alternatives = NULL;
        syn->name = (char *)((uint32)syn + sizeof(Syntax));
        /* Make the name "<enum name> Enum Types" */
        strncpy(syn->name, EnumList.alternatives->syntaxString, EnumList.alternatives->stringLen);
        strcpy((syn->name + EnumList.alternatives->stringLen), ENUM_NAME);
        EnumList.alternatives->nextLevel = syn;

        /* enum rules allow for an optional "(", which would lead down to the
         * bitmask parser.
         */
        si = mymalloc(sizeof(SyntaxItem) + 2);
        if (si == NULL)
        {
            return(NO_MEMORY);
        }
        si->syntaxString = "(";
        si->stringLen = 1;
        si->flags = RESULT_VALID;
        si->syntaxFn = BuildBitMask; /* Sets the type of data to parse */
        si->nextLevel = &Bitmask;
        si->nextSI = NULL;
        si->result.type = SERESULT;
        si->result.u.seResult = syn;     /* BuildBitMask will use this to know which type
                                         * to parse to. We can't point directly to the SyntaxItem
                                         * because we don't know yet where the first item in this
                                         * SyntaxItem list will be (this Item will eventually be the
                                         * last in the list.
                                         */
        syn->alternatives = si;
        syn->countAlt++;
    }
    else
    {
            /* See if this name is already defined */
        err = CheckUniqueName(syn, wordIn, charCount, NULL);
        if (err < NO_ERROR)
        {
            return(err);
        }
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
    si->syntaxFn = ReturnEnumVal;
    si->nextLevel = NULL;

    /* Get the result ready */
    si->flags = (/*CASE_SENSITIVE | */RESULT_VALID);
    si->result.type = IRESULT;
    integerResult = &si->result.u.iResult;
    
    /* Link this in to the list */
    first = syn->alternatives;
    syn->alternatives = si;
    si->nextSI = first;
    syn->countAlt++;
    
    DPRT(("ENUM MEMBER %s\n", si->syntaxString));
    
    return(NO_ERROR);
}
