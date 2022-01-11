#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H


/******************************************************************************
**
**  @(#) debug.h 96/05/23 1.26
**
**  Functions and macros to help debugging code.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __STDIO_H
#include <stdio.h>
#endif


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void kprintf(const char *fmt, ...);
void DebugPutChar(char ch);
void DebugPutStr(const char *str);

#ifdef __DCC__
__asm uint32 DebugBreakpoint(void)
{
  .long 0x00000001
}
#else
#define DebugBreakpoint()
#endif

#ifndef EXTERNAL_RELEASE
int MayGetChar(void);
#endif /* EXTERNAL_RELEASE */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


/* meaning of special characters sent to kprintf() */
#define KPRINTF_STOP	1	/* stop kprintf            */
#define KPRINTF_START	2	/* start kprintf           */
#define KPRINTF_DISABLE	4	/* disable kprintf forever */


/*****************************************************************************/


/* Always-print macros: Print even when compiled in non-DEBUG mode
 * To redirect printout, redefine APRNT on the compiler's command line. */
#ifndef APRNT
#define APRNT(printfArgs) ((void)printf printfArgs)     /* print a msg */
#endif
#define APERR(printfArgs) APRNT(printfArgs)             /* print an error msg */


/*****************************************************************************/


/* Debug message-print macros: Print, as gated by compile-time policy switches.
 *   defined(DEBUG) || PRINT_LEVEL >= 2     --> print all messages.
 *   PRINT_LEVEL == 1                       --> print error messages.
 *   PRINT_LEVEL == 0 or undefined          --> print nothing.
 *
 * The following will compile consistently whether printing is switched on or off.
 *
 *    if ( expression )
 *        PRNT(("Info = %d\n", info));
 *    else
 *        ...
 */

#ifndef PRINT_LEVEL
#define PRINT_LEVEL 0  /* default value, can be preset by the #includer */
#endif

#if defined(DEBUG) || (PRINT_LEVEL >= 2)

#define PERR(printfArgs)            APRNT(printfArgs)
#define PRNT(printfArgs)            APRNT(printfArgs)
#define ERROR_RESULT_STATUS(s, res) { APRNT(("%s:", s)); PrintfSysErr(res); }
#define PRINT_RESULT_STATUS(s, res) ERROR_RESULT_STATUS(s, res)
#define ERROR_NIL_STATUS(s)         { APRNT(("%s --> NULL\n", s)); }
#define PRINT_NIL_STATUS(s)         ERROR_NIL_STATUS(s)
#define CHECK_NEG(s, res)           { if ((res) <  0) ERROR_RESULT_STATUS(s, res) }

#elif PRINT_LEVEL == 1

#define PERR(printfArgs)            APRNT(printfArgs)
#define PRNT(printfArgs)            ((void)0)
#define ERROR_RESULT_STATUS(s, res) { APRNT(("%s:", s)); PrintfSysErr(res); }
#define PRINT_RESULT_STATUS(s, res) {}
#define ERROR_NIL_STATUS(s)         { APRNT(("%s --> NULL\n", s)); }
#define PRINT_NIL_STATUS(s)         {}
#define CHECK_NEG(s, res)           { if ((res) <  0) ERROR_RESULT_STATUS(s, res) }

#elif PRINT_LEVEL == 0

#define PERR(printfArgs)            ((void)0)
#define PRNT(printfArgs)            ((void)0)
#define ERROR_RESULT_STATUS(s, res) {}
#define PRINT_RESULT_STATUS(s, res) {}
#define ERROR_NIL_STATUS(s)         {}
#define PRINT_NIL_STATUS(s)         {}
#define CHECK_NEG(s, res)           { TOUCH(res); }

#endif


/*****************************************************************************/


/* Error handling macros  */
#define FAIL_NEG(s, res)  { if ((res) <  0) { ERROR_RESULT_STATUS(s, res); goto FAILED; } }
#define FAIL_NIL(s, ptr)  { if ((ptr) == NULL) { ERROR_NIL_STATUS(s); goto FAILED; } }


/*****************************************************************************/


#endif /* __KERNEL_DEBUG_H */
