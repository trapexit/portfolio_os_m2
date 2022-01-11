/* @(#) diab_glue.S 96/04/25 1.1 */

/* a bunch of glue code that the Diab compiler uses */


/*****************************************************************************/


	.global	__itof
	.text
__itof:
	mr.	r4,r3		# save val in r4 for sign-bit
	beq-	.LFltInR3_X	# zero int input; output float zero
	bgt+	.LHandlePosFlt	# +ve int input;
	neg	r3,r3		# val = -val
.LHandlePosFlt:
	cntlzw	r5,r3		# r5 <- leading zeros
	cmpwi	r5,8		# Does all bits fit in mantissa
	bge+	.LGetExactMan0	# No loss of precision; get exact mantissa
	addi	r6,r5,25	# r6 <- bit number to look at for precision + 1
	rlwnm.	r6,r3,r6,31,31	# r6 <- precision bit
	add	r3,r3,r6
	cntlzw	r5,r3
.LGetExactMan0:
	addi	r6,r5,24
	rlwnm	r3,r3,r6,9,31	# get mantissa
	li	r6,158
	sub	r6,r6,r5

	rlwimi	r3,r6,23,1,8	# extract exponent
	rlwimi	r3,r4,0,0,0	# take the sign from original value
.LFltInR3_X:
	stw	r3,-4(r1)
	lfs	f1,-4(r1)
	blr


/*****************************************************************************/


	.global	__utof
	.text
__utof:
	cmpwi	r3,0
	beq-	.LFltInR3	# zero int input; output float zero
	cntlzw	r5,r3		# r5 <- leading zeros
	cmpwi	r5,8		# Does all bits fit in mantissa
	bge+	.LGetExactMan1	# No loss of precision; get exact mantissa
	addi	r6,r5,25	# r6 <- bit number to look at for precision + 1
	rlwnm.	r6,r3,r6,31,31	# r6 <- precision bit
	add	r3,r3,r6
	cntlzw	r5,r3
.LGetExactMan1:
	addi	r6,r5,24
	rlwnm	r3,r3,r6,9,31	# get mantissa
	li	r6,158
	sub	r6,r6,r5

	rlwimi	r3,r6,23,0,8	# extract exponent and zero sign bit
.LFltInR3:
	stw	r3,-4(r1)
	lfs	f1,-4(r1)
	blr


/*****************************************************************************/


.globl	_restfpr_14_l
.globl	_restfpr_15_l
.globl	_restfpr_16_l
.globl	_restfpr_17_l
.globl	_restfpr_18_l
.globl	_restfpr_19_l
.globl	_restfpr_20_l
.globl	_restfpr_21_l
.globl	_restfpr_22_l
.globl	_restfpr_23_l
.globl	_restfpr_24_l
.globl	_restfpr_25_l
.globl	_restfpr_26_l
.globl	_restfpr_27_l
.globl	_restfpr_28_l
.globl	_restfpr_29_l
.globl	_restfpr_30_l
.globl	_restfpr_31_l


_restfpr_14_l:
	lfs	f14,-144(r11)
_restfpr_15_l:
	lfs	f15,-136(r11)
_restfpr_16_l:
	lfs	f16,-128(r11)
_restfpr_17_l:
	lfs	f17,-120(r11)
_restfpr_18_l:
	lfs	f18,-112(r11)
_restfpr_19_l:
	lfs	f19,-104(r11)
_restfpr_20_l:
	lfs	f20,-96(r11)
_restfpr_21_l:
	lfs	f21,-88(r11)
_restfpr_22_l:
	lfs	f22,-80(r11)
_restfpr_23_l:
	lfs	f23,-72(r11)
_restfpr_24_l:
	lfs	f24,-64(r11)
_restfpr_25_l:
	lfs	f25,-56(r11)
_restfpr_26_l:
	lfs	f26,-48(r11)
_restfpr_27_l:
	lfs	f27,-40(r11)
_restfpr_28_l:
	lwz	r0,4(r11)
	lfs	f28,-32(r11)
	mtlr	r0
	lfs	f29,-24(r11)
	lfs	f30,-16(r11)
	lfs	f31,-8(r11)
	mr	sp,r11
	blr
