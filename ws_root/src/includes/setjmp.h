#ifndef __SETJMP_H
#define __SETJMP_H


/******************************************************************************
**
**  @(#) setjmp.h 96/02/20 1.10
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


struct JMP_BUF
{
    uint32 jb_CR;
    uint32 jb_LR;
    uint32 jb_R1;
    uint32 jb_R2;
    uint32 jb_GPRs[19];
    uint32 jb_FPRs[18];
    uint32 jb_FPSCR[2];
    uint32 jb_SP;
    uint32 jb_LT;
};

typedef int jmp_buf[sizeof(struct JMP_BUF) / 4];


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int setjmp(jmp_buf /*env*/);
   /* Saves its calling environment in its jmp_buf argument, for later use
    * by the longjmp function.
    * Returns: If the return is from a direct invocation, the setjmp function
    *	       returns the value zero. If the return from a call to the longjmp
    *	       function, the setjmp function returns a non zero value.
    */

extern void longjmp(jmp_buf /*env*/, int /*val*/);
   /* Restores the environment saved by the most recent call to setjmp in the
    * same invocation of the program, with the corresponding jmp_buf argument.
    * If there has been no such call, or if the function containing the call
    * to setjmp has terminated execution (eg. with a return statement) in the
    * interim, the behaviour is undefined.
    * All accessible objects have values as of the time longjmp was called,
    * except that the values of objects of automatic storage duration that do
    * not have volatile type and have been changed between the setjmp and
    * longjmp calls are indeterminate.
    * As it bypasses the usual function call and return mechanism, the longjmp
    * function shall execute correctly in contexts of interrupts, signals and
    * any of their associated functions.
    * Returns: After longjmp is completed, program execution continues as if
    *	       the corresponding call to setjmp had just returned the value
    *	       specified by val. The longjmp function cannot cause setjmp to
    *	       return the value 0; if val is 0, setjmp returns the value 1.
    */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_return longjmp
#endif


/*****************************************************************************/


#endif	/* __SETJMP_H */
