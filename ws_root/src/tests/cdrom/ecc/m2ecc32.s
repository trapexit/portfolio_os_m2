
#include		"m2ecc32.i"
#include		"hardware/PPCMacroequ.i"

/*****************************************************
*
* log table for galois divide
*
*****************************************************/

	.text
gfLog:	
		.byte		0,0,1,25,2,50,26,198
		.byte		3,223,51,238,27,104,199,75
		.byte		4,100,224,14,52,141,239,129
		.byte		28,193,105,248,200,8,76,113
		.byte		5,138,101,47,225,36,15,33
		.byte		53,147,142,218,240,18,130,69
		.byte		29,181,194,125,106,39,249,185
		.byte		201,154,9,120,77,228,114,166
		.byte		6,191,139,98,102,221,48,253
		.byte		226,152,37,179,16,145,34,136
		.byte		54,208,148,206,143,150,219,189
		.byte		241,210,19,92,131,56,70,64
		.byte		30,66,182,163,195,72,126,110
		.byte		107,58,40,84,250,133,186,61
		.byte		202,94,155,159,10,21,121,43
		.byte		78,212,229,172,115,243,167,87
		.byte		7,112,192,247,140,128,99,13
		.byte		103,74,222,237,49,197,254,24
		.byte		227,165,153,119,38,184,180,124
		.byte		17,68,146,217,35,32,137,46
		.byte		55,63,209,91,149,188,207,205
		.byte		144,135,151,178,220,252,190,97
		.byte		242,86,211,171,20,42,93,158
		.byte		132,60,57,83,71,109,65,162
		.byte		31,45,67,216,183,123,164,118
		.byte		196,23,73,236,127,12,111,246
		.byte		108,161,59,82,41,157,85,170
		.byte		251,96,134,177,187,204,62,90
		.byte		203,89,95,176,156,169,160,81
		.byte		11,245,22,235,122,117,44,215
		.byte		79,174,213,233,230,231,173,232
		.byte		116,214,244,234,168,80,88,175

/*****************************************************
*
*       Declare macros
*
*****************************************************/
	.text

	.macro
		msallwz	&dst, &src, &int
		lwzu	&int, 84(&src)					/* int = low long-word from odd-row read	*/
		lhzu	&dst, 4(&src)					/* dst[16:31] = high half-word from odd-row read	*/
		rlwimi	&dst, &int, 16, 0, 15				/* dst = dst[16:31] | ((int << 16) & 0xFFFF)	*/
	.endm


/*****************************************************
*
*	InitECC entry point
*
*****************************************************/
	DECFN	InitECC
	blr

/*****************************************************
*
*	SectorECC entry point
*
*****************************************************/
	DECFN	SectorECC

/*	preserve registers	*/
		subi		sp,sp,StackFrame
		stw		r13,StackFrame.Int13(sp)
		stw		r14,StackFrame.Int14(sp)
		stw		r15,StackFrame.Int15(sp)
		stw		r16,StackFrame.Int16(sp)
		stw		r17,StackFrame.Int17(sp)
		stw		r18,StackFrame.Int18(sp)
		stw		r19,StackFrame.Int19(sp)
		stw		r20,StackFrame.Int20(sp)
		stw		r21,StackFrame.Int21(sp)
		mflr		SaveLR
		mfctr		SaveCTR

/*	initialize registers	*/
		lea		gfLogPtr, gfLog				/* load start of galois log table	*/

		li		StatusBits, 0				/* clear indicator bits	*/
Pchk:	
		li		pCol,0					/* pCol initially zero	*/
		rlwinm		StatusBits,StatusBits,0,0,23		/* clear error bits	*/
SectorECCLoop1:	
		bl		DetectPErr				/* returns "error bits" in r26	*/
		addi		pCol,pCol,4				/* pCol += 4	*/
		cmpwi		pCol,84					/* if (pCol <= 84)	*/
		ble		SectorECCLoop1				/* ... check the next column	*/
		
		addis		StatusBits,StatusBits,0x0010		/* indicate that we just made a pass of P	*/

Qchk:	
		li		qRow,0					/* pCol initially zero	*/
		rlwinm		StatusBits,StatusBits,0,0,23		/* clear error bits	*/