_restfpr_29_l:
	lfs	f29,-24(r11)
_restfpr_30_l:
	lfs	f30,-16(r11)
_restfpr_31_l:
	lwz	r0,4(r11)
	lfs	f31,-8(r11)
	mtlr	r0
	mr	sp,r11
	blr


/*****************************************************************************/


.globl	_restfprs_14_l
.globl	_restfprs_15_l
.globl	_restfprs_16_l
.globl	_restfprs_17_l
.globl	_restfprs_18_l
.globl	_restfprs_19_l
.globl	_restfprs_20_l
.globl	_restfprs_21_l
.globl	_restfprs_22_l
.globl	_restfprs_23_l
.globl	_restfprs_24_l
.globl	_restfprs_25_l
.globl	_restfprs_26_l
.globl	_restfprs_27_l
.globl	_restfprs_28_l
.globl	_restfprs_29_l
.globl	_restfprs_30_l
.globl	_restfprs_31_l


_restfprs_14_l:
	lfs	f14,-72(r11)
_restfprs_15_l:
	lfs	f15,-68(r11)
_restfprs_16_l:
	lfs	f16,-64(r11)
_restfprs_17_l:
	lfs	f17,-60(r11)
_restfprs_18_l:
	lfs	f18,-56(r11)
_restfprs_19_l:
	lfs	f19,-52(r11)
_restfprs_20_l:
	lfs	f20,-48(r11)
_restfprs_21_l:
	lfs	f21,-44(r11)
_restfprs_22_l:
	lfs	f22,-40(r11)
_restfprs_23_l:
	lfs	f23,-36(r11)
_restfprs_24_l:
	lfs	f24,-32(r11)
_restfprs_25_l:
	lfs	f25,-28(r11)
_restfprs_26_l:
	lfs	f26,-24(r11)
_restfprs_27_l:
	lfs	f27,-20(r11)
_restfprs_28_l:
	lwz	r0,4(r11)
	lfs	f28,-16(r11)
	mtlr	r0
	lfs	f29,-12(r11)
	lfs	f30,-8(r11)
	lfs	f31,-4(r11)
	mr	sp,r11
	blr
_restfprs_29_l:
	lfs	f29,-12(r11)
_restfprs_30_l:
	lfs	f30,-8(r11)
_restfprs_31_l:
	lwz	r0,4(r11)
	lfs	f31,-4(r11)
	mtlr	r0
	mr	sp,r11
	blr


/*****************************************************************************/


.globl	_restf14
.globl	_restf15
.globl	_restf16
.globl	_restf17
.globl	_restf18
.globl	_restf19
.globl	_restf20
.globl	_restf21
.globl	_restf22
.globl	_restf23
.globl	_restf24
.globl	_restf25
.globl	_restf26
.globl	_restf27
.globl	_restf28
.globl	_restf29
.globl	_restf30
.globl	_restf31


_restf14:
	lfs	f14,-144(sp)
_restf15:
	lfs	f15,-136(sp)
_restf16:
	lfs	f16,-128(sp)
_restf17:
	lfs	f17,-120(sp)
_restf18:
	lfs	f18,-112(sp)
_restf19:
	lfs	f19,-104(sp)
_restf20:
	lfs	f20,-96(sp)
_restf21:
	lfs	f21,-88(sp)
_restf22:
	lfs	f22,-80(sp)
_restf23:
	lfs	f23,-72(sp)
_restf24:
	lfs	f24,-64(sp)
_restf25:
	lfs	f25,-56(sp)
_restf26:
	lfs	f26,-48(sp)
_restf27:
	lfs	f27,-40(sp)
_restf28:
	lfs	f28,-32(sp)
_restf29:
	lfs	f29,-24(sp)
_restf30:
	lfs	f30,-16(sp)
_restf31:
	lfs	f31,-8(sp)
	blr


/*****************************************************************************/


