#ifndef _PARSER_H_
#define _PARSER_H_

#include "parsertypes.h"
#include "syntax.h"
#include <stdio.h>

extern bool printDebug;
extern bool printParse;
extern bool printDefinitions;
extern bool printTokenising;
extern int32 recursionDepth;
extern int32 fileDepth;
extern int32 lineNumber;

Err InitParser(Syntax *start);
void DeleteParser(void);
Err StartParser(char *filename, Syntax *start, char **tokens);

#endif

