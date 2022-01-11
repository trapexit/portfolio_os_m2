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
extern TokenBuffer *tb;
Err CheckUniqueName(Syntax *syn, char *word, uint32 charCount, SyntaxItem **found);
char *GetCurrentFileName(void);

SyntaxItem *foundEnum = NULL;
SyntaxItem *foundClass = NULL;
uint32 foundClassType;
uint32 parseClassReqCount;

#define CLASS_INC 0x10000
uint32 classCount = 0;
uint32 dataCount;

/*
 * The class is broken down as follows. For the following example:
 *
 * define class MyAnimation
 * {
 *    AnimKind Kind
 *    integer Speed
 * }
 *
 * the sdf_newclass MyAnimation is added to a list of alternatives in ClassList,
 * with the string "MyAnimation" being the syntaxString. A parse list is built at the
 * next level down with all the alternative strings "Kind" and "speed", which in turn
 * parse down to their enumeration parser in the EnumList.
 *
 * If inheriting, then the inheriting class becomes a superset of the inherited class.
 * For example:
 *
 * define class superClass from MyAnimation
 * {
 *      AnimKind morestuff
 * }
 *
 * the sdf_newclass superClass is added to the list of alternatives in ClassList, with
 * the string "superClass" being the syntaxString. We then build a parse list at the next
 * level down, with the first alternative pointing to the first alternative in the MyAnimation
 * list. The new alternative string "morestuff" is then added to the superClass list, so
 * superClass list now becomes "morestuff", "Speed" and "Kind".
 */

Err IncrementClassCounter(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    classCount += CLASS_INC;
    dataCount = 1;
    
    return(NO_ERROR);
}
/*
 * FoundDefineClass() will add this named class to the ClassList definitions
 */
Err FoundDefineClass(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
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
    si->syntaxFn = NULL;   /* Will go to BuildClass() -- SAS */
    si->nextLevel = NULL;    /* HandleClassDefinition() will change this */
    si->result.id = classCount;
    
    /* Now add this to the list */
    first = ClassList.alternatives;
    ClassList.alternatives = si;
    si->nextSI = first;
    ClassList.countAlt++;

    DPRT(("FOUND DEFINE CLASS %s\n", si->syntaxString));
    return(NO_ERROR);
}

/*
 * HandleClassType() will see if this string is in the list of known enums or types.
 */
Err HandleClassType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;

    if (((foundClassType = BASE_TYPE) && (CheckUniqueName(&InbuiltClassList, wordIn, charCount, &foundClass) != DUPLICATE_NAME)) &&
        ((foundClassType = ENUM_TYPE) && (CheckUniqueName(&EnumList, wordIn, charCount, &foundClass) != DUPLICATE_NAME)) &&
        ((foundClassType = ARRAY_TYPE) && (CheckUniqueName(&ArrayList, wordIn, charCount, &foundClass) != DUPLICATE_NAME)) &&
        ((foundClassType = CLASS_TYPE) && (CheckUniqueName(&ClassList, wordIn, charCount, &foundClass) != DUPLICATE_NAME)))
    {
        /* We don't know what this is!! */
        return(UNKNOWN_TYPE);
    }

    DPRT(("FOUND CLASSTYPE %s (type %d)\n", foundClass->syntaxString, foundClassType));
    return(NO_ERROR);
}

/*
 * HandleClassString() will hang this string off of the ClassList we are building,
 * and then point down to the found enumerated type.
 */
