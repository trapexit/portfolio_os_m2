#ifndef _PARSERFUNCTIONS_H
#define _PARSERFUNCTIONS_H

#include "syntax.h"
#include "parsertypes.h"

Err ConvertHexInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ConvertOctInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ConvertDecInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ConvertShortInteger(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ConvertFloat(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ConvertString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundMeters(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundKMeters(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundFeet(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundInches(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundNautMiles(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundUnitsInt(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundUnits(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundSDFVersion(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err FoundDefineEnum(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleEnumDefinition(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleInclude(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleFiles(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err IncrementClassCounter(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundDefineClass(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleClassType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleClassString(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleClassPad(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err HandleClassFrom(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err GetClassType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundClassBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err WriteDataType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err WriteDataID(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err FoundDefineArray(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundArrayType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err ParseArray(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err FoundType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundNewSym(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err BuildBitMask(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err OrBitValue(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err EndBitMask(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err SetEnumDestination(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err FoundInstanceObjectType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err FoundInstanceArrayType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err UseObjectType(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err UseObjectSymbol(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err UseArraySymbol(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err WarnGarbage(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);

Err CloseBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err OpenBrace(char **buff, char *wordIn, uint32 charCount, SyntaxResult *sr);
Err PushSizePointer(void);
Err WriteBlockSize(void);
Err WriteTokenAddress(char *pos, void *addr);
Err CheckTokenBufferOverflow(void);

#endif