SectorECCLoop2:	
		bl		DetectQErr				/* returns "error bits" in r26	*/
		addi		qRow,qRow,2				/* qRow += 2 */
		cmpwi		qRow, 24				/* if (qRow <= 24) */
		ble		SectorECCLoop2				/* ... check the next (pair of) row(s)	*/

		addis		StatusBits,StatusBits,0x0100		/* indicate that we just made a pass of Q	*/
		
		rlwinm.		Itmp0,StatusBits,0,28,31		/* tst error bits */
		beq		SectorECCDone				/* ... getout, successfully corrected sector	*/

		andi.		Itmp0,StatusBits,0x0010			/* if (no bytes where corrected) */
		beq		SectorECCFail				/* ... indicate failure early (more P/Q does nothing) */

		andis.		Itmp0,StatusBits,0x0200			/* if (this is our second pass of Q)	*/
		beq		Pchk
		
SectorECCFail:	
		oris		StatusBits,StatusBits,0x8000		/* ... indicate ECC failed	*/
SectorECCDone:	
		rlwinm		r3,StatusBits,0,0,11			/* ... set return value (P/Q passes performed counts)	*/
		rlwimi		r3,StatusBits,24,20,31			/* ... incorporate error count into return value	*/

/*	restore registers	*/
		mtctr		SaveCTR
		mtlr		SaveLR
		lwz		r13,StackFrame.Int13(sp)
		lwz		r14,StackFrame.Int14(sp)
		lwz		r15,StackFrame.Int15(sp)
		lwz		r16,StackFrame.Int16(sp)
		lwz		r17,StackFrame.Int17(sp)
		lwz		r18,StackFrame.Int18(sp)
		lwz		r19,StackFrame.Int19(sp)
		lwz		r20,StackFrame.Int20(sp)
		lwz		r21,StackFrame.Int21(sp)
		addi		sp,sp,StackFrame
		blr							/* ... return	*/

/*****************************************************
*
*	Detect P Errors
*
*****************************************************/
DetectPErr:	
		mflr		SaveLR2

		li		s0, 0					/* s0 = 0	*/
		li		s1, 0					/* s1 = 0	*/

		add		Index, BufPtr, pCol			/* Index = BufPtr + pCol (adjust for BYTE column number)	*/
		subi		Index, Index, 84			/* pre-decrement so that pre-index works on first load (using common code)	*/

		li		Itmp0,13
		mtctr		Itmp0
DetectPErrLoop:									/* for (x = 0/* x <= 25/* x++)	*/
		lwzu		Even, 84(Index)				/* load even row	*/
		msallwz		Odd,Index,Itmp0				/* load (misaligned) odd row	*/

		xor		s0,s0,Even				/* incorporate even row to s0	*/
		xor		s0,s0,Odd				/* incorporate odd row to s0	*/

		bl		s1gensub				/* "generate" (shift-n-do-voodoo-math) s1	*/
		xor		s1,s1,Even				/* incorporate even row to s1	*/
		bl		s1gensub		 		/* "generate" (shift-n-do-voodoo-math) s1	*/
		xor		s1,s1,Odd				/* incorporate odd row to s1	*/

		bdnz		DetectPErrLoop				/* ... continue with this pCol	*/

		cmpwi		pCol,84					/* if (pCol == 84)	*/
		bne		ChkPBytes
		rlwinm		s0,s0,0,0,15				/* s0 &= 0xFFFF0000	*/
		rlwinm		s1,s1,0,0,15				/* s1 &= 0xFFFF0000	*/

/*****************************************************
*
*	Check for errors
*
*****************************************************/
ChkPBytes:	
		or.		s0s1,s0,s1				/* if (!(s0 | s1))	*/
		beq		ChkPBytesDone

/* scan s0,s1 [0:7] for error	*/

ChkPByte3:	
		andis.		Itmp1,s0s1,0xFF00			/* err in 1st byte col?	*/
		beq		ChkPByte2

		ori		StatusBits,StatusBits,0x8		/* ... yes	*/
		rlwinm.		t0,s0,8,24,31				/* extract s0 byte	*/
		beq		ChkPByte2
		rlwinm.		t1,s1,8,24,31				/* extract s1 byte	*/
		beq		ChkPByte2
		li		ByteLane,0

		bl		CorrectPByte

/* scan s0,s1 [8:15] for error	*/

