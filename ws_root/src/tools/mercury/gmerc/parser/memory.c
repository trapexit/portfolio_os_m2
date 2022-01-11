#include <stdio.h>
#include <stdlib.h>
#include "parsertypes.h"

#define DPRTM(x)  /* printf x; */

typedef struct memHdr
{
    struct memHdr *next;
    char data;
} memHdr;

memHdr *memStart;
memHdr *memNext;

void InitMemHeader(void);
void FreeAll(void);

void InitMemHeader(void)
{
    memStart = memNext = NULL;
}

void *mymalloc(uint32 size)
{
    memHdr *mem;

    mem = (memHdr *)malloc(sizeof(memHdr) + size);
    DPRTM(("malloc 0x%lx\n", mem));
    if (mem == NULL)
    {
        return(mem);
    }

    if (memNext)
    {
        memNext->next = mem;
    }
    else
    {
        memStart = mem;
    }
    memNext = mem;
    mem->next = NULL;
    return((void *)&mem->data);
}

void FreeAll(void)
{
    memHdr *next;
    memHdr *mem;
    
    mem = memStart;
    while (mem)
    {
        next = mem->next;
        DPRTM(("malloc 0x%lx -\n", mem));
        free(mem);
        mem = next;
    }
}

