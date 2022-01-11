
/******************************************************************************
**
**  @(#) cltcopy.c 96/07/09 1.9
**
******************************************************************************/

#include <graphics/clt/clt.h>
#include <string.h>
#include <stdlib.h>


void CLT_CopySnippet(CltSnippet *dest, const CltSnippet *src)
{
    memcpy(dest->data, src->data, src->size*4);
}


void CLT_CopySnippetData(uint32 **pdp, const CltSnippet *src)
{
    memcpy(*pdp, src->data, src->size*4);
    *pdp = &(*pdp)[src->size];
}


void CLT_InitSnippet(CltSnippet *s)
{
    s->data      = NULL;
    s->allocated = 0;
    s->size      = 0;
}


Err CLT_AllocSnippet(CltSnippet *s, uint32 n)
{
    CLT_FreeSnippet(s);

    s->data = malloc(n*4);
    if (s->data == NULL)
    {
#ifdef UNIXSIM
        return -1;
#else
        return CLT_ERR_NOMEM;
#endif
    }


    s->allocated = n;
    return 0;
}


void CLT_FreeSnippet(CltSnippet *s)
{
    if (s->allocated)
    {
        free(s->data);
        s->data      = NULL;
        s->allocated = 0;
    }
}
