/* @(#) es.c 96/05/17 1.6 */

/* Spanish language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


/*****************************************************************************/


typedef struct LigatureMap
{
    unichar lm_Ligature;
    unichar lm_SecondChar;
} LigatureMap;


/*****************************************************************************/


/* locally used only, always masked out before returning to the user */
#define INTL_ATTRF_LIGATURE (1 << 7)


static const uint8 charAttrs[256] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    INTL_ATTRF_SPACE,              /* " " */
    INTL_ATTRF_PUNCTUATION,        /* "!" */
    INTL_ATTRF_PUNCTUATION,        /* '"' */
    0,                             /* "#" */
    0,                             /* "$" */
    INTL_ATTRF_PUNCTUATION,        /* "%" */
    INTL_ATTRF_PUNCTUATION,        /* "&" */
    INTL_ATTRF_PUNCTUATION,        /* "'" */
    INTL_ATTRF_PUNCTUATION,        /* "(" */
    INTL_ATTRF_PUNCTUATION,        /* ")" */
    INTL_ATTRF_PUNCTUATION,        /* "*" */
    INTL_ATTRF_PUNCTUATION,        /* "+" */
    INTL_ATTRF_PUNCTUATION,        /* "," */
    INTL_ATTRF_PUNCTUATION,        /* "-" */
    INTL_ATTRF_PUNCTUATION,        /* "." */
    INTL_ATTRF_PUNCTUATION,        /* "/" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "0" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "1" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "2" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "3" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "4" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "5" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "6" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "7" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "8" */
    INTL_ATTRF_DECIMAL_DIGIT,      /* "9" */
    INTL_ATTRF_PUNCTUATION,        /* ":" */
    INTL_ATTRF_PUNCTUATION,        /* ";" */
    INTL_ATTRF_PUNCTUATION,        /* "<" */
    INTL_ATTRF_PUNCTUATION,        /* "=" */
    INTL_ATTRF_PUNCTUATION,        /* ">" */
    INTL_ATTRF_PUNCTUATION,        /* "?" */
    0,                             /* "@" */
    INTL_ATTRF_UPPERCASE,          /* "A" */
    INTL_ATTRF_UPPERCASE,          /* "B" */
    INTL_ATTRF_UPPERCASE,          /* "C" */
    INTL_ATTRF_UPPERCASE,          /* "D" */
    INTL_ATTRF_UPPERCASE,          /* "E" */
    INTL_ATTRF_UPPERCASE,          /* "F" */
    INTL_ATTRF_UPPERCASE,          /* "G" */
    INTL_ATTRF_UPPERCASE,          /* "H" */
    INTL_ATTRF_UPPERCASE,          /* "I" */
    INTL_ATTRF_UPPERCASE,          /* "J" */
    INTL_ATTRF_UPPERCASE,          /* "K" */
    INTL_ATTRF_UPPERCASE,          /* "L" */
    INTL_ATTRF_UPPERCASE,          /* "M" */
    INTL_ATTRF_UPPERCASE,          /* "N" */
    INTL_ATTRF_UPPERCASE,          /* "O" */
    INTL_ATTRF_UPPERCASE,          /* "P" */
    INTL_ATTRF_UPPERCASE,          /* "Q" */
    INTL_ATTRF_UPPERCASE,          /* "R" */
    INTL_ATTRF_UPPERCASE,          /* "S" */
    INTL_ATTRF_UPPERCASE,          /* "T" */
    INTL_ATTRF_UPPERCASE,          /* "U" */
    INTL_ATTRF_UPPERCASE,          /* "V" */
    INTL_ATTRF_UPPERCASE,          /* "W" */
    INTL_ATTRF_UPPERCASE,          /* "X" */
    INTL_ATTRF_UPPERCASE,          /* "Y" */
    INTL_ATTRF_UPPERCASE,          /* "Z" */
    INTL_ATTRF_PUNCTUATION,        /* "[" */
    INTL_ATTRF_PUNCTUATION,        /* "\" */
    INTL_ATTRF_PUNCTUATION,        /* "]" */
    INTL_ATTRF_PUNCTUATION,        /* "^" */
    INTL_ATTRF_PUNCTUATION,        /* "_" */
    INTL_ATTRF_PUNCTUATION,        /* "`" */
    INTL_ATTRF_LOWERCASE,          /* "a" */
    INTL_ATTRF_LOWERCASE,          /* "b" */
    INTL_ATTRF_LOWERCASE,          /* "c" */
    INTL_ATTRF_LOWERCASE,          /* "d" */
    INTL_ATTRF_LOWERCASE,          /* "e" */
    INTL_ATTRF_LOWERCASE,          /* "f" */
    INTL_ATTRF_LOWERCASE,          /* "g" */
    INTL_ATTRF_LOWERCASE,          /* "h" */
    INTL_ATTRF_LOWERCASE,          /* "i" */
    INTL_ATTRF_LOWERCASE,          /* "j" */
    INTL_ATTRF_LOWERCASE,          /* "k" */
    INTL_ATTRF_LOWERCASE,          /* "l" */
    INTL_ATTRF_LOWERCASE,          /* "m" */
    INTL_ATTRF_LOWERCASE,          /* "n" */
    INTL_ATTRF_LOWERCASE,          /* "o" */
    INTL_ATTRF_LOWERCASE,          /* "p" */
    INTL_ATTRF_LOWERCASE,          /* "q" */
    INTL_ATTRF_LOWERCASE,          /* "r" */
    INTL_ATTRF_LOWERCASE,          /* "s" */
    INTL_ATTRF_LOWERCASE,          /* "t" */
    INTL_ATTRF_LOWERCASE,          /* "u" */
    INTL_ATTRF_LOWERCASE,          /* "v" */
    INTL_ATTRF_LOWERCASE,          /* "w" */
    INTL_ATTRF_LOWERCASE,          /* "x" */
    INTL_ATTRF_LOWERCASE,          /* "y" */
    INTL_ATTRF_LOWERCASE,          /* "z" */
    INTL_ATTRF_PUNCTUATION,        /* "{" */
    INTL_ATTRF_PUNCTUATION,        /* "|" */
    INTL_ATTRF_PUNCTUATION,        /* "}" */
    INTL_ATTRF_PUNCTUATION,        /* "~" */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    INTL_ATTRF_SPACE,                           /* <hardspace> */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    0,                                          /* "�" */
    0,                                          /* "�" */
    0,                                          /* "�" */
    0,                                          /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_DECIMAL_DIGIT,                   /* "�" */
    INTL_ATTRF_DECIMAL_DIGIT,                   /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    0,                                          /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_DECIMAL_DIGIT,                   /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_NUMBERS,                         /* "�" */
    INTL_ATTRF_NUMBERS,                         /* "�" */
    INTL_ATTRF_NUMBERS,                         /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE|INTL_ATTRF_LIGATURE,   /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_UPPERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE|INTL_ATTRF_LIGATURE,   /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_PUNCTUATION,                     /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
    INTL_ATTRF_LOWERCASE,                       /* "�" */
};

