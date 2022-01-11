/* @(#) varargs_glue.s 96/06/10 1.2 */

#include <hardware/PPCMacroequ.i>
#include <kernel/PPCtypes.i>
#include <kernel/PPCitem.i>

	DECVASTUB_1 internalCreateTask
	DECVASTUB_1 CreateItem
	DECVASTUB_2 CreateModuleThread
	DECVASTUB_4 CreateThread
	DECVASTUB_2 CreateTask
	DECVASTUB_1 FindAndOpenItem
	DECVASTUB_1 FindItem

#ifdef BUILD_MEMDEBUG
        DECVASTUB_0 RationMemDebug
        DECVASTUB_0 DumpMemDebug
#endif


/*****************************************************************************/


	.struct LocalFrame
lf_BackChain	.long 1
lf_CalleeLR	.long 1
lf_LR		.long 1
lf_Tag0		.byte TagArg		// for TAG_ITEM_NAME
lf_Tag1		.byte TagArg		// for TAG_ITEM_PRI
lf_R6		.long 1
lf_R7		.long 1
lf_R8		.long 1
lf_R9		.long 1
lf_R10		.long 1
// remaining of tag list follows on the stack
	.ends

	DECFN	CreateNamedItemVA
	lwz	r11,0(r1)  			// load previous SP
	stwu	r11,-(LocalFrame-8)(r1)		// extend stack, storing previous SP
	stw	r10,LocalFrame.lf_R10(r1)	// save pertinent registers into new stack area
	stw	r9,LocalFrame.lf_R9(r1)
	stw	r8,LocalFrame.lf_R8(r1)
	stw	r7,LocalFrame.lf_R7(r1)
	stw	r6,LocalFrame.lf_R6(r1)

	li	r6,TAG_ITEM_NAME
	cmpi	r4,0
	bne-	0$
	li	r6,TAG_NOP
0$:	stw	r6,LocalFrame.lf_Tag0.ta_Tag(r1)
	stw	r4,LocalFrame.lf_Tag0.ta_Arg(r1)

	li	r6,TAG_ITEM_PRI
	stw	r6,LocalFrame.lf_Tag1.ta_Tag(r1)
	stw	r5,LocalFrame.lf_Tag1.ta_Arg(r1)

	li	r5,0				// size parameter for CreateSizedItem()
	la	r4,LocalFrame.lf_Tag0(r1)	// begining of tag array
	mflr	r11				// get current LR
	stw	r11,LocalFrame.lf_LR(r1)	// save into dedicated location into EABI frame
	bl	CreateSizedItem			// do function and return
	lwz	r11,LocalFrame.lf_LR(r1)	// fetch original LR
	mtlr	r11				// set original LR
	lwz	r11,LocalFrame.lf_BackChain(r1)	// fetch old SP
	stwu	r11,LocalFrame-8(r1)		// store old SP and restore old frame
	blr					// return to caller


/*****************************************************************************/


	.struct LocalFrame2
lf_BackChain	.long 1
lf_CalleeLR	.long 1
lf_LR		.long 1
lf_R5		.long 1
lf_R6		.long 1
lf_R7		.long 1
lf_R8		.long 1
lf_R9		.long 1
lf_R10		.long 1
// remaining of tag list follows on the stack
	.ends

	DECFN	CreateSizedItemVA
	lwz	r11,0(r1)  		// load previous SP
	stwu	r11,-(LocalFrame2-8)(r1)	// extend stack, storing previous SP
	stw	r10,LocalFrame2.lf_R10(r1)		// save pertinent registers into new stack area
	stw	r9,LocalFrame2.lf_R9(r1)
	stw	r8,LocalFrame2.lf_R8(r1)
	stw	r7,LocalFrame2.lf_R7(r1)
	stw	r6,LocalFrame2.lf_R6(r1)
	stw	r5,LocalFrame2.lf_R5(r1)

	mr	r5,r4			// size argument
	la	r4,LocalFrame2.lf_R5(r1)	// begining of tag array
	mflr	r11			// get current LR
	stw	r11,LocalFrame2.lf_LR(r1)// save into dedicated location into EABI frame
	bl	CreateSizedItem		// do function and return
	lwz	r11,LocalFrame2.lf_LR(r1)// fetch original LR
	mtlr	r11			// set original LR
	lwz	r11,LocalFrame2.lf_BackChain(r1)	// fetch old SP
	stwu	r11,LocalFrame2-8(r1)	// store old SP and restore old frame
	blr				// return to caller


/*****************************************************************************/


