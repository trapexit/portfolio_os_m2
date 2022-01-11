/*
	File:		M2TXiff.h

	Contains:	Prototypes for M2 Texture iff io functions	 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	To Do:
*/

/* I/O Functions */
#ifdef __MWERKS__
#ifndef applec
#define applec
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef applec

#ifdef __MWERKS__
#pragma only_std_keywords off
#endif

#ifndef __FILES__
#include <Files.h>
#endif

#ifdef __MWERKS__
#pragma only_std_keywords reset
#pragma ANSI_strict reset
#endif

M2Err M2TX_ReadMacFile(const FSSpec *spec, M2TX *tex);
M2Err M2TX_WriteMacFile(const FSSpec *spec, M2TX *tex);

#endif

#ifdef __cplusplus
}
#endif



