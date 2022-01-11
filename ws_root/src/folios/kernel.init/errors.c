/* @(#) errors.c 96/06/10 1.2 */

/* Error text management */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/folio.h>
#include <kernel/kernel.h>
#include <kernel/semaphore.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>


extern const TagArg _ErrTA[];

static const TagArg semaphoreTags[2] =
{
	TAG_ITEM_NAME, (void *) "ErrorText",
	TAG_END, 0,
};


void
ErrorInit(void)
{
	ErrorText *et;
	Semaphore *s;

        s = (Semaphore *)SuperAllocNode ((Folio *) KernelBase, SEMA4NODE);
	if (s)
            KB_FIELD(kb_ErrorSemaphore) = SuperInternalCreateSemaphore (s, semaphoreTags);
        else
            KB_FIELD(kb_ErrorSemaphore) = -1;

	et = (ErrorText *)SuperAllocNode((Folio *) KernelBase,ERRORTEXTNODE);
	if (et)
            SuperInternalCreateErrorText(et,_ErrTA);
}
