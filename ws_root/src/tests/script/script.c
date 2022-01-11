/* @(#) script.c 96/09/04 1.1 */

#include <misc/script.h>
#include <stdio.h>

void main(void)
{
Err   result;
int32 stat;

    result = OpenScriptFolio();
    if (result >= 0)
    {
        result = ExecuteCmdLine("ShowTask shell", &stat, NULL);
        if (result >= 0)
        {
            printf("All OK\n");
        }
        CloseScriptFolio();
    }
}