.globl	_restgpr_14
.globl	_restgpr_15
.globl	_restgpr_16
.globl	_restgpr_17
.globl	_restgpr_18
.globl	_restgpr_19
.globl	_restgpr_20
.globl	_restgpr_21
.globl	_restgpr_22
.globl	_restgpr_23
.globl	_restgpr_24
.globl	_restgpr_25
.globl	_restgpr_26
.globl	_restgpr_27
.globl	_restgpr_28
.globl	_restgpr_29
.globl	_restgpr_30
.globl	_restgpr_31


_restgpr_14:
	lwz	r14,-72(r11)
_restgpr_15:
	lwz	r15,-68(r11)
_restgpr_16:
	lwz	r16,-64(r11)
_restgpr_17:
	lwz	r17,-60(r11)
_restgpr_18:
	lwz	r18,-56(r11)
_restgpr_19:
	lwz	r19,-52(r11)
_restgpr_20:
	lwz	r20,-48(r11)
_restgpr_21:
	lwz	r21,-44(r11)
_restgpr_22:
	lwz	r22,-40(r11)
_restgpr_23:
	lwz	r23,-36(r11)
_restgpr_24:
	lwz	r24,-32(r11)
_restgpr_25:
	lwz	r25,-28(r11)
_restgpr_26:
	lwz	r26,-24(r11)
_restgpr_27:
	lwz	r27,-20(r11)
_restgpr_28:
	lwz	r28,-16(r11)
_restgpr_29:
	lwz	r29,-12(r11)
_restgpr_30:
	lwz	r30,-8(r11)
_restgpr_31:
	lwz	r31,-4(r11)
	blr


/*****************************************************************************/


.globl	_restgpr_14_l
.globl	_restgpr_15_l
.globl	_restgpr_16_l
.globl	_restgpr_17_l
.globl	_restgpr_18_l
.globl	_restgpr_19_l
.globl	_restgpr_20_l
.globl	_restgpr_21_l
.globl	_restgpr_22_l
.globl	_restgpr_23_l
.globl	_restgpr_24_l
.globl	_restgpr_25_l
.globl	_restgpr_26_l
.globl	_restgpr_27_l
.globl	_restgpr_28_l
.globl	_restgpr_29_l
.globl	_restgpr_30_l
.globl	_restgpr_31_l


_restgpr_14_l:
	lwz	r14,-72(r11)
_restgpr_15_l:
	lwz	r15,-68(r11)
_restgpr_16_l:
	lwz	r16,-64(r11)
_restgpr_17_l:
	lwz	r17,-60(r11)
_restgpr_18_l:
	lwz	r18,-56(r11)
_restgpr_19_l:
	lwz	r19,-52(r11)
_restgpr_20_l:
	lwz	r20,-48(r11)
_restgpr_21_l:
	lwz	r21,-44(r11)
_restgpr_22_l:
	lwz	r22,-40(r11)
_restgpr_23_l:
	lwz	r23,-36(r11)
_restgpr_24_l:
	lwz	r24,-32(r11)
_restgpr_25_l:
	lwz	r25,-28(r11)
_restgpr_26_l:
	lwz	r26,-24(r11)
_restgpr_27_l:
	lwz	r0,4(r11)
	lwz	r27,-20(r11)
	mtlr	r0
	lwz	r28,-16(r11)
	lwz	r29,-12(r11)
	lwz	r30,-8(r11)
	lwz	r31,-4(r11)
	mr	sp,r11
	blr

_restgpr_28_l:
	lwz	r28,-16(r11)
_restgpr_29_l:
	lwz	r29,-12(r11)
_restgpr_30_l:
	lwz	r30,-8(r11)
_restgpr_31_l:
	lwz	r0,4(r11)
	lwz	r31,-4(r11)
	mtlr	r0
	mr	sp,r11
	blr


/*****************************************************************************/


.globl	_restgprs_14_l
.globl	_restgprs_15_l
.globl	_restgprs_16_l
.globl	_restgprs_17_l
.globl	_restgprs_18_l
.globl	_restgprs_19_l
.globl	_restgprs_20_l
.globl	_restgprs_21_l
.globl	_restgprs_22_l
.globl	_restgprs_23_l
.globl	_restgprs_24_l
.globl	_restgprs_25_l
.globl	_restgprs_26_l
.globl	_restgprs_27_l
.globl	_restgprs_28_l
.globl	_restgprs_29_l
.globl	_restgprs_30_l
.globl	_restgprs_31_l


