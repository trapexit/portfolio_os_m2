/****
 *
 *	@(#) mtx.h 95/11/26 1.36
 *	Copyright 1994, The 3DO Company
 *
 * Transform: 4x4 Matrix transformation class
 *
 *
 ****/
#ifndef _GPMTX
#define _GPMTX

typedef float MatrixData[4][4];


/*
 * C Function Dispatch Table for Character Virtual Functions
 * A Transform is also a GfxObj so the GfxObj functions must
 * go at the top of this table. If a function is added to GfxObj,
 * it must also be added here.
 */
typedef struct Transform
   {
	GfxObj		obj;					/* standard object header */
	MatrixData	data;					/* 4x4 matrix data area */
	float		hither;					/* ?? remove in 1.1 */
   }Transform;
  

#endif /* GPMTX */