/* A bunch of simple routines to be used by the DECVASTUB macro.
 * What these do is create a stack frame to replace the current one,
 * and stash registers into this stack frame. The goal here is to create
 * an array of TagArg structures suitable for the tag processor.
 *
 * In case you don't know, the problem is that for varargs functions, EABI
 * defines that the first 8 parameters go into registers, while the
 * rest go on the stack. Our TagArg functions need to have everything in
 * contiguous memory. So that's what we try to do here.
 */


	DECFN __tagCall_0
	lwz	r11,0(r1)  	// load previous SP
	stwu	r11,-36(r1)	// extend stack, storing previous SP
	stw	r10,40(r1)	// save pertinent registers into new stack area
	stw	r9,36(r1)
	stw	r8,32(r1)
	stw	r7,28(r1)
	stw	r6,24(r1)
	stw	r5,20(r1)
	stw	r4,16(r1)
	stw	r3,12(r1)
	la	r3,12(r1)	// begining of tag array
	mflr	r11		// get current LR
	stw	r11,8(r1)	// save into dedicated location into EABI frame
	mtlr	r0		// load function pointer
	blrl			// do function and return
	lwz	r11,8(r1)	// fetch original LR
	mtlr	r11		// set original LR
	lwz	r11,0(r1)	// fetch old SP
	stwu	r11,36(r1)	// store old SP and restore old frame
	blr			// return to caller

	DECFN	__tagCall_1
	lwz	r11,0(r1)  	// load previous SP
	stwu	r11,-32(r1)	// extend stack, storing previous SP
	stw	r10,36(r1)	// save pertinent registers into new stack area
	stw	r9,32(r1)
	stw	r8,28(r1)
	stw	r7,24(r1)
	stw	r6,20(r1)
	stw	r5,16(r1)
	stw	r4,12(r1)
	la	r4,12(r1)	// begining of tag array
	mflr	r11		// get current LR
	stw	r11,8(r1)	// save into dedicated location into EABI frame
	mtlr	r0		// load function pointer
	blrl			// do function and return
	lwz	r11,8(r1)	// fetch original LR
	mtlr	r11		// set original LR
	lwz	r11,0(r1)	// fetch old SP
	stwu	r11,32(r1)	// store old SP and restore old frame
	blr			// return to caller

	DECFN	__tagCall_2
	lwz	r11,0(r1)  	// load previous SP
	stwu	r11,-28(r1)	// extend stack, storing previous SP
	stw	r10,32(r1)	// save pertinent registers into new stack area
	stw	r9,28(r1)
	stw	r8,24(r1)
	stw	r7,20(r1)
	stw	r6,16(r1)
	stw	r5,12(r1)
	la	r5,12(r1)	// begining of tag array
	mflr	r11		// get current LR
	stw	r11,8(r1)	// save into dedicated location into EABI frame
	mtlr	r0		// load function pointer
	blrl			// do function and return
	lwz	r11,8(r1)	// fetch original LR
	mtlr	r11		// set original LR
	lwz	r11,0(r1)	// fetch old SP
	stwu	r11,28(r1)	// store old SP and restore old frame
	blr			// return to caller

	DECFN	__tagCall_3
	lwz	r11,0(r1)  	// load previous SP
	stwu	r11,-24(r1)	// extend stack, storing previous SP
	stw	r10,28(r1)	// save pertinent registers into new stack area
	stw	r9,24(r1)
	stw	r8,20(r1)
	stw	r7,16(r1)
	stw	r6,12(r1)
	la	r6,12(r1)	// begining of tag array
	mflr	r11		// get current LR
	stw	r11,8(r1)	// save into dedicated location into EABI frame
	mtlr	r0		// load function pointer
	blrl			// do function and return
	lwz	r11,8(r1)	// fetch original LR
	mtlr	r11		// set original LR
	lwz	r11,0(r1)	// fetch old SP
	stwu	r11,24(r1)	// store old SP and restore old frame
	blr			// return to caller

	DECFN	__tagCall_4
	lwz	r11,0(r1)  	// load previous SP
	stwu	r11,-20(r1)	// extend stack, storing previous SP
	stw	r10,24(r1)	// save pertinent registers into new stack area
	stw	r9,20(r1)
	stw	r8,16(r1)
	stw	r7,12(r1)
	la	r7,12(r1)	// begining of tag array
	mflr	r11		// get current LR
	stw	r11,8(r1)	// save into dedicated location into EABI frame
	mtlr	r0		// load function pointer
	blrl			// do function and return
	lwz	r11,8(r1)	// fetch original LR
	mtlr	r11		// set original LR
	lwz	r11,0(r1)	// fetch old SP
	stwu	r11,20(r1)	// store old SP and restore old frame
	blr			// return to caller