_restgprs_14_l:
	lwz	r14,-72(r11)
_restgprs_15_l:
	lwz	r15,-68(r11)
_restgprs_16_l:
	lwz	r16,-64(r11)
_restgprs_17_l:
	lwz	r17,-60(r11)
_restgprs_18_l:
	lwz	r18,-56(r11)
_restgprs_19_l:
	lwz	r19,-52(r11)
_restgprs_20_l:
	lwz	r20,-48(r11)
_restgprs_21_l:
	lwz	r21,-44(r11)
_restgprs_22_l:
	lwz	r22,-40(r11)
_restgprs_23_l:
	lwz	r23,-36(r11)
_restgprs_24_l:
	lwz	r24,-32(r11)
_restgprs_25_l:
	lwz	r25,-28(r11)
_restgprs_26_l:
	lwz	r26,-24(r11)
_restgprs_27_l:
	lwz	r0,4(r11)
	lwz	r27,-20(r11)
	mtlr	r0
	lwz	r28,-16(r11)
	lwz	r29,-12(r11)
	lwz	r30,-8(r11)
	lwz	r31,-4(r11)
	mr	sp,r11
	blr

_restgprs_28_l:
	lwz	r28,-16(r11)
_restgprs_29_l:
	lwz	r29,-12(r11)
_restgprs_30_l:
	lwz	r30,-8(r11)
_restgprs_31_l:
	lwz	r0,4(r11)
	lwz	r31,-4(r11)
	mtlr	r0
	mr	sp,r11
	blr


/*****************************************************************************/


.globl	_savefpr_14_l
.globl	_savefpr_15_l
.globl	_savefpr_16_l
.globl	_savefpr_17_l
.globl	_savefpr_18_l
.globl	_savefpr_19_l
.globl	_savefpr_20_l
.globl	_savefpr_21_l
.globl	_savefpr_22_l
.globl	_savefpr_23_l
.globl	_savefpr_24_l
.globl	_savefpr_25_l
.globl	_savefpr_26_l
.globl	_savefpr_27_l
.globl	_savefpr_28_l
.globl	_savefpr_29_l
.globl	_savefpr_30_l
.globl	_savefpr_31_l


_savefpr_14_l:
	stfs	f14,-144(r11)
_savefpr_15_l:
	stfs	f15,-136(r11)
_savefpr_16_l:
	stfs	f16,-128(r11)
_savefpr_17_l:
	stfs	f17,-120(r11)
_savefpr_18_l:
	stfs	f18,-112(r11)
_savefpr_19_l:
	stfs	f19,-104(r11)
_savefpr_20_l:
	stfs	f20,-96(r11)
_savefpr_21_l:
	stfs	f21,-88(r11)
_savefpr_22_l:
	stfs	f22,-80(r11)
_savefpr_23_l:
	stfs	f23,-72(r11)
_savefpr_24_l:
	stfs	f24,-64(r11)
_savefpr_25_l:
	stfs	f25,-56(r11)
_savefpr_26_l:
	stfs	f26,-48(r11)
_savefpr_27_l:
	stfs	f27,-40(r11)
_savefpr_28_l:
	stfs	f28,-32(r11)
_savefpr_29_l:
	stfs	f29,-24(r11)
_savefpr_30_l:
	stfs	f30,-16(r11)
_savefpr_31_l:
	stfs	f31,-8(r11)
	stw	r0,4(r11)
	blr


/*****************************************************************************/


.globl	_savefprs_14_l
.globl	_savefprs_15_l
.globl	_savefprs_16_l
.globl	_savefprs_17_l
.globl	_savefprs_18_l
.globl	_savefprs_19_l
.globl	_savefprs_20_l
.globl	_savefprs_21_l
.globl	_savefprs_22_l
.globl	_savefprs_23_l
.globl	_savefprs_24_l
.globl	_savefprs_25_l
.globl	_savefprs_26_l
.globl	_savefprs_27_l
.globl	_savefprs_28_l
.globl	_savefprs_29_l
.globl	_savefprs_30_l
.globl	_savefprs_31_l


