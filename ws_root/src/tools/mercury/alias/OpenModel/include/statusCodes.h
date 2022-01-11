#ifndef _statusCodes
#define _statusCodes
/*
//
//-
//	Copyright (C) 1995, Alias|Wavefront
//
// These coded instructions, statements and computer programs contain
// unpublished information proprietary to Alias|Wavefront and
// are protected by the Canadian and US federal copyright law. They
// may not be disclosed to third parties or copied or duplicated, in 
// whole or in part, without prior written consent of 
// Alias|Wavefront
//
// Unpublished-rights reserved under the Copyright Laws of 
// the United States.
//+
*/

typedef long statusCode;

/* Universal statusCodes */
enum {
	sSuccess = 0,
	sFailure = 1,
	sInsufficientMemory,
	sInvalidArgument,
	sNameChangedToUniqueOne,
	sAlreadyCreated,
	sNoProjectEnvironment,
	sCannotDelete,
	sNoParent,
	sInvalidObject,

	sObjectInSet,				/* statusCodes for class AlSet */
	sObjectInAnotherSet,
	sObjectInExclusiveSet,
	sObjectAncestorInSet,
	sObjectDescendentInSet,
	sObjectNotAMember,

	sInvalidWireFile,

	/*
	// Expression system statusCodes
	*/

	sExprNotValidName,
	sExprNotDagObject,
	sExprNotValidCV,
	sExprNotValidParameter,
	sExprNotValidRelative,
	sExprBadVariable,
	sExprAxInsertBadRef,
	sExprAxInsertSelfRef,
	sExprAxInsertCircRef,
	sExprParseError,

	sAlreadyTrimmed,

	sObjectNotFound,
	sObjectAlreadyPresent,

	sEndOfGlobalCodes		/* don't put any codes after this */
};

#endif	/* _statusCodes */
