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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef applec
#include "M2TXiffMac.h"
#endif

M2Err M2TX_ReadFileNoLODs(char *fileName, M2TX *tex);
M2Err M2TX_ReadFile(char *fileName, M2TX *tex);
M2Err M2TX_WriteFile(char *fileName, M2TX *tex);

M2Err M2TX_WriteIFF(M2TX *tex, bool outLOD, char *fileName, 
		    bool isMac, void *spec);
M2Err M2TX_ReadIFF(M2TX *tex, bool inLOD, char *fileName, bool isMac, 
		   void *spec);

#ifdef __cplusplus
}
#endif


