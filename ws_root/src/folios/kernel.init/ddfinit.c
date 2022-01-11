/* @(#) ddfinit.c 96/02/12 1.1 */

/*#define DEBUG*/

/* Routines to manipulate DDFs (Device Description Files). */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/semaphore.h>
#include <kernel/ddfnode.h>
#include <kernel/ddftoken.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <string.h>
#include <stdio.h>

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define	SYNTAX_ERROR(n)    printf("Syntax error %d\n", n)
#define	DBUG(x) printf x
#else
#define	SYNTAX_ERROR(n)
#define	DBUG(x)
#endif



Err initDDFSem(void)
{
    /* allocate DDF list semaphore */

    Semaphore* DDFSemaphore= (Semaphore*)SuperAllocNode(&KB_FIELD(kb), SEMA4NODE);
    if(!DDFSemaphore) return NOMEM;
    KB_FIELD(kb_DDFs.l_Semaphore)= SuperInternalCreateSemaphore(DDFSemaphore, 0);
    return KB_FIELD(kb_DDFs.l_Semaphore);
}
