
/* 	File:		m2ecc32.i */

/* 	Contains: */
/* 			ecc */

/* 	Written by: */
/* 			Greg Omi */

/* 	Copyright: */
/* 			(c) 1994 by The 3DO Company. All rights reserved. */
/* 			This material constitutes confidential and proprietary */
/* 			information of the 3DO Company and shall not be used by */
/* 			any Person or for any purpose except as expressly */
/* 			authorized in writing by the 3DO Company. */

/* 	Change History (most recent first): */

/* 	To Do: */


/* ******************************************************************************* */
/* 	General registers */
/* ******************************************************************************* */


/* placeholder for total error count (return value) */
#define BufPtr		3
#define gfLogPtr	4
#define StatusBits	5	/*	bit 	0	= error flag
					bits	1-7	= zero
				   	bits	8-11	= p passes
				   	bits	12-15	= q passes
				   	bits	16-27	= gfCounter
					bits	28-31	= ErrorBits
				*/
#define Index		6
#define pCol		7
#define qRow		7
#define s0		8
#define s1		9
#define t0		10
#define t1		11
#define s1p		12
#define s0s1		13
#define Val		14
#define Odd		14
#define Even		15
#define Index2		15
#define ByteLane	15
#define ByteLane2	16
#define qRow2		16

/* -------------------------------------------- */
/* 	Scratch registers */
/* -------------------------------------------- */

#define Itmp0		17
#define Itmp1		18

/* -------------------------------------------- */
/* Stack and counter temp registers */
/* -------------------------------------------- */

#define	SaveCTR		19
#define	SaveLR		20
#define	SaveLR2		21
/* ******************************************************************************* */
/*       Define constants	*/
/* ******************************************************************************* */

#define	kMultiErr		-1
#define	kNoErr			0
#define	kSingleErr		1
	
#define	kGoodCell		0
#define	kBadCell		1
	
/* ******************************************************************************* */
/* 	Condition Register bits used in standard compares */
/* ******************************************************************************* */

#define		LT		0
#define		GT		1
#define 	EQ		2
#define 	SO		3
#define 	cr0		0
#define 	cr1		1
#define 	cr2		2
#define 	cr3		3
#define 	cr4		4
#define 	cr5		5
#define 	cr6		6
#define 	cr7		7

/* ******************************************************************************* */
/* 	record formats */
/* ******************************************************************************* */
/* 	Stack Frame */
    .struct	StackFrame
BackChain	.long		4
Int13		.long		1
Int14		.long		1
Int15		.long		1
Int16		.long		1
Int17		.long		1
Int18		.long		1
Int19		.long		1
Int20		.long		1
Int21		.long		1

    .ends