ChkPByte2:	
		andis.		Itmp1,s0s1,0x00FF			/* err in 2nd byte col?	*/
		beq		ChkPByte1

		ori		StatusBits,StatusBits,0x4		/* ... yes	*/
		rlwinm.		t0,s0,16,24,31				/* extract s0 byte	*/
		beq		ChkPByte1
		rlwinm.		t1,s1,16,24,31				/* extract s1 byte	*/
		beq		ChkPByte1
		li		ByteLane,1

		bl		CorrectPByte

/* scan s0,s1 [16:23] for error	*/

ChkPByte1:	
		andi.		Itmp1,s0s1,0xFF00			/* err in 3rd byte col?	*/
		beq		ChkPByte0

		ori		StatusBits,StatusBits,0x2		/* ... yes	*/
		rlwinm.		t0,s0,24,24,31				/* extract s0 byte	*/
		beq		ChkPByte0
		rlwinm.		t1,s1,24,24,31				/* extract s1 byte	*/
		beq		ChkPByte0
		li		ByteLane,2

		bl		CorrectPByte

/* scan s0,s1 [24:31] for error	*/

ChkPByte0:	
		andi.		Itmp1,s0s1,0x00FF			/* err in 4th byte col?	*/
		beq		ChkPBytesDone

		ori		StatusBits,StatusBits,0x1		/* ... yes	*/
		rlwinm.		t0,s0,0,24,31				/* extract s0 byte	*/
		beq		ChkPBytesDone
		rlwinm.		t1,s1,0,24,31				/* extract s1 byte	*/
		beq		ChkPBytesDone
		li		ByteLane,3				/* indicate error is in odd byte lane	*/

		bl		CorrectPByte
ChkPBytesDone:	
		mtlr		SaveLR2
		blr							/* ... return	*/

/*****************************************************
*
*	Correct P errors
*
*****************************************************/
CorrectPByte:	
		lbzx		Itmp0,gfLogPtr,t1			/* logA = gfLog[s1]	(gfLog is table of BYTEs)	*/
		lbzx		Itmp1,gfLogPtr,t0			/* logB = gfLog[s0]	*/
		sub.		s1p,Itmp0,Itmp1				/* s1' = logA - logB	*/
		bge		CorrectPByteCont			/* if (logA < logB)	*/
		addi		s1p,s1p,255				/* ... s1' += 255	*/
CorrectPByteCont:	
		cmpwi		s1p,25					/* if (s1' > 25)	*/
		bgtlr							/* ... cannot correct this pCol (or qRow), so return	*/

		subfic		s1p,s1p,25				/* else, s1' = 25 - s1'	*/

		mulli		Index,s1p,86
		add		Index,Index,pCol
		add		Index,Index,ByteLane

		lbzx		Itmp0,BufPtr,Index
		xor		Itmp0,Itmp0,t0
		stbx		Itmp0,BufPtr,Index

		ori		StatusBits,StatusBits,0x10		/* Indicate >= 1 correction made */
		addi		StatusBits,StatusBits,0x100		/* StatusBits++	*/
		blr							/* ... return	*/

/*****************************************************
*
*	Detect Q Errors
*
*****************************************************/
DetectQErr:	
		mflr		SaveLR2

		li		s0,0					/* s0 = 0	*/
		li		s1,0					/* s1 = 0	*/

		mulli		Index,qRow,86				/* qRow always even	*/

		subic.		Index,Index,88				/* pre-decrement so that pre-index works on first load (using common code)	*/
		bge		DetectQErrCont1
		addi		Index,Index,2236
DetectQErrCont1:	
		lwzx		s0,BufPtr,Index
		rlwinm		s0,s0,0,16,31				/* s0 &= 0x0000FFFF	*/
		mr		s1,s0

/*	Columns 0 through 80	*/

		li		Itmp0,42
		mtctr		Itmp0
DetectQErrLoop:									/* for (x = 0/* x <= 80/* x++)	*/
		addi		Index,Index,88
		cmpwi		Index,2236
		blt		DetectQErrCont2
		subi		Index,Index,2236
DetectQErrCont2:	
		lwzx		Val,BufPtr,Index
		xor		s0,s0,Val				/* incorporate odd row to s0	*/
		bl		s1gensub					/* "generate" (shift-n-do-voodoo-math) s1	*/
		xor		s1,s1,Val				/* incorporate even row to s1	*/

		bdnz		DetectQErrLoop

/*	Column 84	*/

		addi		Index,Index,88
		cmpwi		Index,2236
		blt		DetectQErrCont3
		subi		Index,Index,2236