extern Syntax BuildObject;
extern Syntax BuildObject2;
extern SyntaxItem BuildObjectItem[];
extern SyntaxItem BuildObjectReqItem[];
extern SyntaxItem BuildObject2Item[];
Err DisableGetSym(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
#define CLASS_NAME " Class Types"
Err HandleClassString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *first, *bo2i;
    Syntax *syn, *bo2;
    int32 i;
    Err err;

    syn = ClassList.alternatives->nextLevel;  /* This is the list we're building on */
    if (syn == NULL)
    {
        /* Must be the first one */
        syn = (Syntax *)mymalloc(sizeof(Syntax) + (strlen(CLASS_NAME) + 1) +
                               ClassList.alternatives->stringLen);
        if (syn == NULL)
        {
            return(NO_MEMORY);
        }
        syn->countReq = syn->countAlt = 0;
        syn->repeatFlags = BuildObject.repeatFlags;
        syn->required = syn->alternatives = NULL;
        syn->name = (char *)((uint32)syn + sizeof(Syntax));
        /* Make the name "<class name> Class Types" */
        strncpy(syn->name, ClassList.alternatives->syntaxString, ClassList.alternatives->stringLen);
        strcpy((syn->name + ClassList.alternatives->stringLen), CLASS_NAME);
        ClassList.alternatives->nextLevel = syn;

        /* When we parse the usage of this class, we want the form:
         * define <class type> <symbol name>
         * {
         *    <entry type> <data> |
         *    use <class type> <symbol name> |
         *    define <class type> <symbol name> <value>
         * }
         *
         * BuildObject contains the rules for parsing "use" and "define", so
         * copy them into the alternative list of class types.
         */
        for (i = 0; i < BuildObject.countAlt; i++)
        {
            si = mymalloc(sizeof(SyntaxItem) + BuildObjectItem[i].stringLen + 1);
            if (si == NULL)
            {
                return(NO_MEMORY);
            }
            *si = BuildObjectItem[i];
            first = syn->alternatives;
            syn->alternatives = si;
            si->nextSI = first;
            syn->countAlt++;
        }

        /* BuildObjectReqItem[0] is goinf to parse a '{' character, and then parse
         * down to BuildObject2. BuildObject2 also has to hang off of this syntax because
         * its nextLevel must point back to this syntax structure.
         */
        BuildObject2Item[1].nextLevel = syn;
        first = NULL;
        for (i = (BuildObject2.countAlt - 1); i >= 0; --i)
        {
            bo2i = (SyntaxItem *)mymalloc(sizeof(SyntaxItem));
            if (bo2i == NULL)
            {
                return(NO_MEMORY);
            }
            *bo2i = BuildObject2Item[i];
            bo2i->nextSI = first;
            first = bo2i;
        }
        
        bo2 = (Syntax *)mymalloc(sizeof(Syntax));
        if (bo2 == NULL)
        {
            return(NO_MEMORY);
        }
        *bo2 = BuildObject2;
        bo2->alternatives = bo2i;
        
        BuildObjectReqItem[0].flags |= RESULT_VALID;
        BuildObjectReqItem[0].result.type = SCRESULT;
        BuildObjectReqItem[0].result.u.scResult = syn;
        BuildObjectReqItem[0].nextLevel = bo2;
        for (i = 0; i < BuildObject.countReq; i++)
        {
            si = mymalloc(sizeof(SyntaxItem) + BuildObjectReqItem[i].stringLen + 1);
            if (si == NULL)
            {
                return(NO_MEMORY);
            }
            *si = BuildObjectReqItem[i];
            first = syn->required;
            syn->required = si;
            si->nextSI = first;
            syn->countReq++;
        }
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
    si->flags = RESULT_VALID | ID_VALID;
    si->syntaxFn = ((foundClassType == ARRAY_TYPE) ? ParseArray :
                    (foundClassType == ENUM_TYPE) ? SetEnumDestination :
                    (foundClassType == BASE_TYPE) ? WriteDataType :
                    WriteDataID);
    si->nextLevel = foundClass->nextLevel;  /* This is where we will parse down to. */
    si->result.id = (classCount + dataCount++);
    si->result.type = foundClassType;
    si->result.u.taResult = foundClass;
    foundClass = NULL;              /* Stop potential silly bugs!! */

        /* Link this in to the list */
    first = syn->alternatives;
    syn->alternatives = si;
    si->nextSI = first;
    syn->countAlt++;
    
    DPRT(("CLASS SDF_STRING %s\n", si->syntaxString));
    return(NO_ERROR);
}

/*
 * HandleClassPad() sets up integerResult to put the integer pad value in the
 * result field of the SyntaxItem structure of this sdf_newclass.
 */
Err HandleClassPad(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    integerResult = &ClassList.alternatives->result.u.iResult;
    ClassList.alternatives->result.type = IRESULT;
    ClassList.alternatives->flags |= RESULT_VALID;
    DPRT(("Setting the Pad value of class %s\n", ClassList.alternatives->syntaxString));
    return(NO_ERROR);
}

/*
 * HandleClassFrom() checks to see if the class we are inheriting is defined. */
Err HandleClassFrom(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    SyntaxItem *si, *first;
    Syntax *syn, *parent;
    Err err;

    err = CheckUniqueName(&ClassList, wordIn, charCount, &foundClass);
    if (err != DUPLICATE_NAME)
    {
        /* We don't know what this is!! */
        return(UNKNOWN_TYPE);
    }
    parent = foundClass->nextLevel;
    
    syn = (Syntax *)mymalloc(sizeof(Syntax) + (strlen(CLASS_NAME) + 1) +
                           ClassList.alternatives->stringLen);
    if (syn == NULL)
    {
        return(NO_MEMORY);
    }
        /* This class will inherit it's parent's features */
    syn->countReq = parent->countReq;
    syn->countAlt = parent->countAlt;
    syn->repeatFlags = parent->repeatFlags;
    syn->alternatives = parent->alternatives;
    syn->required = parent->required;
    syn->name = (char *)((uint32)syn + sizeof(Syntax));
        /* Make the name "<class name> Class Types" */
    strncpy(syn->name, ClassList.alternatives->syntaxString, ClassList.alternatives->stringLen);
    strcpy((syn->name + ClassList.alternatives->stringLen), CLASS_NAME);
    
    ClassList.alternatives->nextLevel = syn;
     
    DPRT(("DEFINING %s FROM CLASSTYPE %s\n", ClassList.alternatives->syntaxString, foundClass->syntaxString));
    return(NO_ERROR);
}

extern SyntaxItem *foundType;
extern SyntaxItem BuildClassItem[];
Err GetClassType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;

        /* Do we know what class this is? */
    if (((err = CheckUniqueName(&ClassList, wordIn, charCount, &foundType)) != DUPLICATE_NAME) &&
        ((err = CheckUniqueName(&InbuiltClassList, wordIn, charCount, &foundType)) != DUPLICATE_NAME))
    {
        return(UNKNOWN_TYPE);
    }

    BuildClassItem[1].nextLevel = foundType->nextLevel;
    BuildClassItem[3].nextLevel = foundType->nextLevel;
    DPRT(("WILL PARSE CLASS %s\n", foundType->nextLevel->name));

    /* Put the ID value in the buffer */
    if (foundType->flags & ID_VALID)
    {
        WRITE_TOKEN(uint32, foundType->result.id);

        /* Is there a pad value? */
        if (foundType->flags & RESULT_VALID)
        {
            WRITE_TOKEN(uint32, CLASS_PAD);
            WRITE_TOKEN(int32, foundType->result.u.iResult);
        }
    }
    else
    {
        printf("=== WARNING === Should never see this!\n");
    }

    return(NO_ERROR);
}

Err FoundClassBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr)
{
    Err err;

    DPRT(("**** Found brace. next level = %s\n", sr->u.scResult->name));
    err = OpenBrace(buff, wordIn, charCount, sr);
    return(err);
}
