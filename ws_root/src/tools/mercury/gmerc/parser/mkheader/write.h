#ifndef _WRITE_H
#define _WRITE_H

#include "../syntax.h"
#include "../parsertypes.h"

Err WriteHeader(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err IncClassCounter(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err IncArrayCounter(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err mhFoundDefineClass(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err mhFoundDefineArray(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err mhHandleClassString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err EndClassDefinition(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err EndEnumDefinition(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err mhFoundDefineEnum(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleEnumString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleEnumValue(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

#endif