DetectQErrCont3:	
		lwzx		Val,BufPtr,Index

/*	get data from low half word from ECC column	*/

		subic.		qRow2,qRow,2
		bge		DetectQErrCont4
		li		qRow2,24
DetectQErrCont4:	
		mulli		Index2,qRow2,2
		addi		Index2,Index2,2236
		lwzx		Itmp0,BufPtr,Index2

		rlwimi		Val,Itmp0,0,16,31			/* mask words together	*/

		xor		s0,s0,Val				/* incorporate odd row to s0	*/
		bl		s1gensub					/* "generate" (shift-n-do-voodoo-math) s1	*/
		xor		s1,s1,Val				/* incorporate even row to s1	*/

/*	Column 86	*/
/*	get data Ecc column	*/

		mulli		Index,qRow,2
		addi		Index,Index,2236
		lwzx		Val,BufPtr,Index

		addi		Index2,Index2,2288-2236
		lwzx		Itmp0,BufPtr,Index2

		rlwimi		Val,Itmp0,0,16,31			/* mask words together	*/

		xor		s0,s0,Val				/* incorporate odd row to s0	*/
		bl		s1gensub					/* "generate" (shift-n-do-voodoo-math) s1	*/
		xor		s1,s1,Val				/* incorporate even row to s1	*/

/*	Column 88	*/

		addi		Index,Index,2288-2236
		lwzx		Val,BufPtr,Index

		rlwinm		Val,Val,0,0,15				/* Val &= 0xFFFF0000	*/

		xor		s0,s0,Val				/* incorporate odd row to s0	*/

		cmpwi		cr2,s1,0				/* s1 <<= 1, set Carry if needed	*/
		rlwimi		s1,s1,1,0,14				/* shift upper 16 bits	*/
		andis.		Itmp0,s1,0x0001
		beq		DetectQErrCont5
		xoris		s1,s1,0x0001				/* clear bit 15 */
DetectQErrCont5:	
		andis.		Itmp0,s1,0x0100				/* if bit[7] set	*/
		beq		DetectQErrCont6
		xoris		s1,s1,0x011D				/* ... generate mid-hi-byte	*/
DetectQErrCont6:	
		bge		cr2,DetectQErrCont7				/* if carry set	*/
		xoris		s1,s1,0x1D00				/* ... generate MSB	*/
DetectQErrCont7:	
		xor		s1,s1,Val				/* incorporate even row to s1	*/

/*****************************************************
*
*	Check for errors
*
*****************************************************/
ChkQBytes:	
		or.		s0s1,s0,s1				/* if (!(s0 | s1))	*/
		beq		ChkQBytesDone

/* scan s0,s1 [0:7] for error	*/

ChkQByte3:	
		andis.		Itmp1,s0s1,0xFF00			/* err in 1st byte col?	*/
		beq		ChkQByte2

		ori		StatusBits,StatusBits,0x8		/* ... yes	*/
		rlwinm.		t0,s0,8,24,31				/* extract s0 byte	*/
		beq		ChkQByte2
		rlwinm.		t1,s1,8,24,31				/* extract s1 byte	*/
		beq		ChkQByte2
		li		ByteLane,0
		li		ByteLane2,0

		bl		CorrectQByte

/* scan s0,s1 [8:15] for error	*/

ChkQByte2:	
		andis.		Itmp1,s0s1,0x00FF			/* err in 2nd byte col?	*/
		beq		ChkQByte1

		ori		StatusBits,StatusBits,0x4		/* ... yes	*/
		rlwinm.		t0,s0,16,24,31				/* extract s0 byte	*/
		beq		ChkQByte1
		rlwinm.		t1,s1,16,24,31				/* extract s1 byte	*/
		beq		ChkQByte1
		li		ByteLane,1
		li		ByteLane2,1

		bl		CorrectQByte

/* scan s0,s1 [16:23] for error	*/

ChkQByte1:	
		andi.		Itmp1,s0s1,0xFF00			/* err in 3rd byte col?	*/
		beq		ChkQByte0

		ori		StatusBits,StatusBits,0x2		/* ... yes	*/
		rlwinm.		t0,s0,24,24,31				/* extract s0 byte	*/
		beq		ChkQByte0
		rlwinm.		t1,s1,24,24,31				/* extract s1 byte	*/
		beq		ChkQByte0
		li		ByteLane,-86

		cmpwi		qRow,0
		bgt		ChkQByte0Cnt1
		li		ByteLane2,50
		b		ChkQByte0Cnt2
