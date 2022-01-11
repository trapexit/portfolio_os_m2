/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//+
*/

/*
 *	This file contains enumeration types that are used by the modeling
 *	class methods.
 *
 *	This file MUST be compilable under C as well as C++ for the
 *	C interface.  If you add to this file, please make sure C-style
 *	comments are used.
 */

#ifndef _AlModel
#define _AlModel

#ifndef _curveFormType_defined
#define _curveFormType_defined

/*
 * Curve form types
 */
typedef enum  {
	kClosed,
	kOpen,
	kPeriodic,
	kInvalidCurve
} curveFormType;

#endif

#ifndef _AlClusterRestrict_defined
#define _AlClusterRestrict_defined

/*
 * Cluster restriction types
 */
typedef enum {
	kMultiCluster,
	kExclusiveCluster
} AlClusterRestrict;

#endif

#ifndef _AlOutputType_defined
#define _AlOutputType_defined

/*
 * Output types for AlPrintf
 */
typedef enum {
	kStdout,
	kStderr, 
	kPrompt,
	kErrlog,
	kPromptNoHistory
} AlOutputType;

#endif

#endif	/* _AlModel */
