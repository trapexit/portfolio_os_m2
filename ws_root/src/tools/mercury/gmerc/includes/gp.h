/****
 *
 * @(#) gp.h 95/11/02 1.22
 *
 * Public includes for graphics pipeline
 *
 ****/
#ifndef _GP_
#define _GP_

#include "gfxtypes.h"
#include "error.h"
#include "obj.h"
#include "mtx.h"
#include "vec.h"
#include "box.h"
#include "geo.h"
#include "surf.h"
typedef struct Array
   {
	GfxObj	m_Obj;
	int8	m_IsObj;		/* TRUE if object array */
	int8	m_UserData;		/* TRUE if user manages data area */
	int16	m_ElemSize;		/* byte size of element */
	int32	m_Size;			/* number of elements added so far */
	int32	m_MaxSize;		/* maximum number of elements we have room for */
	char*	m_Data;			/* data area for array */
   } Array;

#endif