ChkQByte0Cnt1:	
		li		ByteLane2,-2
ChkQByte0Cnt2:	
		bl		CorrectQByte

/* scan s0,s1 [24:31] for error	*/

ChkQByte0:	
		andi.		Itmp1,s0s1,0x00FF			/* err in 4th byte col?	*/
		beq		ChkQBytesDone

		ori		StatusBits,StatusBits,0x1		/* ... yes	*/
		rlwinm.		t0,s0,0,24,31				/* extract s0 byte	*/
		beq		ChkQBytesDone
		rlwinm.		t1,s1,0,24,31				/* extract s1 byte	*/
		beq		ChkQBytesDone
		li		ByteLane,-85

		cmpwi		qRow,0
		bgt		ChkQByte0Cnt3
		li		ByteLane2,51
		b		ChkQByte0Cnt4
ChkQByte0Cnt3:	
		li		ByteLane2,-1
ChkQByte0Cnt4:	
		bl		CorrectQByte
ChkQBytesDone:	
		mtlr		SaveLR2
		blr							/* ... return	*/

/*****************************************************
*
*	Correct Q errors
*
*****************************************************/
CorrectQByte:	
		lbzx		Itmp0,gfLogPtr,t1			/* logA = gfLog[s1]	(gfLog is table of BYTEs)	*/
		lbzx		Itmp1,gfLogPtr,t0			/* logB = gfLog[s0]	*/
		sub.		s1p,Itmp0,Itmp1				/* s1' = logA - logB	*/
		bge		CorrectQByteCnt1			/* if (logA < logB)	*/
		addi		s1p,s1p,255				/* ... s1' += 255	*/
CorrectQByteCnt1:	
		cmpwi		s1p,44					/* if (s1' > 44)	*/
		bgtlr							/* ... cannot correct this pCol (or qRow), so return	*/

		subfic		s1p,s1p,44				/* else, s1' = 44 - s1'	*/

		cmpwi		s1p,43
		blt		CorrectQByteCnt2

/*	in ECC Column	*/

		subi		Itmp0,s1p,43
		mulli		Itmp0,Itmp0,52
		mulli		Index,qRow,2
		add.		Index,Index,Itmp0
		add		Index,Index,ByteLane2
		addi		Index,Index,2236
		b		CorrectQByteCnt4

/*	in regular Column	*/

CorrectQByteCnt2:	
		mulli		Index,qRow,86
		mulli		Itmp0,s1p,88
		add		Index,Index,Itmp0
		add.		Index,Index,ByteLane
		bge		CorrectQByteCnt3
		addi		Index,Index,2236
CorrectQByteCnt3:	
		cmpwi		Index,2236
		blt		CorrectQByteCnt4
		subi		Index,Index,2236
		b		CorrectQByteCnt3
CorrectQByteCnt4:	
		lbzx		Itmp0,BufPtr,Index
		xor		Itmp0,Itmp0,t0
		stbx		Itmp0,BufPtr,Index

		ori		StatusBits,StatusBits,0x10		/* Indicate >= 1 correction made */
		addi		StatusBits,StatusBits,0x100		/* StatusBits++	*/
		blr							/* ... return	*/

/*****************************************************
*
*	s1 galois shift
*
*****************************************************/
s1gensub:	
		cmpwi		cr2,s1,0				/* if bit[0] set	*/
		add		s1,s1,s1
		andi.		Itmp0,s1,0x0100				/* if bit[24] set	*/
		beq		byte2
		xori		s1,s1,0x011D				/* ... generate LSB	*/
byte2:	
		andis.		Itmp0,s1,0x0001				/* if bit[16] set	*/
		beq		byte3
		xori		s1,s1,0x1D00				/* ... gerenate mid-lo-byte	*/
		xoris		s1,s1,0x0001				/* ... cleanup bit in mid-hi-byte	*/
byte3:	
		andis.		Itmp0,s1,0x0100				/* if bit[8] set	*/
		beq		byte4
		xoris		s1,s1,0x011D				/* ... generate mid-hi-byte	*/
byte4:	
		bgelr		cr2					/* if carry set	*/
		xoris		s1,s1,0x1D00				/* ... generate MSB	*/
		blr

