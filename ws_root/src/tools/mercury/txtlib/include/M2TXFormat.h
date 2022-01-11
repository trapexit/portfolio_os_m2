/*
	File:		M2TXFormat.h

	Contains:	M2 Texture Library, TexFormat manipulation functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2+>	 5/30/95	TMA		Made C++-friendly.
		 <2>	 5/17/95	TMA		Update Get functions API.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/
#ifdef __cplusplus
extern "C" {
#endif

uint8 M2TXFormat_GetCDepth(M2TXFormat format);
bool M2TXFormat_GetFHasSSB(M2TXFormat format);
uint8 M2TXFormat_GetADepth(M2TXFormat format);
bool M2TXFormat_GetFHasAlpha(M2TXFormat format);
bool M2TXFormat_GetFHasColor(M2TXFormat format);
bool M2TXFormat_GetFIsLiteral(M2TXFormat format);
bool M2TXFormat_GetFIsTrans(M2TXFormat format);
M2Err M2TXFormat_SetCDepth(M2TXFormat *format, uint8 cDepth);			/* 1.03 */
M2Err M2TXFormat_SetFHasSSB(M2TXFormat *format, bool flag);
M2Err M2TXFormat_SetFHasAlpha(M2TXFormat *format, bool flag);
M2Err M2TXFormat_SetFHasColor(M2TXFormat *format, bool flag);
M2Err M2TXFormat_SetFIsLiteral(M2TXFormat *format, bool flag);
M2Err M2TXFormat_SetFIsTrans(M2TXFormat *format, bool flag);
M2Err M2TXFormat_SetADepth(M2TXFormat *format, uint8 aDepth);

#ifdef __cplusplus
}
#endif

