/*
 *    @(#) cltcopy.c 95/08/09 1.3
 *  Copyright 1994, The 3DO Company
 */

#include <stdlib.h>
#include "kerneltypes.h"
#include "clt.h"

void
CLT_CopySnippet(CltSnippet *dest, CltSnippet *src)
{
	int32 i = src->size;
	uint32 *sp = src->data;
	uint32 *dp = dest->data;
	while (i-- > 0) *dp++ = *sp++;
}

void
CLT_CopySnippetData(uint32 **pdp, CltSnippet *src)
{
	int32 i = src->size;
	uint32 *sp = src->data;
	uint32 *dp = *pdp;
	while (i-- > 0) *dp++ = *sp++;
	*pdp = dp;
}

void
CLT_InitSnippet(CltSnippet *s)
{
	s->data = NULL;
	s->size = 0;
}

int
CLT_AllocSnippet(CltSnippet *s, uint32 n)
{
	s->data = (uint32 *)malloc(n*4);
	if (s->data == NULL) {
		return -1;
	}
	s->allocated = n;
	return 0;
}

void
CLT_FreeSnippet(CltSnippet *s)
{
	if (s->data != NULL) 
	  free(s->data);
}