_savefprs_14_l:
	stfs	f14,-72(r11)
_savefprs_15_l:
	stfs	f15,-68(r11)
_savefprs_16_l:
	stfs	f16,-64(r11)
_savefprs_17_l:
	stfs	f17,-60(r11)
_savefprs_18_l:
	stfs	f18,-56(r11)
_savefprs_19_l:
	stfs	f19,-52(r11)
_savefprs_20_l:
	stfs	f20,-48(r11)
_savefprs_21_l:
	stfs	f21,-44(r11)
_savefprs_22_l:
	stfs	f22,-40(r11)
_savefprs_23_l:
	stfs	f23,-36(r11)
_savefprs_24_l:
	stfs	f24,-32(r11)
_savefprs_25_l:
	stfs	f25,-28(r11)
_savefprs_26_l:
	stfs	f26,-24(r11)
_savefprs_27_l:
	stfs	f27,-20(r11)
_savefprs_28_l:
	stfs	f28,-16(r11)
_savefprs_29_l:
	stfs	f29,-12(r11)
_savefprs_30_l:
	stfs	f30,-8(r11)
_savefprs_31_l:
	stfs	f31,-4(r11)
	stw	r0,4(r11)
	blr


/*****************************************************************************/


.globl	_savef14
.globl	_savef15
.globl	_savef16
.globl	_savef17
.globl	_savef18
.globl	_savef19
.globl	_savef20
.globl	_savef21
.globl	_savef22
.globl	_savef23
.globl	_savef24
.globl	_savef25
.globl	_savef26
.globl	_savef27
.globl	_savef28
.globl	_savef29
.globl	_savef30
.globl	_savef31


_savef14:
	stfs	f14,-144(sp)
_savef15:
	stfs	f15,-136(sp)
_savef16:
	stfs	f16,-128(sp)
_savef17:
	stfs	f17,-120(sp)
_savef18:
	stfs	f18,-112(sp)
_savef19:
	stfs	f19,-104(sp)
_savef20:
	stfs	f20,-96(sp)
_savef21:
	stfs	f21,-88(sp)
_savef22:
	stfs	f22,-80(sp)
_savef23:
	stfs	f23,-72(sp)
_savef24:
	stfs	f24,-64(sp)
_savef25:
	stfs	f25,-56(sp)
_savef26:
	stfs	f26,-48(sp)
_savef27:
	stfs	f27,-40(sp)
_savef28:
	stfs	f28,-32(sp)
_savef29:
	stfs	f29,-24(sp)
_savef30:
	stfs	f30,-16(sp)
_savef31:
	stfs	f31,-8(sp)
	blr


/*****************************************************************************/


.globl	_savegpr_14
.globl	_savegpr_15
.globl	_savegpr_16
.globl	_savegpr_17
.globl	_savegpr_18
.globl	_savegpr_19
.globl	_savegpr_20
.globl	_savegpr_21
.globl	_savegpr_22
.globl	_savegpr_23
.globl	_savegpr_24
.globl	_savegpr_25
.globl	_savegpr_26
.globl	_savegpr_27
.globl	_savegpr_28
.globl	_savegpr_29
.globl	_savegpr_30
.globl	_savegpr_31


_savegpr_14:
	stw	r14,-72(r11)
_savegpr_15:
	stw	r15,-68(r11)
_savegpr_16:
	stw	r16,-64(r11)
_savegpr_17:
	stw	r17,-60(r11)
_savegpr_18:
	stw	r18,-56(r11)
_savegpr_19:
	stw	r19,-52(r11)
_savegpr_20:
	stw	r20,-48(r11)
_savegpr_21:
	stw	r21,-44(r11)
_savegpr_22:
	stw	r22,-40(r11)
_savegpr_23:
	stw	r23,-36(r11)
_savegpr_24:
	stw	r24,-32(r11)
