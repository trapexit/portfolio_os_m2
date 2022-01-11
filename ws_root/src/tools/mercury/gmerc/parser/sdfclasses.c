/*
 * This file defines the ASCII SDF built-in classes in terms of a tree of Syntax structures.
 */
#include "syntax.h"
#include "parserfunctions.h"

/* A filename is optianally surrounded by quotes. */
SyntaxItem FileTypeItemAlt2[] =
{
    {
        "\"",
        1,
        (MATCH_RIGHT | SKIP_MATCHED),
        HandleFiles,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax FileType2 =
{
    1,
    0,
    DONT_REPEAT,
    &FileTypeItemAlt2[0],
    NULL,
    "File2",
};
SyntaxItem FileTypeItem[] =
{
    {
        "\"",
        1,
        (MATCH_LEFT | SKIP_MATCHED | REPEAT_WORD | CONTINUE_ON_ERR | RETURN),
        NULL,
        &FileType2,
        &FileTypeItem[1],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        HandleFiles,
        NULL,
        NULL,
        {NULL},
    },
};
Syntax FileType =
{
    2,
    0,
    DONT_REPEAT,
    &FileTypeItem[0],
    NULL,
    "File",
};

/* Conversion of ASCII to short is handled by strtol(), which will show
 * errors itself.
 */
SyntaxItem ShortItem[] =
{
    {
        NULL,
        0,
        0,
        ConvertShortInteger,
        NULL,
        NULL,
        {NULL},
    }
};
Syntax Short =
{
    0,
    1,
    DONT_REPEAT,
    NULL,
    &ShortItem[0],
    "Short",
};

/* Conversion of ASCII to Float is handled by strtod(), which will show
 * errors itself.
 */
SyntaxItem FloatItem[] =
{
    {
        NULL,
        0,
        0,
        ConvertFloat,
        NULL,
        NULL,
        {NULL},
    }
};
Syntax Float =
{
    0,
    1,
    DONT_REPEAT,
    NULL,
    &FloatItem[0],
    "Float",
};

SyntaxItem StringItem[] =
{
    {
        NULL,
        0,
        0,
        ConvertString,
        NULL,
        NULL,
        {NULL},
    }
};
Syntax String =
{
    0,
    1,
    DONT_REPEAT,
    NULL,
    &StringItem[0],
    "String",
};

/* An integer consists of "0x..." for a hex number,
 * "0.." for an octal number, or characters in the range 1-9 for a decimal number.
 */
SyntaxItem IntegerItem[] =
{
    {
        "0x",
        2,
        MATCH_LEFT,
        ConvertHexInteger,
        NULL,
        &IntegerItem[1],
        {NULL},
    },
    {
        "0",
        1,
        MATCH_LEFT,
        ConvertOctInteger,
        NULL,
        &IntegerItem[2],
        {NULL},
    },
    {
        NULL,
        0,
        0,
        ConvertDecInteger,
        NULL,
        NULL,
        {NULL},
    }
};
Syntax Integer =
{
    0,
    3,
    DONT_REPEAT,
    NULL,
    &IntegerItem[0],
    "Integer",
};

/* Store the list of names of type definitions off of here */
SyntaxItem InbuiltClassListItem[] =
{
    {
        "integer",
        7,
        0,
        NULL,
        &Integer,
        &InbuiltClassListItem[1],
        {
            BASE_TYPE,
            NULL,
        }
    },
    {
        "float",
        5,
        0,
        NULL,
        &Float,
        &InbuiltClassListItem[2],
        {
            BASE_TYPE,
            NULL,
        }
    },
    {
        "string",
        6,
        0,
        NULL,
        &String,
        &InbuiltClassListItem[3],
        {
            BASE_TYPE,
            NULL,
        }
    },
    {
        "file",
        4,
        0,
        NULL,
        &FileType,
        &InbuiltClassListItem[4],
        {
            BASE_TYPE,
            NULL,
        }
    },
    {
        "short",
        5,
        0,
        NULL,
        &Short,
        NULL,
        {
            BASE_TYPE,
            NULL,
        }
    },
};      
Syntax InbuiltClassList =
{
    0,
    5,
    DONT_REPEAT,
    NULL,
    &InbuiltClassListItem[0],
    "InbuiltClassList",
};
