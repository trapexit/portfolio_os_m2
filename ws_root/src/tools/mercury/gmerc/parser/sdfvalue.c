/*
 * This file parses the values of the SDF item
 */
#include "syntax.h"
#include "parserfunctions.h"

extern Syntax Integer;

SyntaxItem QuotedStringItem[] =
{
    {
        "\"",
        1,
        (TO_QUOTE | MATCH_RIGHT | SKIP_MATCHED),
        FoundNewSym,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax QuotedString =
{
    1,
    0,
    DONT_REPEAT,
    &QuotedStringItem[0],
    NULL,
    "QuotedString",
};
SyntaxItem OptionallyQuotedStringItem[] =
{
    {
        "\"",
        1,
        (MATCH_LEFT | SKIP_MATCHED | REPEAT_WORD | CONTINUE_ON_ERR | RETURN),
        NULL,
        &QuotedString,
        &OptionallyQuotedStringItem[1],
        {NULL},
    },
    {                           /* String was not quoted */
        NULL,
        0,
        0,
        FoundNewSym,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax OptionallyQuotedString =
{
    2,
    0,
    DONT_REPEAT,
    &OptionallyQuotedStringItem[0],
    NULL,
    "OptionallyQuotedString",
};

/* sdf_number ::= float | integer */
SyntaxItem NumberItem[] =
{
    {
        NULL,
        0,
        0,  /*CASE_SENSITIVE*/
        NULL,
        NULL, /* FoundNewSym() will point
               * down to the class to check.
               */
        NULL,
        {NULL},
    },
};
Syntax Number =
{
    1,
    0,
    DONT_REPEAT,
    &NumberItem[0],
    NULL,
    "Number",
};

/* sdf_bitmask ::= "(" sdf_bit [ "|" sdf_bit ] * ")" | sdf_bit
 * sdf_bit ::= sdf_enum_string | integer
 */
SyntaxItem BitmaskItem[] =
{
    {
        ")",
        1,
        RETURN,
        EndBitMask,
        NULL,
        &BitmaskItem[1],
         {NULL},
    },
    {
        "|",
        1,
        0,
        OrBitValue,
        NULL,
        &BitmaskItem[2],
         {NULL},
    },
    {
        NULL,
        0,
        /*CASE_SENSITIVE | */CONTINUE_ON_ERR,
        NULL,
        NULL,
        &BitmaskItem[3],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        &Integer,
        NULL,
        {NULL},
    },
};
Syntax Bitmask =
{
    0,
    4,
    REPEAT,
    NULL,
    &BitmaskItem[0],
    "Bitmask",
};

/*
 * sdf_instance ::= sdf_object | sdf_array
 * sdf_object ::= "{" sdf_object_element * "}"
 * sdf_array ::= "{" sdf_array_element * "}"
 *
 * sdf_object_element ::=
 *    "use" sdf_member sdf_symbol |
 *    "define" sdf_member sdf_newsym sdf_value |
 *    sdf_member "use" sdf_symbol |
 *    sdf_member sdf_value
 *
 * sdf_array_element ::=
 *    "use" sdf_symbol |
 *    "define" sdf_newsym sdf_value -- doesn't work in original sdf parser
 *    sdf_arraymem sdf_value
 *    sdf_value
 */

SyntaxItem BuildClassItem[] =
{
    {
        NULL,
        0,
        0,
        NULL,
        &OptionallyQuotedString,
        &BuildClassItem[1],
        {NULL},
    },
    {
        "{",
        1,
        0,
        OpenBrace,
        NULL,   /* GetClassType() sets this */
        NULL,
        {NULL},
    },
    {
        "}",
        1,
        RETURN,
        CloseBrace,
        NULL, 
        &BuildClassItem[3],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        NULL,   /* GetClassType() sets this */
        NULL,
        {NULL},
    },
};
Syntax BuildClass =
{
    2,
    2,
    REPEAT | REPEAT_ONLY_ALT,
    &BuildClassItem[0],
    &BuildClassItem[2],
    "BuildClass",
};

SyntaxItem UseObjectQuotedNameItem[] =
{
    {
        "\"",
        1,
        (TO_QUOTE | MATCH_RIGHT | SKIP_MATCHED),
        UseObjectSymbol,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax UseObjectQuotedName =
{
    1,
    0,
    DONT_REPEAT,
    &UseObjectQuotedNameItem[0],
    NULL,
    "UseObjectQuotedName",
};
SyntaxItem UseObjectNameItem[] =
{
    {
        "\"",
        1,
        (MATCH_LEFT | SKIP_MATCHED | REPEAT_WORD | CONTINUE_ON_ERR | RETURN),
        NULL,
        &UseObjectQuotedName,
        &UseObjectNameItem[1],
        {NULL},
    },
    {                           /* String was not quoted */
        NULL,
        0,
        0,
        UseObjectSymbol,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax UseObjectName =
{
    2,
    0,
    DONT_REPEAT,
    &UseObjectNameItem[0],
    NULL,
    "UseObjectName",
};
SyntaxItem QuotedObjectItem[] =
{
    {
        "\"",
        1,
        (TO_QUOTE | MATCH_RIGHT | SKIP_MATCHED),
        UseObjectType,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax QuotedObject =
{
    1,
    0,
    DONT_REPEAT,
    &QuotedObjectItem[0],
    NULL,
    "QuotedObject",
};
SyntaxItem OptionallyQuotedObjectItem[] =
{
    {
        "\"",
        1,
        (MATCH_LEFT | SKIP_MATCHED | REPEAT_WORD | CONTINUE_ON_ERR | RETURN),
        NULL,
        &QuotedObject,
        &OptionallyQuotedObjectItem[1],
        {NULL},
    },
    {                           /* String was not quoted */
        NULL,
        0,
        0,
        UseObjectType,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax OptionallyQuotedObject =
{
    2,
    0,
    DONT_REPEAT,
    &OptionallyQuotedObjectItem[0],
    NULL,
    "OptionallyQuotedObject",
};
SyntaxItem UseObjectItem[] =
{
    {
        NULL,
        0,
        0,
        NULL,
        &OptionallyQuotedObject,
        &UseObjectItem[1],
        {NULL},
    },
    {             /* not quoted object */
        NULL,
        0,
        0,
        NULL,
        &UseObjectName,
        NULL,
        {NULL},
    },       
};
Syntax UseObject =
{
    2,
    0,
    REPEAT | REPEAT_ONLY_ALT, 
    &UseObjectItem[0],
    NULL,
    "UseObject",
};


SyntaxItem BuildObject2Item[] =
{
    {
        "}",
        1,
        RETURN,
        CloseBrace,
        NULL, 
        &BuildObject2Item[1], 
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        NULL,   /* HandleClassString() sets this before copying it*/
        NULL, 
        {NULL},
    },
};
Syntax BuildObject2 =
{
    0,
    2,
    REPEAT,
    NULL,
    &BuildObject2Item[0], /* HandleClassString() sets this to point
                           * to the copy of BuildObject2Item[0],
                           */
    "BuildObject2",
};
/* BuildObject will be copied into every Class parse list
 * by HandleClassString().
 */
SyntaxItem BuildObjectReqItem[] =
{
    {
        "{",
        1,
        CONTINUE_ON_ERR | RETURN,
        FoundClassBrace,
        &BuildObject2,
        NULL,
        {NULL},
    },
};
extern Syntax GetDefiningType;
SyntaxItem BuildObjectItem[] =
{
    {
        "use",
        3,
        0,
        NULL,
        &UseObject,
        &BuildObjectItem[1],
        {NULL},
    },
    {
        "define",
        6,
        0,
        NULL,
        &GetDefiningType,
        NULL,
        {NULL},
    },
};
Syntax BuildObject =
{
    1,
    2,
    DONT_REPEAT,
    &BuildObjectReqItem[0],
    &BuildObjectItem[0],
    "BuildObject",
};

/****************************************************************/
SyntaxItem UseArrayItem[] =
{
    {
        "",     /* sdf_symbol */
        0,
        MATCH_LEFT,
        UseArraySymbol,
        NULL, 
        NULL,
        {NULL},
    },
};
Syntax UseArray =
{
    1,
    0,
    DONT_REPEAT,
    &UseArrayItem[0],
    NULL,
    "UseArray",
};

SyntaxItem ParseUseArraySymbolItem[] =
{
    {
        "use",
        3,
        0,
        NULL,
        &UseArray,
        &ParseUseArraySymbolItem[1],
        {NULL},
    },
   {
        "{",
        1,
        REPEAT_WORD | RETURN,
        NULL,
        NULL,
        &ParseUseArraySymbolItem[2],
        {NULL},
    },
   {
        "",  /* Just garbage here */
        0,
        MATCH_LEFT,
        WarnGarbage,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax ParseUseArraySymbol =
{
    0,
    3,
    REPEAT,
    NULL,
    &ParseUseArraySymbolItem[0],
    "ParseUseArraySymbol",
};

/* Array type is followed by the Symbol name and a {.
 *  The actual array data must also be enclosed in { }.
 */
SyntaxItem ParseArrayDataItem[] =
{
    {
        ",",    /* There is an optional "," between numbers */
        1,
        0,
        NULL,
        NULL,
        &ParseArrayDataItem[1],
        {NULL},
    },
    {
        "}", 
        1,
        RETURN | REPEAT_WORD,
        NULL,
        NULL,
        &ParseArrayDataItem[2],
        {NULL},
    },
    {
        NULL,    /* sdf_value */
        0,
        0,
        FoundInstanceArrayType,
        NULL,  /* ParseArray() sets this to the type of data to parse */
        NULL,
        {NULL},
    },
};
Syntax ParseArrayData =
{
    0,
    3,
    REPEAT,
    NULL,
    &ParseArrayDataItem[0],
    "ParseArrayData",
};

/* ParseArrayArray parses an array of arrays */
SyntaxItem ParseArrayArrayDataItem[] =
{
    {
        NULL,
        0,
        0,
        NULL,
        &ParseUseArraySymbol,
        &ParseArrayArrayDataItem[1],
        {NULL},
    },
    {
        "{",
        1,
        0,
        OpenBrace,
        NULL,
        NULL,
        {NULL},
    },
    {
        ",",    /* There is an optional "," between numbers */
        1,
        0,
        NULL,
        NULL,
        &ParseArrayArrayDataItem[3],
        {NULL},
    },
    {
        "}", 
        1,
        RETURN,
        CloseBrace,
        NULL,
        &ParseArrayArrayDataItem[4],
        {NULL},
    },
    {
        NULL,    /* sdf_value */
        0,
        0,
        FoundInstanceArrayType,
        NULL,  /* ParseArray() sets this to the type of data to parse */
        NULL,
        {NULL},
    },
};
Syntax ParseArrayArrayData =
{
    2,
    3,
    REPEAT | REPEAT_ONLY_ALT,
    &ParseArrayArrayDataItem[0],
    &ParseArrayArrayDataItem[2],
    "ParseArrayArrayData",
};
SyntaxItem ParseArrayArrayItem[] =
{
    {
        "}",
        1,
        RETURN | REPEAT_WORD | CONTINUE_ON_ERR,
        NULL,
        NULL,
        &ParseArrayArrayItem[1],
         {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        &ParseArrayArrayData,
        NULL,
         {NULL},
    },
};
Syntax ParseArrayArray =
{
    2,
    0,
    REPEAT,
    &ParseArrayArrayItem[0],
    NULL,
    "ParseArrayArray",
};

SyntaxItem GetArraySymbol2Item[] =
{
     {
        "}",
        1,
        RETURN | CONTINUE_ON_ERR,
        CloseBrace,
        NULL, 
        &GetArraySymbol2Item[1],
        {NULL},
    },
   {
        NULL,
        0,
        0,
        NULL,
        NULL,  /* This will be set by ParseArray()*/
        NULL,
        {NULL},
    },
};
Syntax GetArraySymbol2 =
{
    2,
    0,
    REPEAT,
    &GetArraySymbol2Item[0],
    0,
    "GetArraySymbol2",
};
SyntaxItem GetArraySymbolItem[] =
{
    { 
        "{",
        1,
        CONTINUE_ON_ERR | RETURN,
        OpenBrace,
        &GetArraySymbol2,
        &GetArraySymbolItem[1],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        &OptionallyQuotedString,
        &GetArraySymbolItem[2],
        {NULL},
    },
    {
        "{",
        1,
        0,
        OpenBrace,
        NULL,  /* This will be set by ParseArray()*/
        &GetArraySymbolItem[3],
        {NULL},
    },
    {
        "}",
        1,
        0,
        CloseBrace,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax GetArraySymbol =
{
    4,
    0,
    DONT_REPEAT,
    &GetArraySymbolItem[0],
    NULL,
    "GetArraySymbol",
};


extern Syntax EnumList;
extern Syntax ArrayList;
extern Syntax ClassList;
extern Syntax InbuiltClassList;

SyntaxItem GetDefiningTypeItem[] =
{
    {
        NULL,    /* This will be the name of the sdf_newsym */
        0,
        CONTINUE_ON_ERR,
        0,
        &EnumList,  /* Is this an Enum? */
        &GetDefiningTypeItem[1],
         {NULL},
    },
    {
        NULL,    /* This will be the name of the sdf_newsym */
        0,
        CONTINUE_ON_ERR,
        0,
        &ArrayList,  /* Is this an Array? */
        &GetDefiningTypeItem[2],
         {NULL},
    },
    /* Class types are a special case */
    {
        "",    /* This will be the name of the sdf_newsym */
        0,
        MATCH_LEFT,
        GetClassType,
        &BuildClass,
        NULL,
         {NULL},
    },
   
};
Syntax GetDefiningType =
{
    0,
    3,
    DONT_REPEAT,
    NULL,
    &GetDefiningTypeItem[0],
    "GetDefiningType",
};