static const uint8 primCollationOrder[256] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,   /* " " */
    2,   /* "!" */
    3,   /* '"' */
    4,   /* "#" */
    5,   /* "$" */
    9,   /* "%" */
    10,  /* "&" */
    11,  /* "'" */
    12,  /* "(" */
    13,  /* ")" */
    14,  /* "*" */
    15,  /* "+" */
    16,  /* "," */
    17,  /* "-" */
    18,  /* "." */
    19,  /* "/" */
    20,  /* "0" */
    21,  /* "1" */
    22,  /* "2" */
    23,  /* "3" */
    24,  /* "4" */
    25,  /* "5" */
    26,  /* "6" */
    27,  /* "7" */
    28,  /* "8" */
    29,  /* "9" */
    30,  /* ":" */
    31,  /* ";" */
    32,  /* "<" */
    33,  /* "=" */
    34,  /* ">" */
    35,  /* "?" */
    36,  /* "@" */
    37,  /* "A" */
    38,  /* "B" */
    39,  /* "C" */
    40,  /* "D" */
    41,  /* "E" */
    42,  /* "F" */
    43,  /* "G" */
    44,  /* "H" */
    45,  /* "I" */
    46,  /* "J" */
    47,  /* "K" */
    48,  /* "L" */
    49,  /* "M" */
    50,  /* "N" */
    51,  /* "O" */
    52,  /* "P" */
    53,  /* "Q" */
    54,  /* "R" */
    55,  /* "S" */
    56,  /* "T" */
    57,  /* "U" */
    58,  /* "V" */
    59,  /* "W" */
    60,  /* "X" */
    61,  /* "Y" */
    62,  /* "Z" */
    63,  /* "[" */
    64,  /* "\" */
    65,  /* "]" */
    66,  /* "^" */
    67,  /* "_" */
    68,  /* "`" */
    37,  /* "a" */
    38,  /* "b" */
    39,  /* "c" */
    40,  /* "d" */
    41,  /* "e" */
    42,  /* "f" */
    43,  /* "g" */
    44,  /* "h" */
    45,  /* "i" */
    46,  /* "j" */
    47,  /* "k" */
    48,  /* "l" */
    49,  /* "m" */
    50,  /* "n" */
    51,  /* "o" */
    52,  /* "p" */
    53,  /* "q" */
    54,  /* "r" */
    55,  /* "s" */
    56,  /* "t" */
    57,  /* "u" */
    58,  /* "v" */
    59,  /* "w" */
    60,  /* "x" */
    61,  /* "y" */
    62,  /* "z" */
    69,  /* "{" */
    70,  /* "|" */
    71,  /* "}" */
    72,  /* "~" */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,   /* <hardspace> */
    76,  /* "�" */
    81,  /* "�" */
    6,   /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    70,  /* "�" */
    82,  /* "�" */
    78,  /* "�" */
    86,  /* "�" */
    88,  /* "�" */
    3,   /* "�" */
    83,  /* "�" */
    85,  /* "�" */
    87,  /* "�" */
    89,  /* "�" */
    91,  /* "�" */
    95,  /* "�" */
    98,  /* "�" */
    99,  /* "�" */
    79,  /* "�" */
    92,  /* "�" */
    84,  /* "�" */
    90,  /* "�" */
    80,  /* "�" */
    97,  /* "�" */
    96,  /* "�" */
    3,   /* "�" */
    100, /* "�" */
    101, /* "�" */
    102, /* "�" */
    77,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    39,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    40,  /* "�" */
    50,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    93,  /* "�" */
    51,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    61,  /* "�" */
    73,  /* "�" */
    75,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    39,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    41,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    45,  /* "�" */
    40,  /* "�" */
    50,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    51,  /* "�" */
    94,  /* "�" */
    51,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    61,  /* "�" */
    74,  /* "�" */
    61,  /* "�" */
};

