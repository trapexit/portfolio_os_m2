/*
 * This file defines the ASCII SDF syntax in terms of a tree of Syntax structures.
 */
#include "syntax.h"
#include "parserfunctions.h"

extern Syntax Integer;
extern Syntax Float;
extern Syntax GetDefiningType;

/* Store the list of enum definitions off of here */
Syntax EnumList =
{
    0,
    0,
    DONT_REPEAT,
    NULL,
    NULL,
    "EnumList",
};

/* Store the list of class definitions off of here */
Syntax ClassList =
{
    0,
    0,
    DONT_REPEAT,
    NULL,
    NULL,
    "ClassList",
};

/* Store the list of array definitions off of here */
Syntax ArrayList =
{
    0,
    0,
    DONT_REPEAT,
    NULL,
    NULL,
    "ArrayList",
};


/************************************************************************************/


/* unit ::= "meters" | "kilometers" | "feet" | "inches" | "nautmiles" | integer */
SyntaxItem unitItem[] =
{
    {
        "meters",
        6,
        0,
        FoundMeters,
        NULL,                    /* A base type */
        &unitItem[1],
        {NULL},
    },
    {
        "kilometers",
        10,
        0,
        FoundKMeters,
        NULL,                    /* A base type */
        &unitItem[2],
        {NULL},
    },
    {
        "feet",
        4,
        0,
        FoundFeet,
        NULL,                    /* A base type */
        &unitItem[3],
        {NULL},
    },
    {
        "inches",
        6,
        0,
        FoundInches,
        NULL,                    /* A base type */
        &unitItem[4],
        {NULL},
    },
    {
        "nautmiles",
        9,
        0,
        FoundNautMiles,
        NULL,                    /* A base type */
        &unitItem[5],
        {NULL},
    },
    {
        NULL,                   /* Shows default */
        0,
        0,
        FoundUnitsInt,
        &Integer,
        NULL,
        {NULL},
    }
};
Syntax unit =
{
    0,
    6,
    DONT_REPEAT,
    NULL,
    &unitItem[0],
    "unit",
};   
        
/* Units ::= "units" <unit> */
SyntaxItem unitsItem[] =
{
    {
        "units",
        5,
        0,
        FoundUnits,
        &unit,
        NULL,
        {NULL},
     },
};
Syntax units =
{
    0,
    1,
    DONT_REPEAT,
    NULL,
    &unitsItem[0],
    "units",
};

