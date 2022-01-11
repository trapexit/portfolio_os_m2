#ifndef _PARSERERRORS_H_
#define _PARSERERRORS_H_

#include "parsertypes.h"
#include "syntax.h"

enum
{
    TOO_MANY_CLOSE_BRACES = -13,
    TOO_MANY_BRACES = -12,
    SYMBOL_TYPE_MISMATCH = -11,
    UNKNOWN_SYMBOL = -10,
    UNKNOWN_TYPE = -9,
    DUPLICATE_NAME = -8,
    NO_INCLUDE = -7,
    NO_MEMORY = -6,
    TOO_MANY_INCLUDES = -5,
    MALFORMED_FLOAT = -4,
    MALFORMED_INTEGER = -3,
    EOF_ERROR = -2,
    SYNTAX_ERROR = -1,
    NO_ERROR = 0,
    NO_MORE_DATA = 1
};

void ExplainError(Err err, Syntax *syn, char *_word, uint32 charCount);

#define UNWIND_ON_ERR(e) \
{ \
      if ((e) == EOF_ERROR) \
      { \
            e = HandleEOF(buff); \
      } \
      if ((e) < NO_ERROR) \
      { \
            ExplainError(e, syn, word, charCount); \
            if (errorDepth != -1) errorDepth = recursionDepth; \
            recursionDepth--; \
            return(e); \
      } \
      if ((e) == NO_MORE_DATA) \
      { \
            if (errorDepth != -1) errorDepth = recursionDepth; \
            return(e); \
      } \
}

#define RET_ON_ERR(e) if ((e) < NO_ERROR) return(e);

#endif
