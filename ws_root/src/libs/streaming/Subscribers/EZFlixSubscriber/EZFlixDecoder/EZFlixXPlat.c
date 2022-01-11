/*
 *      @(#) EZFlixXPlat.c 96/03/04 1.4
 *
	File:		EZFlixXPlat.c

	Contains:	Cross Platform runtime for EZFlix

	Written by:	Donn Denman and John R. McMullen

	To Do:
*/

#include "EZFlixXPlat.h"

#ifndef makeformac

#include <kernel/mem.h>

Ptr NewEZFlixPtr(Size byteCount)
{
	return (Ptr) AllocMem(byteCount, MEMTYPE_ANY);
}

void DeleteEZFlixPtr(Ptr memPtr, Size byteCount)
{
	FreeMem(memPtr, byteCount);
}

#else /* makeformac */

#include <memory.h>

Ptr NewEZFlixPtr(Size byteCount)
{
	Ptr result = NewPtrClear(byteCount);
	if (result != NULL) return result;
	return NewPtrSysClear(byteCount);
}

void DeleteEZFlixPtr(Ptr memPtr, Size byteCount)
{
	DisposePtr(memPtr);
}

#endif  /* makeformac */
