#include "parsertypes.h"
#include "parsererrors.h"
#include "parser.h"
#include "syntax.h"
#include <stdio.h>
#include <string.h>

char *ErrorStrings[] =
{
    "No Error",
    "Syntax Error",
    "End Of File",
    "Malformed Integer",
    "Malformed Float",
    "Too many includes",
    "Not enough RAM",
    "Could not open the include file",
    "Duplicate name",
    "Unknown type",
    "Unknown symbol",
    "Symbol/type mismatch",
    "Too many braces",
    "Too many '}'s found - mismatched!"
};

bool explained = FALSE;
char *GetCurrentFileName(void);
void PrintRule(Syntax *syn);

void PrintRule(Syntax *syn)
{
    SyntaxItem *si;
    int32 i;
    bool both = FALSE;
    
    if (syn)
    {
        si = syn->required;
        for (i = 0; i < syn->countReq; i++)
        {
            printf("<%s> ", (si->syntaxString ? si->syntaxString : "<default>"));
            si = si->nextSI;
        }
        if (si && syn->alternatives)
        {
            printf("(");
            both = TRUE;
        }
        if (si = syn->alternatives)
        {
            printf("<%s> ", (si->syntaxString ? si->syntaxString : "<default>"));
            si = si->nextSI;
            for (i = 1; i < syn->countAlt; i++)
            {
                if ((si->stringLen == 0) && (si->nextLevel))
                {
                    printf("| (--> rule %s)\n[", si->nextLevel->name);
                    PrintRule(si->nextLevel);
                    printf("]\n");
                }
                else
                {
                    printf("| <%s> ", (si->syntaxString ? si->syntaxString : "<default>"));
                }
                si = si->nextSI;
            }
        }
        if (both)
        {
            printf(")");
        }
    }
}

void ExplainError(Err err, Syntax *syn, char *_word, uint32 charCount)
{
    char word[64];

    if (explained)
    {
        return;
    }
    explained = TRUE;
    
    printf("----------------------\n");
    printf("Parser error %ld at depth %ld\n", err, recursionDepth);
    printf("%s: [line %d: %s]\n", ErrorStrings[-err], lineNumber, GetCurrentFileName());
    printf("in rule %s\n", syn->name);
    if (_word)
    {
        strncpy(word, _word, (charCount <= 64 ? charCount : 64));
        word[charCount] = '\0';
        printf("Found %s\n", word);
    }

    if (syn)
    {
        printf("Was looking for:\n");
        PrintRule(syn);
    }
    
    printf("\n");
    printf("----------------------\n");
}
