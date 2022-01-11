
/******************************************************************************
**
**  @(#) memdebug.c 96/01/22 1.7
**
******************************************************************************/

/**
|||	AUTODOC -public -class Examples -name memdebug
|||	Demonstrates the memory debugging subsystem.
|||
|||	  Synopsis
|||
|||	    memdebug
|||
|||	  Description
|||
|||	    This program demonstrates the features of the memory debugging subsystem.
|||
|||	    The memory debugging subsystem helps you make sure that your
|||	    programs are freeing all of their memory, not stomping on innocent
|||	    memory areas, and generally doing illegal things to the Portfolio
|||	    memory manager. As it detects errors and illegal operations, it
|||	    displays information in the debugging terminal.
|||
|||	    For more information about the memory debugging subsystem, refer to
|||	    the documentation for the CreateMemDebug() function.
|||
|||	  Caveats
|||
|||	    This program intentionally does some illegal things. Don't do this
|||	    in your programs!
|||
|||	  Associated Files
|||
|||	    memdebug.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#define MEMDEBUG

#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <stdio.h>


/*****************************************************************************/


int main(void)
{
void   *mem;
uint32 *ptr;
Err     err;
uint32  i;

    printf("This program does a number of dubious things with the memory\n");
    printf("allocator, to see if MemDebug will catch these.\n");

    /* initialize the memory debugging subsystem */
    err = CreateMemDebug(NULL);
    if (err >= 0)
    {
        err = ControlMemDebug(MEMDEBUGF_PAD_COOKIES |
                              MEMDEBUGF_ALLOC_PATTERNS |
                              MEMDEBUGF_FREE_PATTERNS |
                              MEMDEBUGF_CHECK_ALLOC_FAILURES |
                              MEMDEBUGF_KEEP_TASK_DATA);
        if (err >= 0)
        {
            /* test what happens when we allocate nothing */
            AllocMem(0,0);

            /* test what happens when we ask for too much */
            AllocMem(123456789,0);

            /* test a NULL pointer */
            FreeMem(NULL,0);

            /* should say there's nothing to say... */
            DumpMemDebug(NULL);

            /* test a bogus pointer */
            FreeMem((void *)1,0);

            /* check if DumpMemDebug() will list these correctly */
            AllocMem(123,MEMTYPE_NORMAL);
            AllocMem(234,MEMTYPE_NORMAL);
            AllocMem(345,MEMTYPE_NORMAL);
            AllocMem(456,MEMTYPE_NORMAL);

            /* check a free operation with the wrong size */
            mem = AllocMem(123,MEMTYPE_NORMAL);
            FreeMem(mem,234);

            /* trash the cookie before the allocation and see if it is noticed */
            ptr = (uint32 *)AllocMem(333,MEMTYPE_NORMAL);
            ptr--;
            *ptr = 0;
            ptr++;

            /* see if the trashed cookie is detected */
            SanityCheckMemDebug(NULL,NULL);

            /* the trashed cookie should be reported once again by this call */
            FreeMem(ptr,333);

            /* try out MEMTYPE_TRACKSIZE handling */
            ptr = (uint32 *)AllocMem(373,MEMTYPE_TRACKSIZE);
            FreeMem(ptr,-1);
            ptr = (uint32 *)AllocMem(373,MEMTYPE_TRACKSIZE);
            FreeMem(ptr,373);

            /* output everything still allocated */
            DumpMemDebug(NULL);

            RationMemDebugVA(RATIONMEMDEBUG_TAG_ACTIVE,   TRUE,
                             RATIONMEMDEBUG_TAG_INTERVAL, 5,
                             RATIONMEMDEBUG_TAG_VERBOSE,  TRUE,
                             TAG_END);

            for (i = 0; i <= 20; i++)
            {
                ptr = AllocMem(123, MEMTYPE_NORMAL);
                if (!ptr)
                {
                    printf("Allocation %d was rationed\n",i);
                }
                else
                {
                    FreeMem(ptr, 123);
                }
            }

            RationMemDebugVA(RATIONMEMDEBUG_TAG_ACTIVE, FALSE, TAG_END);
        }
        else
        {
            printf("ControlMemDebug() failed: ");
            PrintfSysErr(err);
        }

        DeleteMemDebug();
    }
    else
    {
        printf("CreateMemDebug() failed: ");
        PrintfSysErr(err);
    }

    return 0;
}
