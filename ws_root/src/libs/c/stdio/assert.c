#include <kernel/debug.h>

void __assert(const char *expr, const char *file, int line)
{
    printf("Assertion failed: %s: File %s Line %d\n",expr,file,line);
}