static const uint8 secCollationOrder[256] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,   /* " " */
    1,   /* "!" */
    1,   /* '"' */
    1,   /* "#" */
    1,   /* "$" */
    1,   /* "%" */
    1,   /* "&" */
    1,   /* "'" */
    1,   /* "(" */
    1,   /* ")" */
    1,   /* "*" */
    1,   /* "+" */
    1,   /* "," */
    1,   /* "-" */
    1,   /* "." */
    1,   /* "/" */
    1,   /* "0" */
    1,   /* "1" */
    1,   /* "2" */
    1,   /* "3" */
    1,   /* "4" */
    1,   /* "5" */
    1,   /* "6" */
    1,   /* "7" */
    1,   /* "8" */
    1,   /* "9" */
    1,   /* ":" */
    1,   /* ";" */
    1,   /* "<" */
    1,   /* "=" */
    1,   /* ">" */
    1,   /* "?" */
    1,   /* "@" */
    1,   /* "A" */
    1,   /* "B" */
    1,   /* "C" */
    1,   /* "D" */
    1,   /* "E" */
    1,   /* "F" */
    1,   /* "G" */
    1,   /* "H" */
    1,   /* "I" */
    1,   /* "J" */
    1,   /* "K" */
    1,   /* "L" */
    1,   /* "M" */
    1,   /* "N" */
    1,   /* "O" */
    1,   /* "P" */
    1,   /* "Q" */
    1,   /* "R" */
    1,   /* "S" */
    1,   /* "T" */
    1,   /* "U" */
    1,   /* "V" */
    1,   /* "W" */
    1,   /* "X" */
    1,   /* "Y" */
    1,   /* "Z" */
    1,   /* "[" */
    1,   /* "\" */
    1,   /* "]" */
    1,   /* "^" */
    1,   /* "_" */
    1,   /* "`" */
    9,   /* "a" */
    2,   /* "b" */
    3,   /* "c" */
    3,   /* "d" */
    6,   /* "e" */
    2,   /* "f" */
    2,   /* "g" */
    2,   /* "h" */
    6,   /* "i" */
    2,   /* "j" */
    2,   /* "k" */
    2,   /* "l" */
    2,   /* "m" */
    3,   /* "n" */
    8,   /* "o" */
    2,   /* "p" */
    2,   /* "q" */
    2,   /* "r" */
    2,   /* "s" */
    2,   /* "t" */
    6,   /* "u" */
    2,   /* "v" */
    2,   /* "w" */
    2,   /* "x" */
    2,   /* "y" */
    2,   /* "z" */
    1,   /* "{" */
    1,   /* "|" */
    1,   /* "}" */
    1,   /* "~" */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    2,   /* <hardspace> */
    1,   /* "�" */
    1,   /* "�" */
    6,   /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    2,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    3,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    4,   /* "�" */
    3,   /* "�" */
    5,   /* "�" */
    7,   /* "�" */
    6,   /* "�" */
    8,   /* "�" */
    2,   /* "�" */
    2,   /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4,   /* "�" */
    5,   /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4,   /* "�" */
    5,   /* "�" */
    2,   /* "�" */
    2,   /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4,   /* "�" */
    6,   /* "�" */
    5,   /* "�" */
    1,   /* "�" */
    7,   /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4,   /* "�" */
    5,   /* "�" */
    5,   /* "�" */
    1,   /* "�" */
    1,   /* "�" */
    12,  /* "�" */
    11,  /* "�" */
    13,  /* "�" */
    15,  /* "�" */
    14,  /* "�" */
    16,  /* "�" */
    10,  /* "�" */
    4,   /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    9,   /* "�" */
    10,  /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    9,   /* "�" */
    10,  /* "�" */
    4,   /* "�" */
    4,   /* "�" */
    10,  /* "�" */
    9,   /* "�" */
    11,  /* "�" */
    13,  /* "�" */
    12,  /* "�" */
    1,   /* "�" */
    14,  /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    9,   /* "�" */
    10,  /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4,   /* "�" */
};

