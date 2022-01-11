/* @(#) da.c 96/05/17 1.6 */
/* $Id: da.c,v 1.3 1994/05/12 19:19:55 vertex Exp $ */

/* Danish language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


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
    INTL_ATTRF_LOWERCASE                        /* "�" */
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
    66,  /* "[" */
    67,  /* "\" */
    68,  /* "]" */
    69,  /* "^" */
    70,  /* "_" */
    71,  /* "`" */
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
    72,  /* "{" */
    73,  /* "|" */
    74,  /* "}" */
    75,  /* "~" */
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
    79,  /* "�" */
    84,  /* "�" */
    6,   /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    73,  /* "�" */
    85,  /* "�" */
    81,  /* "�" */
    89,  /* "�" */
    91,  /* "�" */
    3,   /* "�" */
    86,  /* "�" */
    88,  /* "�" */
    90,  /* "�" */
    92,  /* "�" */
    94,  /* "�" */
    98,  /* "�" */
    101, /* "�" */
    102, /* "�" */
    82,  /* "�" */
    95,  /* "�" */
    87,  /* "�" */
    93,  /* "�" */
    83,  /* "�" */
    100, /* "�" */
    99,  /* "�" */
    3,   /* "�" */
    103, /* "�" */
    104, /* "�" */
    105, /* "�" */
    80,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    65,  /* "�" */
    63,  /* "�" */
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
    96,  /* "�" */
    64,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    61,  /* "�" */
    76,  /* "�" */
    78,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    37,  /* "�" */
    65,  /* "�" */
    63,  /* "�" */
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
    97,  /* "�" */
    64,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    57,  /* "�" */
    61,  /* "�" */
    77,  /* "�" */
    61   /* "�" */
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
    1,   /* "�" */
    1,   /* "�" */
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
    1,   /* "�" */
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
    2,   /* "�" */
    2,   /* "�" */
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
    2,   /* "�" */
    8,   /* "�" */
    7,   /* "�" */
    9,   /* "�" */
    10,  /* "�" */
    3,   /* "�" */
    2,   /* "�" */
    4    /* "�" */
};

static const char * const dateStrings[]=
{
    "S�ndag",
    "Mandag",
    "Tirsdag",
    "Onsdag",
    "Torsdag",
    "Fredag",
    "L�rdag",

    "S�n",
    "Man",
    "Tir",
    "Ons",
    "Tor",
    "Fre",
    "L�r",

    "Januar",
    "Februar",
    "Marts",
    "April",
    "Maj",
    "Juni",
    "Juli",
    "August",
    "September",
    "Oktober",
    "November",
    "December",
    "Lunar",		/* !!! incorrect, get real word !!! */

    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "Maj",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Okt",
    "Nov",
    "Dec",
    "Lun",		/* !!! incorrect, get real word !!! */

    "AM",
    "PM"
};


/*****************************************************************************/


static int32 CompareStr(const unichar *str1, const unichar *str2,
                        const uint8 *collationTable, bool skipPunct)
{
uint32  index1, index2;
unichar ch1, ch2;
unichar nextCh1, nextCh2;
uint8   order1, order2;

    index1  = 0;
    index2  = 0;
    nextCh1 = str1[0];
    nextCh2 = str2[0];

    /* Compare the strings according to the collation ordering table supplied.
     *
     * If characters exist within the string which exceed 0x00ff, then the
     * string compare degenerates into a purely code-based comparison.
     */

    while (TRUE)
    {
        ch1 = nextCh1;
        ch2 = nextCh2;

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

            nextCh1 = str1[++index1];
            nextCh2 = str2[++index2];
        }
    }
}


/*****************************************************************************/


static int32 CompareStrings(const unichar *str1, const unichar *str2)
{
int32 result;

    result = CompareStr(str1,str2,primCollationOrder,TRUE);
    if (result == 0)
        result = CompareStr(str1,str2,secCollationOrder,FALSE);

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
