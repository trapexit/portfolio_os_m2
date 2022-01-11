/*
 * @(#) MatrixData.c 96/05/10 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include "matrix.h"

/*
 * Identity matrix2x2
 */
Matrix2x2 matrixIdentity2x2 =
	{
		{ { 1.0, 0.0 },
		  { 0.0, 1.0 } }
	};

/*
 * Identity matrix3x3
 */
Matrix3x3 matrixIdentity3x3 =
	{
		{ { 1.0, 0.0, 0.0 },
		  { 0.0, 1.0, 0.0 },
		  { 0.0, 0.0, 1.0 } }
	};

/*
 * Identity matrix
 */
Matrix matrixIdentity =
	{
		{ { 1.0, 0.0, 0.0 },
		  { 0.0, 1.0, 0.0 },
		  { 0.0, 0.0, 1.0 },
		  { 0.0, 0.0, 0.0 } }
	};

/*
 * Identity matrix4x4
 */
Matrix4x4 matrixIdentity4x4 =
	{
		{ { 1.0, 0.0, 0.0, 0.0 },
		  { 0.0, 1.0, 0.0, 0.0 },
		  { 0.0, 0.0, 1.0, 0.0 },
		  { 0.0, 0.0, 0.0, 1.0 } }
	};