static const LigatureMap ligatureMap[] =
{
    {'�','E'},
    {'�','e'}
};

#define NUM_LIGATURES (sizeof(ligatureMap) / sizeof(LigatureMap))


static const char * const dateStrings[]=
{
    "domingo",
    "lunes",
    "martes",
    "mi�rcoles",
    "jueves",
    "viernes",
    "s�bado",

    "dom",
    "lun",
    "mar",
    "mie",
    "jue",
    "vie",
    "sab",

    "enero",
    "febrero",
    "marzo",
    "abril",
    "mayo",
    "junio",
    "julio",
    "agosto",
    "septiembe",
    "octubre",
    "noviembre",
    "diciembre",
    "lunar",

    "ene",
    "feb",
    "mar",
    "abr",
    "may",
    "jun",
    "jul",
    "ago",
    "sep",
    "oct",
    "nov",
    "dic",
    "lun",

    "AM",
    "PM"
};


/*****************************************************************************/


static int32 CompareStr(const unichar *str1, const unichar *str2,
                        const uint8 *collationTable, bool skipPunct,
                        bool firstOrder)
{
uint32  index1, index2;
unichar ch1, ch2;
unichar nextCh1, nextCh2;
uint8   order1, order2;
int32   i;

    index1  = 0;
    index2  = 0;
    nextCh1 = str1[0];
    nextCh2 = str2[0];

    /* Compare the strings according to the collation ordering table supplied.
     *
     * Ligatures are handled semi-automatically. The sorting order
     * for ligatures is set the same as the sorting order of the
     * first character in the ligature. Once we have looked at the
     * sorting order, and before moving on to the next character, we look to
     * see if the current character is a ligature. If it is, then we do not
     * read a new character from the source string, and instead set the
     * current character to the second character of the ligature. The sort
     * then continues. This has the effect of treating the ligature as two
     * separate characters, which is how they should be sorted.
     *
     * This code works just like the English language version, except
     * that CH sequences sort as a single character, after C and before D.
     *
     * If characters exist within the string which exceed 0x00ff, then the
     * string compare degenerates into a purely code-based comparison.
     */

    while (TRUE)
    {
        ch1 = nextCh1;
        ch2 = nextCh2;

        if ((ch1 == 'C' || ch1 == 'c') && (ch2 == 'C' || ch2 == 'c'))
        {
            if ((str1[index1 + 1] == 'H') || (str1[index1 + 1] == 'h') ||
                (str2[index2 + 1] == 'H') || (str2[index2 + 1] == 'h'))
            {
                if (str1[index1 + 1] == 'H')
                {
                    if (firstOrder)
                    {
                        if ((str2[index2 + 1] != 'h') && (str2[index2 + 1] != 'H'))
                            return 1;
                    }
                    else
                    {
                        if (str2[index2 + 1] == 'h')
                            return -1;

                        if (str2[index2 + 1] != 'H')
                            return 1;
                    }
                }
                else if (str1[index1 + 1] == 'h')
                {
                    if (firstOrder)
                    {
                        if ((str2[index2 + 1] != 'h') && (str2[index2 + 1] != 'H'))
                            return 1;
                    }
                    else
                    {
                        if (str2[index2 + 1] == 'H')
                            return 1;

                        if (str2[index2 + 1] != 'h')
                            return -1;
                    }
                }
            }
        }

        if (ch1 == ch2)
        {
            if (ch1 == 0)
                return (0);

            nextCh1 = str1[++index1];
            nextCh2 = str2[++index2];
        }
        else if ((ch1 > 0x00ff) || (ch2 > 0x00ff))
        {
            return (ch1 < ch2 ? -1 : 1);
        }
        else if (skipPunct && (charAttrs[ch1] & INTL_ATTRF_PUNCTUATION))
        {
            nextCh1 = str1[++index1];
            if (charAttrs[ch2] & INTL_ATTRF_PUNCTUATION)
                nextCh2 = str2[++index2];
        }
        else if (skipPunct && (charAttrs[ch2] & INTL_ATTRF_PUNCTUATION))
        {
            nextCh2 = str2[++index2];
        }
        else
        {
            order1 = collationTable[ch1];
            order2 = collationTable[ch2];

            if (order1 != order2)
                return (order1 < order2 ? -1 : 1);

            if (charAttrs[ch1] & INTL_ATTRF_LIGATURE)
            {
                for (i = NUM_LIGATURES-1; i >= 0; i--)
                    if (ch1 == ligatureMap[i].lm_Ligature)
                        nextCh1 = ligatureMap[i].lm_SecondChar;
            }
            else
            {
                nextCh1 = str1[++index1];
            }

            if (charAttrs[ch2] & INTL_ATTRF_LIGATURE)
            {
                for (i = NUM_LIGATURES-1; i >= 0; i--)
                    if (ch2 == ligatureMap[i].lm_Ligature)
                        nextCh2 = ligatureMap[i].lm_SecondChar;
            }
            else
            {
                nextCh2 = str2[++index2];
            }
        }
    }
}


/*****************************************************************************/


static int32 CompareStrings(const unichar *str1, const unichar *str2)
{
int32 result;

    result = CompareStr(str1,str2,primCollationOrder,TRUE,TRUE);
    if (result == 0)
        result = CompareStr(str1,str2,secCollationOrder,FALSE,FALSE);

    return (result);
}


/*****************************************************************************/


static bool GetDateStr(DateComponents dc, unichar *result, uint32 resultSize)
{
uint32 i;

    if (dc > PM)
        return (FALSE);

    i = 0;
    while (dateStrings[dc][i] && (i < resultSize - 1))
    {
        result[i] = dateStrings[dc][i];
        i++;
    }
    result[i] = 0;

    return (TRUE);
}


/*****************************************************************************/


static const LanguageDriverInfo driverInfo =
{
    sizeof(LanguageDriverInfo),

    CompareStrings,
    NULL,
    NULL,
    GetDateStr
};


LanguageDriverInfo *main(void)
{
    return (&driverInfo);
}