/* "define array" sdf_newclass "of" sdf_type */
SyntaxItem ArrayBodyItem[] =
{
    {
        "of",
        2,
        0,
        NULL,
        NULL,
        &ArrayBodyItem[1],
        {NULL},
     },
    {
        "",       /* This will be the sdf_type name */
        0,
        /*CASE_SENSITIVE |*/ MATCH_LEFT,
        FoundArrayType,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax ArrayBody =
{
    2,
    0,
    DONT_REPEAT,
    &ArrayBodyItem[0],
    NULL,
    "ArrayBody",
};

SyntaxItem DefineArrayItem[] =
{
    {
        "",       /* This will be the sdf_newclass name */
        0,
        /*CASE_SENSITIVE |*/ MATCH_LEFT,
        FoundDefineArray,
        &ArrayBody,
        NULL,
        {NULL},
    }
};
Syntax DefineArray =
{
    1,
    0,
    DONT_REPEAT,
    &DefineArrayItem[0],
    NULL,
    "DefineArray",
};   

/* sdf_class_member ::= sdf_type sdf_string | "pad" integer */
SyntaxItem ClassMemberStringItem[] =
{
     {
        "",      /* This will be the sdf_string */
        0,
        /*CASE_SENSITIVE |*/ MATCH_LEFT,
        HandleClassString,
        NULL,
        NULL,
        {NULL},
     },
};
Syntax ClassMemberString =
{
    1,
    0,
    DONT_REPEAT,
    &ClassMemberStringItem[0],
    NULL,
    "ClassMemberString",
};
SyntaxItem ClassMemberItem[] =
{
    {
        "pad",
        3,
        0,
        HandleClassPad,
        &Integer,
        &ClassMemberItem[1],
        {NULL},
    },
    {
        "",      /* This will be the sdf_type */
        0,
        MATCH_LEFT,
        HandleClassType,
        &ClassMemberString,
        NULL,
        {NULL},
    },
};
Syntax ClassMember =
{
    0,
    2,
    REPEAT_UNTIL_BRACE,
    NULL,
    &ClassMemberItem[0],
    "ClassMember",
};

SyntaxItem ClassFromItem[] =
{
    {
        "",      /* This will be the sdf_class */
        0,
        /*CASE_SENSITIVE | */MATCH_LEFT,
        HandleClassFrom,
        NULL,
        &ClassFromItem[1],
        {NULL},
    },
    {
        "{",   /* Start of the sdf_class_body definition */
        1,
        0,
        NULL,
        &ClassMember,
        NULL,
        {NULL},
    },
};
Syntax ClassFrom =
{
    2,
    0,
    DONT_REPEAT,
    &ClassFromItem[0],
    NULL,
    "ClassFrom",
};

/* sdf_class_body ::= "{" sdf_class_member * "}" */
SyntaxItem ClassBodyItem[] =
{
    {
        "{",
        1,
        0,
        NULL,
        &ClassMember,
        &ClassBodyItem[1],
        {NULL},
    },
    {
        "from",
        4,
        0,
        NULL,
        &ClassFrom,
        NULL,
        {NULL},
    },
};
Syntax ClassBody =
{
    0,
    2,
    DONT_REPEAT,
    NULL,
    &ClassBodyItem[0],
    "ClassBody",
};

/* "define class" sdf_newclass sdf_class_body |
 * "define class" sdf_newclass "from" sdf_class sdf_body
 */
SyntaxItem DefineClassItem[] =
{
    {
        "",       /* This will be the sdf_newclass name */
        0,
        MATCH_LEFT,
        FoundDefineClass,
        &ClassBody,
        NULL,
        {NULL},
    }
};
Syntax DefineClass =
{
    1,
    0,
    DONT_REPEAT,
    &DefineClassItem[0],
    NULL,
    "DefineClass",
};    


/* sdf_enum_member ::= sdf_string integer */
SyntaxItem ParseEnumMemberItem[] =
{
    {
        "",      /* This will be the sdf_string */
        0,
        /*CASE_SENSITIVE |*/ MATCH_LEFT,
        HandleEnumDefinition,
        NULL,
        &ParseEnumMemberItem[1],
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
    }
};
Syntax ParseEnumMember =
{
    2,
    0,
    REPEAT_UNTIL_BRACE,
    &ParseEnumMemberItem[0],
    NULL,
    "ParseEnumMember",
};
/* "define enum" sdf_newclass "{" sdf_enum_member * "}"
 * DefineEnum will just parse out the sdf_newclass, and then move down
 * to the actual definition.
 */
SyntaxItem ParseEnumItem[] =
{
    {
        "{",
        1,
        0,
        NULL,
        &ParseEnumMember,
        NULL,
        {NULL},
    }
};
Syntax ParseEnum =
{
    1,
    0,
    DONT_REPEAT,
    &ParseEnumItem[0],
    NULL,
    "ParseEnum",
};

SyntaxItem DefineEnumItem[] =
{
    {
        "",       /* This will be the sdf_newclass name */
        0,
        /*CASE_SENSITIVE |*/ MATCH_LEFT,
        FoundDefineEnum,
        &ParseEnum,
        NULL,
        {NULL},
    }
};
Syntax DefineEnum =
{
    1,
    0,
    DONT_REPEAT,
    &DefineEnumItem[0],
    NULL,
    "DefineEnum",
};    

/* sdf_define ::= "define class" sdf_newclass sdf_class_body |
 *                    "define class" sdf_newclass "from" sdf_class sdf_class_body |
 *                    "define array" sdf_newclass "of" sdf_type |
 *                    "define enum" sdf_newclass "{" sdf_enum_member * "}" |
 *                    "define" sdf_type sdf_newsym sdf_value
 */
SyntaxItem DefineItem[] =
{
    {
        "class",
        5,
        0,
        IncrementClassCounter,
        &DefineClass,
        &DefineItem[1],
        {NULL},
    },
    {
        "array",
        5,
        0,
        NULL,
        &DefineArray,
        &DefineItem[2],
        {NULL},
    },
    {
        "enum",
        4,
        0,
        NULL,
        &DefineEnum,
        &DefineItem[3],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        &GetDefiningType,
        NULL,
        {NULL},
    },
};
Syntax Define = 
{
    0,
    4,
    DONT_REPEAT,
    NULL,
    &DefineItem[0],
    "Define",
};

/* sdf_include ::= "include" '"'filename'"' */
SyntaxItem IncludeItem[] =
{
    {
        "\"",
        1,
        (MATCH_LEFT | SKIP_MATCHED | CASE_SENSITIVE),
        NULL,
        NULL,
        &IncludeItem[1],
        {NULL},
    },
    {
        "\"",
        1,
        (MATCH_RIGHT | KEEP_WORD | SKIP_MATCHED | CASE_SENSITIVE),
        HandleInclude,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax Include =
{
    2,
    0,
    DONT_REPEAT,
    &IncludeItem[0],
    NULL,
    "Include",
};

/* sdf_item ::= sdf_units | sdf_define | sdf_include */
SyntaxItem itemItem[] =
{
    {
        "define",
        6,
        0,
        NULL,
        &Define,
        &itemItem[1],
        {NULL},
    },
     {
        "include",
        7,
        0,
        NULL,
        &Include,
        &itemItem[2],
        {NULL},
    },
   {
        NULL,
        0,
        0,
        NULL,
        &units,
        NULL,
        {NULL},
    },
};
Syntax item =
{
    0,
    3,
    REPEAT,
    NULL,
    &itemItem[0],
    "item",
};

/* File is where it all starts */
/* file ::= sdf_version sdf_item.
 * sdf_version ::= SDFVERSION integer
 */ 
SyntaxItem FileItem[] =
{
    {
        "sdfversion",
        10,
        0,
        FoundSDFVersion,
        &Float,
        &FileItem[1],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        NULL,
        &item,
        NULL,
        {NULL},
    },
};
Syntax File =
{
    2,
    0,
    DONT_REPEAT,
    &FileItem[0],
    NULL,
    "File",
};