_savegpr_25:
	stw	r25,-28(r11)
_savegpr_26:
	stw	r26,-24(r11)
_savegpr_27:
	stw	r27,-20(r11)
_savegpr_28:
	stw	r28,-16(r11)
_savegpr_29:
	stw	r29,-12(r11)
_savegpr_30:
	stw	r30,-8(r11)
_savegpr_31:
	stw	r31,-4(r11)
	blr


/*****************************************************************************/


.globl	_savegpr_14_l
.globl	_savegpr_15_l
.globl	_savegpr_16_l
.globl	_savegpr_17_l
.globl	_savegpr_18_l
.globl	_savegpr_19_l
.globl	_savegpr_20_l
.globl	_savegpr_21_l
.globl	_savegpr_22_l
.globl	_savegpr_23_l
.globl	_savegpr_24_l
.globl	_savegpr_25_l
.globl	_savegpr_26_l
.globl	_savegpr_27_l
.globl	_savegpr_28_l
.globl	_savegpr_29_l
.globl	_savegpr_30_l
.globl	_savegpr_31_l


_savegpr_14_l:
	stw	r14,-72(r11)
_savegpr_15_l:
	stw	r15,-68(r11)
_savegpr_16_l:
	stw	r16,-64(r11)
_savegpr_17_l:
	stw	r17,-60(r11)
_savegpr_18_l:
	stw	r18,-56(r11)
_savegpr_19_l:
	stw	r19,-52(r11)
_savegpr_20_l:
	stw	r20,-48(r11)
_savegpr_21_l:
	stw	r21,-44(r11)
_savegpr_22_l:
	stw	r22,-40(r11)
_savegpr_23_l:
	stw	r23,-36(r11)
_savegpr_24_l:
	stw	r24,-32(r11)
_savegpr_25_l:
	stw	r25,-28(r11)
_savegpr_26_l:
	stw	r26,-24(r11)
_savegpr_27_l:
	stw	r27,-20(r11)
_savegpr_28_l:
	stw	r28,-16(r11)
_savegpr_29_l:
	stw	r29,-12(r11)
_savegpr_30_l:
	stw	r30,-8(r11)
_savegpr_31_l:
	stw	r31,-4(r11)
	stw	r0,4(r11)
	blr


/*****************************************************************************/


.globl	_savegprs_14_l
.globl	_savegprs_15_l
.globl	_savegprs_16_l
.globl	_savegprs_17_l
.globl	_savegprs_18_l
.globl	_savegprs_19_l
.globl	_savegprs_20_l
.globl	_savegprs_21_l
.globl	_savegprs_22_l
.globl	_savegprs_23_l
.globl	_savegprs_24_l
.globl	_savegprs_25_l
.globl	_savegprs_26_l
.globl	_savegprs_27_l
.globl	_savegprs_28_l
.globl	_savegprs_29_l
.globl	_savegprs_30_l
.globl	_savegprs_31_l


_savegprs_14_l:
	stw	r14,-72(r11)
_savegprs_15_l:
	stw	r15,-68(r11)
_savegprs_16_l:
	stw	r16,-64(r11)
_savegprs_17_l:
	stw	r17,-60(r11)
_savegprs_18_l:
	stw	r18,-56(r11)
_savegprs_19_l:
	stw	r19,-52(r11)
_savegprs_20_l:
	stw	r20,-48(r11)
_savegprs_21_l:
	stw	r21,-44(r11)
_savegprs_22_l:
	stw	r22,-40(r11)
_savegprs_23_l:
	stw	r23,-36(r11)
_savegprs_24_l:
	stw	r24,-32(r11)
_savegprs_25_l:
	stw	r25,-28(r11)
_savegprs_26_l:
	stw	r26,-24(r11)
_savegprs_27_l:
	stw	r27,-20(r11)
_savegprs_28_l:
	stw	r28,-16(r11)
_savegprs_29_l:
	stw	r29,-12(r11)
_savegprs_30_l:
	stw	r30,-8(r11)
_savegprs_31_l:
	stw	r31,-4(r11)
	stw	r0,4(r11)
	blr
