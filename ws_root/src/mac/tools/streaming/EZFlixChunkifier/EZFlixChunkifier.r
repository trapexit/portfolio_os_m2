/*
 *      @(#) EZFlixChunkifier.r 96/04/05 1.6
 *
	File:		EZFlixChunkifier.r

	Contains:	xxx put contents here xxx

	Written by:	Donn Denman and John R. McMullen

	To Do:
*/

#include "Types.r";
#include "SysTypes.r";
#include "version.h"
#include "CodeFragmentTypes.r"


resource 'vers' (1) {
	kCurrentReleaseMajor,
	kCurrentReleaseMinor<<4,
	kCurrentReleaseLevel,
	kCurrentReleaseDelta,
	verUS,
	kCurrentReleaseVersion,
	kCurrentReleaseVersion ", © 1993-1996 The 3DO Company"
};

resource 'cfrg' (0)
{
	{
		kPowerPC,
		kFullLib,
		kNoVersionNum,
		kNoVersionNum,
		kDefaultStackSize,
		kNoAppSubFolder,
		kIsApp,
		kOnDiskFlat,
		kZeroOffset,
		kWholeFork,
		"EZFlixChunkifier"
	}
};
