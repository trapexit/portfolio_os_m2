/*
 * This file defines the structure used to define the syntax being parsed.
 */
#ifndef _SYNTAX_H_
#define _SYNTAX_H_

#include "parsertypes.h"

typedef struct SyntaxResult
{
    uint32 type;
    uint32 id;    /* Unique ID (see sdftokens.h) */
    union
    {
        int32 iResult;  /* integer */
        float fResult;   /* float */
        char cResult;  /* character */
        char *sResult; /* string */
        void *pResult; /* pointer */
        struct SyntaxItem *taResult; /* array data type */
        struct SyntaxItem *teResult; /* enum data type */
        struct SyntaxItem *tcResult; /* class data type */
        struct Syntax *saResult;       /* Syntax of array data type */
        struct Syntax *seResult;       /* Syntax of enum data type */
        struct Syntax *scResult;       /* Syntax of class data type */
    } u;
} SyntaxResult;

enum
{
    IRESULT = 0,
    FRESULT = 1,
    CRESULT = 2,
    SRESULT = 3,
    PRESULT = 4,
    TARESULT = 5,
    TERESULT = 6,
    TCRESULT = 7,
    SARESULT = 8,
    SERESULT = 9,
    SCRESULT = 10
};
enum
{
    ARRAY_TYPE = TARESULT,
    ENUM_TYPE = TERESULT,
    CLASS_TYPE = TCRESULT,
    BASE_TYPE = 100
};

typedef struct SyntaxItem
{
    char *syntaxString;     /* The string we are looking for */
    uint32 stringLen;        /* pre-calculate the strlen - saves some time */
    uint32 flags;              /* Special flags for string matching -- see below */
    Err (*syntaxFn)(char **,char *,unsigned long ,struct SyntaxResult*);       /* Function to call to do something with this info */
    struct Syntax *nextLevel;    /* syntaxString must be followed by something in the next syntactic level */
    struct SyntaxItem *nextSI;   /* Pointer to the next SyntaxItem in the list */
    SyntaxResult result;   /* Any root data */
} SyntaxItem;

typedef struct Syntax
{
    uint32 countReq;       /* Number of required tokens at this level */
    uint32 countAlt;        /* Number of alternative tokens at this level */
    uint32 repeatFlags;   /* See below */
    SyntaxItem *required;      /* Array of syntaxes that must be present */
    SyntaxItem *alternatives; /* Array of alternative syntax at this level */
    char *name;             /* name of this rule - for error reporting */
} Syntax;

/* String matching flags */
enum SyntaxItemFlags
{
    MATCH_LEFT = 0x1,         /* Match the first stringLen chars */
    MATCH_RIGHT = 0x2,        /* Match the last stringLen chars */
    KEEP_WORD = 0x4,           /* Keep this word for the next word */
    SKIP_MATCHED = 0x8 ,     /* Skip over the matched characters */
    CASE_SENSITIVE = 0x10,
    SKIP_THIS = 0x20,              /* Skip over this SyntaxItem */
    RESULT_VALID = 0x40,      /* If the SyntaxResult field is valid */
    CONTINUE_ON_ERR = 0x80, /* Don't return on error, but continue to next rule */
    RETURN = 0x100,              /* Always return if this rule matches */
    REPEAT_WORD = 0x200,  /* Parse the same word at the next level */
    ID_VALID = 0x400,            /* SyntaxResult.ID is valid */
    TO_QUOTE = 0x800       /* Use all the characters up to the quote character  */
};

enum RepeatFlags
{
    DONT_REPEAT = 1,
    REPEAT = 2,    /* After matching, repeat this rule again */
    REPEAT_UNTIL_BRACE = 4,
    KEEP_UNMATCHED = 8,
    REPEAT_ONLY_REQ = 0x10,
    REPEAT_ONLY_ALT = 0x20
};

typedef struct Symbol
{
    char *symbolString;
    uint32 stringLen;        /* pre-calculate the strlen - saves some time */
    SyntaxItem *type;    /* This symbol's data type */
    SyntaxResult data;   /* Any symbol data */
    struct Symbol *next; /* the next symbol in the list */
} Symbol;

#define COMMENT_CHAR '#'
#define SPECIAL_CHARS "{}()|#,"
#define SPECIAL_CHARS_EXCLUDE SPECIAL_CHARS /*")}|#,"*/
#endif

